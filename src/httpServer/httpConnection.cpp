#include "httpConnection.h"

HttpConnection::HttpConnection(HttpServerConfig *config, HttpRequestHandler *requestHandler, qintptr socketDescriptor, QSslConfiguration *sslConfig, QObject *parent) :
    QObject(parent), config(config), currentRequest(nullptr), currentResponse(nullptr), requestHandler(requestHandler), sslConfig(sslConfig)
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
        connect(sslSocket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &HttpConnection::sslErrors);
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
            currentResponse = new HttpResponse(config, this);
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

        // If a response exists, then just send that, doesn't matter if its an error or not
        if (!currentResponse->isValid())
        {
            if (config->verbosity >= HttpServerConfig::Verbose::Info)
                qInfo().noquote() << QString("Received %1 request to %2 from %3").arg(currentRequest->method()).arg(currentRequest->uriStr()).arg(address.toString());

            if (config->responseTimeout > 0)
            {
                requestHandler->handle(currentRequest, currentResponse)
                    .timeout(config->responseTimeout)
                    .finally([]() {
                        // TODO Done
                    });
            }
            else
            {
                requestHandler->handle(currentRequest, currentResponse).finally([]() {
                    // TODO Done
                });
            }

            try
            {
                const x = ;

                requestHandler->handle(currentRequest, currentResponse).finally([]() {
                    // TODO Done
                });

                // Block signals so that the finished signal is not called
                currentResponse->blockSignals(true);
                requestHandler->handle(currentRequest, currentResponse);
                currentResponse->blockSignals(false);
            }
            catch (const std::exception &e)
            {
                currentResponse->setError(HttpStatus::InternalServerError, "An error occurred while processing request");

                if (config->verbosity >= HttpServerConfig::Verbose::Warning)
                    qWarning().noquote() << QString("Encountered an exception while attempting to parse request from %1: %2").arg(address.toString()).arg(e.what());
            }
        }

        currentResponse->setupFromRequest(currentRequest);

        if (currentResponse->isValid())
        {
            // Save the request (delete after done sending response)
            requests.emplace(currentResponse, currentRequest);
            currentRequest = nullptr;

            currentResponse->prepareToSend();
            pendingResponses.push(currentResponse);
            currentResponse = nullptr;

            // If this is the only pending response, call bytesWritten to get things rolling
            if (pendingResponses.size() == 1)
                bytesWritten(0);
        }
        else
        {
            // Start response timer
            if (config->responseTimeout > 0)
            {
                QTimer *responseTimer = new QTimer(this);
                connect(responseTimer, &QTimer::timeout, this, &HttpConnection::responseTimeout);
                responseTimers.emplace(currentResponse, responseTimer);
                responseTimer->start(config->responseTimeout * 1000);
            }

            // We connect as a queued connection so that the function will be called on the next event loop. This allows additional instructions to be called
            // to the response before actually sending the data
            connect(currentResponse, &HttpResponse::finished, this, &HttpConnection::responseFinished, Qt::QueuedConnection);

            // Save the request (delete after done sending response)
            requests.emplace(currentResponse, currentRequest);
            currentRequest = nullptr;

            pendingResponses.push(currentResponse);
            currentResponse = nullptr;
        }
    }
}

void HttpConnection::bytesWritten(qint64 bytes)
{
    bool closeConnection = false;

    // Keep sending the responses until the buffer fills up
    while (!pendingResponses.empty())
    {
        // If the response has not been prepared for sending, it means we're still waiting for a response from this
        // Due to the setup of HTTP pipelining, we must send responses in the same order we received them, so we can't send anything else)
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
        auto it = requests.find(response);
        if (it != requests.end())
        {
            delete it->second;
            requests.erase(it);
        }

        // Delete response and pop from queue
        delete response;
        pendingResponses.pop();
    }

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
    // If we are in keep-alive mode (meaning this socket has already had one successful request) and there is no data that's been read,
    // we just close the socket peacefully
    if (keepAliveMode && (!currentRequest || currentRequest->state() != HttpRequest::State::ReadRequestLine))
    {
        socket->disconnectFromHost();
        return;
    }

    // Otherwise we send a request timeout response
    if (!currentResponse)
        currentResponse = new HttpResponse(config, this);

    currentResponse->setError(HttpStatus::RequestTimeout, "", true);
    currentResponse->prepareToSend();

    // Assume that the entire request will be written in one go, relatively safe assumption
    currentResponse->writeChunk(socket);

    // This will disconnect after all bytes have been written
    socket->disconnectFromHost();
}

void HttpConnection::responseTimeout()
{
    // Grab the timer that called this function and try to find the corresponding response by value in the responseTimers map
    QTimer *timer = (QTimer *)QObject::sender();
    auto it = std::find_if(responseTimers.begin(), responseTimers.end(), [timer](const std::pair<HttpResponse *, QTimer *> &x) { return x.second == timer; });

    // If the response could not be found, then we just delete the timer and do nothing
    // This should NEVER happen
    //
    // Otherwise, if the response has already been set, then it means we are currently sending the response
    // It's very unlikely this will occur
    if (it == responseTimers.end())
    {
        timer->deleteLater();
        return;
    }
    else if (it->first->isValid())
    {
        timer->deleteLater();
        responseTimers.erase(it);
        return;
    }

    HttpResponse *response = it->first;

    // This signal is used to notify the request handler to stop using the pointer because it's going to be deleted
    emit response->cancelled();

    // Dont emit finished signal
    response->setError(HttpStatus::RequestTimeout, "", false, false);
    response->prepareToSend();

    // If we were waiting on this response to be sent, then call bytesWritten to get things rolling
    if (response == pendingResponses.front())
        bytesWritten(0);

    // Delete timer (the response will be deleted after it is sent)
    timer->deleteLater();
    responseTimers.erase(it);
}

void HttpConnection::responseFinished()
{
    // Retrieve the response that called finish
    HttpResponse *response = (HttpResponse *)QObject::sender();

    // Find the corresponding timer for the response and delete it
    auto it = responseTimers.find(response);
    if (it != responseTimers.end())
    {
        it->second->deleteLater();
        responseTimers.erase(it);
    }

    response->prepareToSend();

    // If we were waiting on this response to be sent, then call bytesWritten to get things rolling
    if (response == pendingResponses.front())
        bytesWritten(0);
}

void HttpConnection::socketDisconnected()
{
    if (config->verbosity >= HttpServerConfig::Verbose::Debug)
        qDebug().noquote() << QString("Client %1 disconnected").arg(address.toString());

    timeoutTimer->stop();

    for (auto timer : responseTimers)
        timer.second->stop();

    emit disconnected();
}

void HttpConnection::sslErrors(const QList<QSslError> &errors)
{
    if (config->verbosity >= HttpServerConfig::Verbose::Warning)
    {
        // Combine all the SSL error messages into one string delineated by commas
        QString errorMessages = std::accumulate(errors.begin(), errors.end(), QString(""), [](const QString &str, const QSslError &error)
        {
            return str.isEmpty() ? error.errorString() : str + ", " + error.errorString();
        });

        qWarning().noquote() << QString("TLS handshake failed for client %1: %2").arg(address.toString()).arg(errorMessages);
    }

    // Connection will automatically disconnect
    // A response is not sent back here because it will not be encrypted
}

HttpConnection::~HttpConnection()
{
    delete timeoutTimer;
    delete socket;

    for (auto timer : responseTimers)
        delete timer.second;
    responseTimers.clear();

    // Delete pending responses
    while (!pendingResponses.empty())
    {
        delete pendingResponses.front();
        pendingResponses.pop();
    }

    // Delete pending requests
    for (auto it : requests)
        delete it.second;
    requests.clear();

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
