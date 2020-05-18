#include "../middleware.h"

namespace middleware
{

HttpPromise CORS(HttpDataPtr data)
{
    data->response->setHeader("Access-Control-Allow-Origin", data->request->headerDefault("Origin", "*"));
    data->response->setHeader("Access-Control-Allow-Credentials", "true");

    if (data->request->method() == "OPTIONS")
    {
        // Pre-flight request, send additional headers
        data->response->setHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS, PUT, DELETE");
        data->response->setHeader("Access-Control-Max-Age", "3600");
        data->response->setHeader("Access-Control-Allow-Headers", "Content-Type, Accept, X-Requested-With");
        data->response->setStatus(HttpStatus::Ok);
    }

    return HttpPromise::resolve(data);
}

}
