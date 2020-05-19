#ifndef HTTP_SERVER_HTTP_CONNECTION_H
#define HTTP_SERVER_HTTP_CONNECTION_H

#include "httpData.h"
#include "httpServerConfig.h"
#include "httpRequest.h"
#include "httpRequestHandler.h"
#include "httpResponse.h"
#include "util.h"

#include <exception>
#include <list>
#include <memory>
#include <QTcpSocket>
#include <QThread>
#include <QSslConfiguration>
#include <QTimer>
#include <QtPromise>
#include <queue>
#include <unordered_map>



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
    // Store data for each request to enable asynchronous logic
    std::unordered_map<HttpResponse *, HttpDataPtr> data;

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

#endif // HTTP_SERVER_HTTP_CONNECTION_H
