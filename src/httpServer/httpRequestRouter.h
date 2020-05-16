#ifndef HTTP_REQUEST_ROUTER_H
#define HTTP_REQUEST_ROUTER_H

#include <functional>
#include <list>
#include <QtPromise>
#include <vector>

#include "const.h"
#include "httpRequest.h"
#include "httpResponse.h"
#include "util.h"


typedef std::function<HttpPromise(const QRegularExpressionMatch &, HttpRequest *, HttpResponse *)> HttpRequestMapFunction;

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
        HttpPromise (T::*handler)(const QRegularExpressionMatch &, HttpRequest *, HttpResponse *))
    {
        return addRoute(method, regex, std::bind(handler, inst, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));
    }

    template <typename T>
    void addRoute(QString method, QString regex, T *inst,
        HttpPromise (T::*handler)(const QRegularExpressionMatch &, HttpRequest *, HttpResponse *) const)
    {
        return addRoute(method, regex, std::bind(handler, inst, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));
    }

    template <typename T>
    void addRoute(std::vector<QString> methods, QString regex, T *inst,
        HttpPromise (T::*handler)(const QRegularExpressionMatch &, HttpRequest *, HttpResponse *))
    {
        return addRoute(methods, regex, std::bind(handler, inst, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));
    }

    template <typename T>
    void addRoute(std::vector<QString> methods, QString regex, T *inst,
        HttpPromise (T::*handler)(const QRegularExpressionMatch &, HttpRequest *, HttpResponse *) const)
    {
        return addRoute(methods, regex, std::bind(handler, inst, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3));
    }

    HttpPromise route(HttpRequest *request, HttpResponse *response, bool *foundRoute = nullptr);
};

class httpRequestRouter
{
public:
    httpRequestRouter();
};

#endif // HTTP_REQUEST_ROUTER_H
