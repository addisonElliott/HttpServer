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
    HttpRequestRouter router_;
    QString root_dir_;

public:
    RequestHandler(const QString& root_dir);

    HttpPromise handle(HttpDataPtr data) override;
    HttpPromise handleFile(HttpDataPtr data);
};

#endif // REQUESTHANDLER_H
