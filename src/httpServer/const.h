#ifndef HTTP_SERVER_CONST_H
#define HTTP_SERVER_CONST_H

#include <QMetaType>
#include <QRegularExpressionMatch>
#include <QtPromise>

#include "httpData.h"


using QtPromise::QPromiseTimeoutException;
using QtPromise::QPromise;
using HttpDataPtr = std::shared_ptr<HttpData>;
using HttpPromise = QPromise<std::shared_ptr<HttpData>>;
using HttpFunc = std::function<HttpPromise(std::shared_ptr<HttpData> data)>;
using HttpResolveFunc = const QtPromise::QPromiseResolve<std::shared_ptr<HttpData>> &;
using HttpRejectFunc = const QtPromise::QPromiseReject<std::shared_ptr<HttpData>> &;

Q_DECLARE_METATYPE(QRegularExpressionMatch);

#endif // HTTP_SERVER_CONST_H
