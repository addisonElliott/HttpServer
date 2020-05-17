#include "../middleware.h"

namespace middleware
{

HttpPromise getArray(std::shared_ptr<HttpData> data)
{
    QJsonDocument jsonDocument = data->request->parseJsonBody();
    if (jsonDocument.isNull())
        throw HttpException(HttpStatus::BadRequest, "Invalid JSON");

    QJsonArray requestJson = jsonDocument.array();
    data->state["requestArray"] = requestJson;
    return HttpPromise::resolve(data);
}

}
