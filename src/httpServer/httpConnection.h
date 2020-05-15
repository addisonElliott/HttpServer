#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include "httpServerConfig.h"
#include "httpRequest.h"
#include "httpRequestHandler.h"
#include "httpResponse.h"
#include "util.h"

#include <exception>
#include <list>
#include <QTcpSocket>
#include <QThread>
#include <QSslConfiguration>
#include <QTimer>
#include <queue>


class HTTPSERVER_EXPORT HttpConnection : public QObject
{
	Q_OBJECT

private:
    HttpServerConfig *config;
    QTcpSocket *socket;
    QHostAddress address;
    QTimer *timeoutTimer;
    bool keepAliveMode;

    HttpRequest *currentRequest;
    HttpResponse *currentResponse;

    HttpRequestHandler *requestHandler;
    // Responses are stored in a queue to support HTTP pipelining and sending multiple responses
    std::queue<HttpResponse *> pendingResponses;
    // Requests are stored and are deleted once the response is sent, this enables asynchronous logic
    std::unordered_map<HttpResponse *, HttpRequest *> requests;

    const QSslConfiguration *sslConfig;

    void createSocket(qintptr socketDescriptor);

public:
    HttpConnection(HttpServerConfig *config, HttpRequestHandler *requestHandler, qintptr socketDescriptor,
        QSslConfiguration *sslConfig = nullptr, QObject *parent = nullptr);
    ~HttpConnection();

private slots:
    void read();
    void bytesWritten(qint64 bytes);
    void timeout();
    void socketDisconnected();
    void sslErrors(const QList<QSslError> &errors);

signals:
    void disconnected();
};

#endif // HTTP_CONNECTION_H
