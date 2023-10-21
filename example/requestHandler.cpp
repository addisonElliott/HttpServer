#include "requestHandler.h"
#include <fmt/core.h>

using fmt::print;

RequestHandler::RequestHandler(const QString& root_dir)
  : root_dir_(root_dir)
{
  router_.addRoute("GET", "^/(.*)$", this, &RequestHandler::handleFile);
}

HttpPromise RequestHandler::handle(HttpDataPtr data)
{
#if 0
//  data->response->setStatus(HttpStatus::Ok,
//                            QByteArray("<h1>Lupchi!</h1>stupon"),
//                            "text/html");
  data->response->sendFile("/tmp/hello.html", "text/html", "utf-8");
  data->response->setStatus(HttpStatus::Ok);
  return HttpPromise::resolve(data);
#endif
    bool foundRoute;
    HttpPromise promise = router_.route(data, &foundRoute);
    if (foundRoute)
        return promise;
    // Return error
    return promise;
}

HttpPromise RequestHandler::handleFile(HttpDataPtr data)
{
    auto match = data->state["match"].value<QRegularExpressionMatch>();
    QString filename = match.captured(1);

    QString filepath = root_dir_ + "/" + filename;

    print("Requested to get {}\n", filepath.toStdString());

    if (QFileInfo::exists(filepath))
    {
      data->response->sendFile(filepath,
                               "text/html", "utf-8");
      data->response->setStatus(HttpStatus::Ok);
    }
    else {
      data->response->setStatus(HttpStatus::NotFound,
                                QByteArray("File not found!"),
                                "text/html");
    }

    return HttpPromise::resolve(data);
}
