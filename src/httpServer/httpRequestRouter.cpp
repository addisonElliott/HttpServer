#include "httpRequestRouter.h"

void HttpRequestRouter::addRoute(QString method, QString regex, HttpRequestMapFunction handler)
{
    HttpRequestRoute route = {{method}, QRegularExpression(regex), handler};
    route.pathRegex.optimize();
    routes.push_back(route);
}

void HttpRequestRouter::addRoute(std::vector<QString> methods, QString regex, HttpRequestMapFunction handler)
{
    HttpRequestRoute route = {methods, QRegularExpression(regex), handler};
    route.pathRegex.optimize();
    routes.push_back(route);
}

bool HttpRequestRouter::route(HttpRequest *request, HttpResponse *response)
{
    // Iterate through each route
    for (const HttpRequestRoute &route : routes)
    {
        // Check for matching method and URI match
        const bool methodMatch = std::find(route.methods.begin(), route.methods.end(), request->method()) != route.methods.end();
        const QRegularExpressionMatch regexMatch = route.pathRegex.match(request->uriStr());

        // Found one, call route handler and return
        if (methodMatch && regexMatch.hasMatch())
        {
            route.handler(regexMatch, request, response);
            return true;
        }
    }

    // No match found, defer back to handler
    return false;
}
