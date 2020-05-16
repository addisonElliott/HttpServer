#ifndef CONST_H
#define CONST_H

#include <QtPromise>

#include "httpData.h"


using QtPromise::QPromiseTimeoutException;
using QtPromise::QPromise;
using HttpPromise = QPromise<HttpData *>;

#endif // CONST_H
