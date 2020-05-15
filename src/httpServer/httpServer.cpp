#include "httpServer.h"

HttpServer::HttpServer(const HttpServerConfig &config, HttpRequestHandler *requestHandler, QObject *parent) :
    QTcpServer(parent), config(config), requestHandler(requestHandler), sslConfig(nullptr)
{
    setMaxPendingConnections(config.maxPendingConnections);
    loadSslConfig();
}

bool HttpServer::listen()
{
    if (!QTcpServer::listen(config.host, config.port))
    {
        if (config.verbosity >= HttpServerConfig::Verbose::Warning)
        {
            qWarning().noquote() << QString("Unable to listen on %1:%2: %3").arg(config.host.toString())
                .arg(config.port).arg(errorString());
        }

        return false;
    }

    if (config.verbosity >= HttpServerConfig::Verbose::Info)
        qInfo() << "Listening...";

    return true;
}

void HttpServer::close()
{
    QTcpServer::close();
}

void HttpServer::loadSslConfig()
{
    // TODO Want to handle caching SSL sessions here if able too

    if (!config.sslKeyPath.isEmpty() && !config.sslCertPath.isEmpty())
    {
        if (!QSslSocket::supportsSsl())
        {
            if (config.verbosity >= HttpServerConfig::Verbose::Warning)
            {
                qWarning().noquote() << QString("OpenSSL is not supported for HTTP server (OpenSSL Qt build "
                    "version: %1). Disabling TLS").arg(QSslSocket::sslLibraryBuildVersionString());
            }

            return;
        }

        // Load the SSL certificate
        QFile certFile(config.sslCertPath);
        if (!certFile.open(QIODevice::ReadOnly))
        {
            if (config.verbosity >= HttpServerConfig::Verbose::Warning)
            {
                qWarning().noquote() << QString("Failed to open SSL certificate file for HTTP server: %1 (%2). "
                    "Disabling TLS").arg(config.sslCertPath).arg(certFile.errorString());
            }

            return;
        }

        QSslCertificate certificate(&certFile, QSsl::Pem);
        certFile.close();

        if (certificate.isNull())
        {
            if (config.verbosity >= HttpServerConfig::Verbose::Warning)
            {
                qWarning().noquote() << QString("Invalid SSL certificate file for HTTP server: %1. Disabling TLS")
                    .arg(config.sslCertPath);
            }

            return;
        }

        // Load the key file
        QFile keyFile(config.sslKeyPath);
        if (!keyFile.open(QIODevice::ReadOnly))
        {
            if (config.verbosity >= HttpServerConfig::Verbose::Warning)
            {
                qWarning().noquote() << QString("Failed to open private SSL key file for HTTP server: %1 (%2). "
                    "Disabling TLS").arg(config.sslKeyPath).arg(keyFile.errorString());
            }

            return;
        }

        QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey, config.sslKeyPassPhrase);
        keyFile.close();

        if (sslKey.isNull())
        {
            if (config.verbosity >= HttpServerConfig::Verbose::Warning)
            {
                qWarning().noquote() << QString("Invalid private SSL key for HTTP server: %1. Disabling TLS")
                    .arg(config.sslKeyPath);
            }

            return;
        }

        sslConfig = new QSslConfiguration();
        sslConfig->setLocalCertificate(certificate);
        sslConfig->setPrivateKey(sslKey);
        sslConfig->setPeerVerifyMode(QSslSocket::VerifyNone);
        sslConfig->setProtocol(QSsl::SecureProtocols);

        if (config.verbosity >= HttpServerConfig::Verbose::Debug)
            qDebug().noquote() << "Successfully setup SSL configuration, HTTPS enabled";
    }
    else if (config.verbosity >= HttpServerConfig::Verbose::Debug)
        qDebug().noquote() << "No private key or certificate file path given. Disabling TLS";
}

void HttpServer::incomingConnection(qintptr socketDescriptor)
{
    if ((int)connections.size() >= config.maxConnections)
    {
        // Create TCP socket
        // Delete the socket automatically once a disconnected signal is received
        QTcpSocket *socket = new QTcpSocket(this);
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);

        if (!socket->setSocketDescriptor(socketDescriptor))
        {
            if (config.verbosity >= HttpServerConfig::Verbose::Critical)
                qCritical() << QString("Invalid socket descriptor given (%1)").arg(socket->errorString());

            return;
        }

        if (config.verbosity >= HttpServerConfig::Verbose::Warning)
        {
            qWarning() << QString("Maximum connections reached (%1). Rejecting connection from %2")
                .arg(config.maxConnections).arg(socket->peerAddress().toString());
        }

        HttpResponse *response = new HttpResponse(&config);
        response->setError(HttpStatus::ServiceUnavailable, "Too many connections", true);
        response->prepareToSend();

        // Assume that the entire request will be written in one go, relatively safe assumption
        response->writeChunk(socket);
        delete response;

        // This will disconnect after all bytes have been written
        socket->disconnectFromHost();
        return;
    }

    HttpConnection *connection = new HttpConnection(&config, requestHandler, socketDescriptor, sslConfig);
    connect(connection, &HttpConnection::disconnected, this, &HttpServer::connectionDisconnected);
    connections.push_back(connection);
}

void HttpServer::connectionDisconnected()
{
    HttpConnection *connection = dynamic_cast<HttpConnection *>(sender());
    if (!connection)
        return;

    // Remove connection from connections list
    auto it = std::find(connections.begin(), connections.end(), connection);
    if (it != connections.end())
        connections.erase(it);

    // We do delete later here because if this signal was emitted while socket is disconnecting, it still needs the
    // socket reference for a bit
    connection->deleteLater();
}

HttpServer::~HttpServer()
{
    for (HttpConnection *connection : connections)
        delete connection;

    delete sslConfig;
    close();
}
