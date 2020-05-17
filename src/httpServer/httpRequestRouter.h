#ifndef HTTP_SERVER_HTTP_REQUEST_ROUTER_H
#define HTTP_SERVER_HTTP_REQUEST_ROUTER_H

#include <functional>
#include <list>
#include <QtPromise>
#include <vector>

#include "const.h"
#include "httpRequest.h"
#include "httpResponse.h"
#include "util.h"


typedef std::function<HttpPromise(std::shared_ptr<HttpData> data)> HttpRequestMapFunction;

struct HttpRequestRoute
{
    std::vector<QString> methods;
    QRegularExpression pathRegex;

    HttpRequestMapFunction handler;
};

class HTTPSERVER_EXPORT HttpRequestRouter
{
private:
    std::list<HttpRequestRoute> routes;

public:
    void addRoute(QString method, QString regex, HttpRequestMapFunction handler);
    void addRoute(std::vector<QString> methods, QString regex, HttpRequestMapFunction handler);

    // Allows registering member functions using addRoute(..., <CLASS>, &Class:memberFunction)
    template <typename T>
    void addRoute(QString method, QString regex, T *inst,
        HttpPromise (T::*handler)(std::shared_ptr<HttpData> data))
    {
        return addRoute(method, regex, std::bind(handler, inst, std::placeholders::_1));
    }

    template <typename T>
    void addRoute(QString method, QString regex, T *inst,
        HttpPromise (T::*handler)(std::shared_ptr<HttpData> data) const)
    {
        return addRoute(method, regex, std::bind(handler, inst, std::placeholders::_1));
    }

    template <typename T>
    void addRoute(std::vector<QString> methods, QString regex, T *inst,
        HttpPromise (T::*handler)(std::shared_ptr<HttpData> data))
    {
        return addRoute(methods, regex, std::bind(handler, inst, std::placeholders::_1));
    }

    template <typename T>
    void addRoute(std::vector<QString> methods, QString regex, T *inst,
        HttpPromise (T::*handler)(std::shared_ptr<HttpData> data) const)
    {
        return addRoute(methods, regex, std::bind(handler, inst, std::placeholders::_1));
    }

    HttpPromise route(std::shared_ptr<HttpData> data, bool *foundRoute = nullptr);
};

#endif // HTTP_SERVER_HTTP_REQUEST_ROUTER_H
