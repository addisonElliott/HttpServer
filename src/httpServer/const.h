#ifndef HTTP_SERVER_CONST_H
#define HTTP_SERVER_CONST_H

#include <QMetaType>
#include <QRegularExpressionMatch>
#include <QtPromise>

#include "httpData.h"


using QtPromise::QPromiseTimeoutException;
using QtPromise::QPromise;
using HttpPromise = QPromise<std::shared_ptr<HttpData>>;
using HttpFunc = std::function<HttpPromise(std::shared_ptr<HttpData> data)>;
using HttpResolveFunc = QtPromise::QPromiseResolve<std::shared_ptr<HttpData>>;
using HttpRejectFunc = QtPromise::QPromiseReject<std::shared_ptr<HttpData>>;

Q_DECLARE_METATYPE(QRegularExpressionMatch);

#endif // HTTP_SERVER_CONST_H
