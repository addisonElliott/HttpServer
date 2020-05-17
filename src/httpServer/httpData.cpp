#include "httpData.h"
#include "httpRequest.h"
#include "httpResponse.h"

HttpData::HttpData(HttpRequest *request, HttpResponse *response) : request(request), response(response), state(), finished(false)
{
}

HttpData::~HttpData()
{
    delete request;
    delete response;
}
