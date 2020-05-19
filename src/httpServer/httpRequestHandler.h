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

    virtual HttpPromise handle(HttpDataPtr data) = 0;
};

#endif // HTTP_SERVER_HTTP_REQUEST_HANDLER_H
