#include "httpData.h"
#include "httpRequest.h"
#include "httpResponse.h"

static int test = 0;

HttpData::HttpData(HttpRequest *request, HttpResponse *response) : request(request), response(response), state(), finished(false)
{
    testValue = test++;
    printf("Created HttpData (%i)\n", testValue);
}

void HttpData::checkFinished()
{
    if (finished)
        throw HttpException(HttpStatus::None);
}

HttpData::~HttpData()
{
    printf("HttpData deleted %s (%i)\n", qUtf8Printable(request->address().toString()), testValue);
    delete request;
    delete response;
}
