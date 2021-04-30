#include "httpConnection.h"

HttpConnection::HttpConnection(HttpServerConfig *config, HttpRequestHandler *requestHandler, qintptr socketDescriptor,
    QSslConfiguration *sslConfig, QObject *parent) : QObject(parent), config(config), currentRequest(nullptr),
    currentResponse(nullptr), requestHandler(requestHandler), sslConfig(sslConfig)
{
    timeoutTimer = new QTimer(this);
    keepAliveMode = false;

	// Create TCP or SSL socket
    createSocket(socketDescriptor);

	// Connect signals
    connect(socket, &QTcpSocket::readyRead, this, &HttpConnection::read);
    connect(socket, &QTcpSocket::bytesWritten, this, &HttpConnection::bytesWritten);
    connect(socket, &QTcpSocket::disconnected, this, &HttpConnection::socketDisconnected);
    connect(timeoutTimer, &QTimer::timeout, this, &HttpConnection::timeout);
}

void HttpConnection::createSocket(qintptr socketDescriptor)
{
	// If SSL is supported and configured, then create an instance of QSslSocket
    if (sslConfig)
	{
        QSslSocket *sslSocket = new QSslSocket();
        sslSocket->setSslConfiguration(*sslConfig);
        socket = sslSocket;

        // Use QOverload because there is another function sslErrors that causes issues
        // Any errors in TLS handshake will be notified via this signal
        connect(sslSocket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this,
            &HttpConnection::sslErrors);
	}
    else
        socket = new QTcpSocket();

    if (!socket->setSocketDescriptor(socketDescriptor))
    {
        if (config->verbosity >= HttpServerConfig::Verbose::Critical)
            qCritical() << QString("Invalid socket descriptor given (%1)").arg(socket->errorString());

        return;
    }

    if (config->verbosity >= HttpServerConfig::Verbose::Debug)
        qDebug().noquote() << QString("New incoming connection from %1").arg(socket->peerAddress().toString());

    // Begin TLS handshake if SSL is enabled
    if (sslConfig)
        dynamic_cast<QSslSocket *>(socket)->startServerEncryption();

    address = socket->peerAddress();

    // Begin timer for read timeout, occurs in case the client fails to send full message in a specified amount of time
    timeoutTimer->start(config->requestTimeout * 1000);
}

void HttpConnection::read()
{
    // Looping adds support for HTTP pipelining
	while (socket->bytesAvailable())
    {
        // Create new request if necessary
        if (!currentRequest)
        {
            currentRequest = new HttpRequest(config);
            currentResponse = new HttpResponse(config);
        }

        // If this returns false, that indicates there is no more data left to read
        // Otherwise, true means a request was parsed or the request was aborted
        if (!currentRequest->parseRequest(socket, currentResponse))
        {
            // If we are reading the body, give additional time for large files
            if (currentRequest->state() == HttpRequest::State::ReadBody)
                timeoutTimer->start(config->requestTimeout * 1000);

            return;
        }

        // We are done parsing data, whether it be an error or not
        timeoutTimer->stop();

        // Store request & response in map while it is processed asynchronously
        auto httpData = std::make_shared<HttpData>(currentRequest, currentResponse);
        data.emplace(currentResponse, httpData);
        pendingResponses.push(currentResponse);

        // If a response exists, then just send that, doesn't matter if its an error or not
        if (currentResponse->isValid())
        {
            currentResponse->setupFromRequest(currentRequest);
            currentRequest = nullptr;
            currentResponse = nullptr;
            return;
        }

        if (config->verbosity >= HttpServerConfig::Verbose::Info)
            qInfo().noquote() << QString("Received %1 request to %2 from %3").arg(currentRequest->method())
                .arg(currentRequest->uriStr()).arg(address.toString());

        // Handle request and setup timeout timer if necessary
        // Note: Wrap the handler in a promise so exceptions are handled correctly
        // Note: Create local copies of current request and response so they are captured by value in the lambda
        auto request = currentRequest;
        auto response = currentResponse;
        auto promise = HttpPromise::resolve(httpData).then([=](HttpDataPtr data) {
            return requestHandler->handle(data);
        });
        if (config->responseTimeout > 0)
            promise = promise.timeout(config->responseTimeout * 1000);

        promise
            .fail([=](const QPromiseTimeoutException &error) {
                // Request timed out
                response->setError(HttpStatus::RequestTimeout, "", false);
                return nullptr;
            })
            .fail([=](const HttpException &error) {
                response->setError(error.status, error.message, false);
                return nullptr;
            })
            .fail([=](const std::exception &error) {
                response->setError(HttpStatus::InternalServerError, error.what(), false);
                return nullptr;
            })
            .finally([=]() {
                // If response is already finished, don't do anything
                // This can occur if the socket is closed prematurely
                if (httpData->finished)
                    return;

                // Handle if no response is set
                // This should not happen, but handle it and warn the user
                if (!response->isValid())
                {
                    if (config->verbosity >= HttpServerConfig::Verbose::Warning)
                    {
                        qWarning().noquote() << QString("No valid response set, defaulting to 500: %1 %2 %3")
                            .arg(request->method()).arg(request->uriStr()).arg(address.toString());
                    }
                    response->setError(HttpStatus::InternalServerError, "An unknown error occurred", false);
                }

                // Send response
                httpData->finished = true;
                response->prepareToSend();

                // If we were waiting on this response to be sent, then call bytesWritten to get things rolling
                if (response == pendingResponses.front())
                    bytesWritten(0);
            });

        currentResponse->setupFromRequest(currentRequest);

        // Clear pointers for next request
        currentRequest = nullptr;
        currentResponse = nullptr;
    }
}

void HttpConnection::bytesWritten(qint64 bytes)
{
    bool closeConnection = false;

    // Keep sending the responses until the buffer fills up
    while (!pendingResponses.empty())
    {
        // If the response has not been prepared for sending, it means we're still waiting for a response
        // from this. Due to the setup of HTTP pipelining, we must send responses in the same order we received them,
        // so we can't send anything else)
        HttpResponse *response = pendingResponses.front();
        if (!response->isSending())
            break;

        // If writeChunk returns false, means buffer is full
        if (!response->writeChunk(socket))
            break;

        // Read connection header, default to keep-alive
        QString connection;
        if (!response->header("Connection", &connection))
            connection = "keep-alive";

        // If any of the responses say to close the connection, then do that
        closeConnection |= connection.contains("close", Qt::CaseInsensitive);

        // Delete the corresponding request for the response
        auto it = data.find(response);
        if (it != data.end())
        {
            it->second->finished = true;
            data.erase(it);
        }

        // Delete response and pop from queue
        pendingResponses.pop();
    }

    socket->flush();

    // If we are done sending responses, close the connection or start keep-alive timer
    if (pendingResponses.empty())
    {
        if (closeConnection)
        {
            socket->disconnectFromHost();
        }
        else
        {
            keepAliveMode = true;
            timeoutTimer->start(config->keepAliveTimeout * 1000);
        }
    }
}

void HttpConnection::timeout()
{
    // If we are in keep-alive mode (meaning this socket has already had one successful request) and there is no data
    // that's been read, we just close the socket peacefully
    if (keepAliveMode && (!currentRequest || currentRequest->state() != HttpRequest::State::ReadRequestLine))
    {
        socket->disconnectFromHost();
        return;
    }

    // Otherwise we send a request timeout response
    if (!currentResponse)
        currentResponse = new HttpResponse(config);

    currentResponse->setError(HttpStatus::RequestTimeout, "", true);
    currentResponse->prepareToSend();

    // Assume that the entire request will be written in one go, relatively safe assumption
    currentResponse->writeChunk(socket);

    // This will disconnect after all bytes have been written
    socket->disconnectFromHost();
}

void HttpConnection::socketDisconnected()
{
    if (config->verbosity >= HttpServerConfig::Verbose::Debug)
        qDebug().noquote() << QString("Client %1 disconnected").arg(address.toString());

    timeoutTimer->stop();
    emit disconnected();
}

void HttpConnection::sslErrors(const QList<QSslError> &errors)
{
    if (config->verbosity >= HttpServerConfig::Verbose::Warning)
    {
        // Combine all the SSL error messages into one string delineated by commas
        QString errorMessages = std::accumulate(errors.begin(), errors.end(), QString(""),
            [](const QString str, const QSslError &error) {
                return str.isEmpty() ? error.errorString() : str + ", " + error.errorString();
            });

        qWarning().noquote() << QString("TLS handshake failed for client %1: %2").arg(address.toString()).arg(errorMessages);
    }

    // Connection will automatically disconnect
    // A response is not sent back here because it will not be encrypted
}

HttpConnection::~HttpConnection()
{
    socket->abort();
    delete socket;
    delete timeoutTimer;

    // Delete pending responses
    while (!pendingResponses.empty())
        pendingResponses.pop();

    // Clear pending requests, will be automatically cleaned up
    for (auto it : data)
        it.second->finished = true;
    data.clear();

    if (currentRequest)
    {
        delete currentRequest;
        currentRequest = nullptr;
    }

    if (currentResponse)
    {
        delete currentResponse;
        currentResponse = nullptr;
    }
}
