#include "../middleware.h"

namespace middleware
{

HttpPromise verifyJson(std::shared_ptr<HttpData> data)
{
    if (data->request->mimeType().compare("application/json", Qt::CaseInsensitive) != 0)
        throw HttpException(HttpStatus::BadRequest, "Request body content type must be application/json")

    return HttpPromise::resolve(data);
}

}
