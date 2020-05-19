#include "httpData.h"
#include "httpRequest.h"
#include "httpResponse.h"

HttpData::HttpData(HttpRequest *request, HttpResponse *response) : request(request), response(response), state(), finished(false)
{
}

void HttpData::checkFinished()
{
    if (finished)
        throw HttpException(HttpStatus::None);
}

HttpData::~HttpData()
{
    delete request;
    delete response;
}
