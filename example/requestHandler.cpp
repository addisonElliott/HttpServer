//======================================================================
//  A file retrieval request handler.
//
// 2023-10-21 Sat
// Dov Grobgeld <dov.grobgeld@gmail.com>
//----------------------------------------------------------------------

#include "requestHandler.h"

RequestHandler::RequestHandler(const QString& root_dir)
    : root_dir_(root_dir)
{
    router_.addRoute("GET", "^/(.*)$", this, &RequestHandler::handleFile);
}

HttpPromise RequestHandler::handle(HttpDataPtr data)
{
    bool foundRoute;
    HttpPromise promise = router_.route(data, &foundRoute);
    if (foundRoute)
        return promise;

    // We should never be here 
    data->response->setStatus(HttpStatus::BadRequest);
    return HttpPromise::resolve(data);
}

HttpPromise RequestHandler::handleFile(HttpDataPtr data)
{
    auto match = data->state["match"].value<QRegularExpressionMatch>();
    QString filename = match.captured(1);

    // Add index.html if it wasn't specified
    if (filename == "" || filename.endsWith("/"))
      filename += "index.html";
    QString filepath = root_dir_ + "/" + filename;

    if (QFileInfo::exists(filepath)) {
        // Get the mime type
        QMimeDatabase db;
        QString mimetype = db.mimeTypeForFile(filepath).name();

        // Override for html as it sometimes gets application/x-extension-html
        if (filepath.endsWith(".html"))
          mimetype = "text/html";

        data->response->sendFile(filepath, mimetype, "utf-8");
        data->response->setStatus(HttpStatus::Ok);
    }
    else {
        data->response->setStatus(
          HttpStatus::NotFound,
          QByteArray(("File not found: "+filepath+"\n"
                      + "match.captured(1)=" + match.captured(1)
                      ).toUtf8()),
          "text/plain");
    }

    return HttpPromise::resolve(data);
}
