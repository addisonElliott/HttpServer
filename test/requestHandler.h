#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <QTimer>

#include "httpServer/httpRequestHandler.h"
#include "httpServer/httpRequestRouter.h"


class RequestHandler : public HttpRequestHandler
{
private:
    HttpRequestRouter router;

public:
    RequestHandler();

    void handle(HttpRequest *request, HttpResponse *response);

    void handleGetUsername(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
    void handleGzipTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
    void handleFormTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
    void handleFileTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
    void handleErrorTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
    void handleAsyncTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
};

#endif // REQUESTHANDLER_H
