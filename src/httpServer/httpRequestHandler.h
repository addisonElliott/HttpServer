#ifndef HTTP_REQUEST_HANDLER_H
#define HTTP_REQUEST_HANDLER_H

#include "httpRequest.h"
#include "httpResponse.h"
#include "util.h"


class HTTPSERVER_EXPORT HttpRequestHandler : public QObject
{
    Q_OBJECT

public:
    HttpRequestHandler(QObject *parent = nullptr) : QObject(parent) {}

    virtual void handle(HttpRequest *request, HttpResponse *response) = 0;
};

#endif // HTTP_REQUEST_HANDLER_H
