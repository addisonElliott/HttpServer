#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <QtPromise>
#include <QTimer>

#include "httpServer/httpData.h"
#include "httpServer/httpRequestHandler.h"
#include "httpServer/httpRequestRouter.h"


using QtPromise::QPromise;

class RequestHandler : public HttpRequestHandler
{
private:
    HttpRequestRouter router;

public:
    RequestHandler();

    HttpPromise handle(HttpDataPtr data);

    HttpPromise handleGetUsername(HttpDataPtr data);
    HttpPromise handleGzipTest(HttpDataPtr data);
    HttpPromise handleFormTest(HttpDataPtr data);
    HttpPromise handleFileTest(HttpDataPtr data);
    HttpPromise handleErrorTest(HttpDataPtr data);
    HttpPromise handleAsyncTest(HttpDataPtr data);
};

#endif // REQUESTHANDLER_H
