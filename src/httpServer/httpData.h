#ifndef HTTPDATA_H
#define HTTPDATA_H

#include "util.h"

#include <unordered_map>
#include <QString>
#include <QVariant>


// Forward declarations
class HttpRequest;
class HttpResponse;

struct HTTPSERVER_EXPORT HttpData
{
    HttpRequest *request;
    HttpResponse *response;
    std::unordered_map<QString, QVariant> state;
    bool finished;

    HttpData(HttpRequest *request, HttpResponse *response);
    ~HttpData();

    void checkFinished();
};

#endif // HTTPDATA_H
