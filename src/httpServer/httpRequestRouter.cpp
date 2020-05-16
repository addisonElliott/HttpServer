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

HttpPromise HttpRequestRouter::route(HttpData *data, bool *foundRoute)
{
    // Iterate through each route
    for (const HttpRequestRoute &route : routes)
    {
        // Check for matching method and URI match
        const bool methodMatch = std::find(route.methods.begin(), route.methods.end(), data->request->method()) != route.methods.end();
        const QRegularExpressionMatch regexMatch = route.pathRegex.match(data->request->uriStr());

        // Found one, call route handler and return
        if (methodMatch && regexMatch.hasMatch())
        {
            data->state["matches"] = regexMatch.capturedTexts();
            data->state["match"] = QVariant::fromValue(regexMatch);

            if (foundRoute) *foundRoute = true;
            return route.handler(data);
        }
    }

    // No match found, defer back to handler
    if (foundRoute) *foundRoute = false;
    return HttpPromise::resolve(data);
}
