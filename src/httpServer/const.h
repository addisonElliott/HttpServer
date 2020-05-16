#ifndef CONST_H
#define CONST_H

#include <QMetaType>
#include <QRegularExpressionMatch>
#include <QtPromise>

#include "httpData.h"


using QtPromise::QPromiseTimeoutException;
using QtPromise::QPromise;
using HttpPromise = QPromise<HttpData *>;

Q_DECLARE_METATYPE(QRegularExpressionMatch);

#endif // CONST_H
