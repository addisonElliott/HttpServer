#include "../middleware.h"

namespace middleware
{

HttpPromise getObject(HttpDataPtr data)
{
    QJsonDocument jsonDocument = data->request->parseJsonBody();
    if (jsonDocument.isNull())
        throw HttpException(HttpStatus::BadRequest, "Invalid JSON");

    QJsonObject requestJson = jsonDocument.object();
    data->state["requestObject"] = requestJson;
    return HttpPromise::resolve(data);
}

}
