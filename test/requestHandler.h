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

    HttpPromise handle(HttpData *data);

    HttpPromise handleGetUsername(HttpData *data);
    HttpPromise handleGzipTest(HttpData *data);
    HttpPromise handleFormTest(HttpData *data);
    HttpPromise handleFileTest(HttpData *data);
    HttpPromise handleErrorTest(HttpData *data);
    HttpPromise handleAsyncTest(HttpData *data);
};

#endif // REQUESTHANDLER_H
