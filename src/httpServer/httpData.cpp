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

bool HttpData::test2()
{
    return true;
}

HttpData::~HttpData()
{
    delete request;
    delete response;
}
