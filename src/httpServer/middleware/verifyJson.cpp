#include "../middleware.h"

namespace middleware
{

HttpPromise verifyJson(HttpDataPtr data)
{
    if (data->request->mimeType().compare("application/json", Qt::CaseInsensitive) != 0)
        throw HttpException(HttpStatus::BadRequest, "Request body content type must be application/json");

    return HttpPromise::resolve(data);
}

}
