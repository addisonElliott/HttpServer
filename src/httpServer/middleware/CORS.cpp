#include "../middleware.h"

namespace middleware
{

HttpPromise CORS(std::shared_ptr<HttpData> data)
{
    response->setHeader("Access-Control-Allow-Origin", request->headerDefault("Origin", "*"));
    response->setHeader("Access-Control-Allow-Credentials", "true");

    if (request->method() == "OPTIONS")
    {
        // Pre-flight request, send additional headers
        response->setHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS, PUT, DELETE");
        response->setHeader("Access-Control-Max-Age", "3600");
        response->setHeader("Access-Control-Allow-Headers", "Content-Type, Accept, X-Requested-With");
        response->setStatus(HttpStatus::Ok);
    }

    return HttpPromise::resolve(data);
}

}
