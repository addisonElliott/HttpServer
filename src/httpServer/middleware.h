#ifndef HTTP_SERVER_MIDDLEWARE_H
#define HTTP_SERVER_MIDDLEWARE_H

#include "const.h"
#include "httpData.h"
#include "httpRequest.h"
#include "httpResponse.h"

#include <memory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>

// Synchronous vs Asynchronous Middleware
// ----------------------------------------------------------------------------------------------------
// Sync middleware can be used exactly as async middleware, e.g. with .then(middleware) clauses
// but it also has the added benefit of being able to just call the function in a .then clause without
// having to return it as the next promise.
//
// In other words, it can take this:
//          promise.then(middleware1).then(middleware2).then(middleware3)
// and turn it into:
//          promise.then(middleware1).then(middleware2).then(middleware3)
//          promise.then([](HttpDataPtr data) {
//              middleware1(data);
//              middleware2(data);
//              middleware3(data);
//          });
//
// Middleware returning HttpPromise vs HttpFunc
// ----------------------------------------------------------------------------------------------------
// Option 1:
//      HttpPromise middleware(HttpDataPtr data);
//
//      Used whenever all state information is stored in data->state
//
// Option 2:
//      HttpFunc middleware(...args);
//
//      Used whenever arguments besides ones saved in data->state are necessary for the middleware
namespace middleware
{
    // Synchronous Middleware
    HttpPromise CORS(HttpDataPtr data);
    HttpPromise verifyJson(HttpDataPtr data);
    HttpPromise getArray(HttpDataPtr data);
    HttpPromise getObject(HttpDataPtr data);
    HttpPromise checkAuthBasic(HttpDataPtr data, QString validUsername, QString validPassword);

    // Asynchronous Middleware
}

#endif // HTTP_SERVER_MIDDLEWARE_H
