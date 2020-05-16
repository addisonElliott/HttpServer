#ifndef HTTPDATA_H
#define HTTPDATA_H

#include <unordered_map>
#include <QString>
#include <QVariant>


// Forward declarations
class HttpRequest;
class HttpResponse;

struct HttpData
{
    HttpRequest *request;
    HttpResponse *response;
    std::unordered_map<QString, QVariant> state;

    HttpData(HttpRequest *request, HttpResponse *response) : request(request), response(response), state() {}
};

#endif // HTTPDATA_H
