#ifndef HTTP_SERVER_HTTP_REQUEST_HANDLER_H
#define HTTP_SERVER_HTTP_REQUEST_HANDLER_H

#include <QtPromise>

#include "const.h"
#include "httpData.h"
#include "httpRequest.h"
#include "httpResponse.h"


class HTTPSERVER_EXPORT HttpRequestHandler : public QObject
{
    Q_OBJECT

public:
    HttpRequestHandler(QObject *parent = nullptr) : QObject(parent) {}



    // Synchronous Middleware
    HttpPromise handleCORS(std::shared_ptr<HttpData> data);
    HttpPromise handleVerifyJson(std::shared_ptr<HttpData> data);
    HttpPromise handleGetArray(std::shared_ptr<HttpData> data);
    HttpPromise handleGetObject(std::shared_ptr<HttpData> data);
    HttpFunc handleBasicAuth(QString validUsername, QString validPassword);

    // Asynchronous Middleware

    virtual HttpPromise handle(std::shared_ptr<HttpData> data) = 0;
};

#endif // HTTP_SERVER_HTTP_REQUEST_HANDLER_H
