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

    HttpPromise handle(std::shared_ptr<HttpData> data);

    HttpPromise handleGetUsername(std::shared_ptr<HttpData> data);
    HttpPromise handleGzipTest(std::shared_ptr<HttpData> data);
    HttpPromise handleFormTest(std::shared_ptr<HttpData> data);
    HttpPromise handleFileTest(std::shared_ptr<HttpData> data);
    HttpPromise handleErrorTest(std::shared_ptr<HttpData> data);
    HttpPromise handleAsyncTest(std::shared_ptr<HttpData> data);
};

#endif // REQUESTHANDLER_H
