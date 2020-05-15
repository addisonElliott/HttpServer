#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <QtPromise>
#include <QTimer>

#include "httpServer/httpRequestHandler.h"
#include "httpServer/httpRequestRouter.h"


using QtPromise::QPromise;

class RequestHandler : public HttpRequestHandler
{
private:
    HttpRequestRouter router;

public:
    RequestHandler();

    QPromise<void> handle(HttpRequest *request, HttpResponse *response);

    // QPromise<void> handleGetUsername(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
    // QPromise<void> handleGzipTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
    // QPromise<void> handleFormTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
    // QPromise<void> handleFileTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
    // QPromise<void> handleErrorTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
    // QPromise<void> handleAsyncTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response);
};

#endif // REQUESTHANDLER_H
