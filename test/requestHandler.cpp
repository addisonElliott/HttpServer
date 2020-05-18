#include "requestHandler.h"

RequestHandler::RequestHandler()
{
    router.addRoute("GET", "^/users/(\\w*)/?$", this, &RequestHandler::handleGetUsername);
    router.addRoute({"GET", "POST"}, "^/gzipTest/?$", this, &RequestHandler::handleGzipTest);
    router.addRoute({"GET", "POST"}, "^/formTest/?$", this, &RequestHandler::handleFormTest);
    router.addRoute("GET", "^/fileTest/(\\d*)/?$", this, &RequestHandler::handleFileTest);
    router.addRoute("GET", "^/errorTest/(\\d*)/?$", this, &RequestHandler::handleErrorTest);
    router.addRoute("GET", "^/asyncTest/(\\d*)/?$", this, &RequestHandler::handleAsyncTest);
}

HttpPromise RequestHandler::handle(HttpDataPtr data)
{
    bool foundRoute;
    HttpPromise promise = router.route(data, &foundRoute);
    if (foundRoute)
        return promise;

    if (data->request->mimeType().compare("application/json", Qt::CaseInsensitive) != 0)
        throw HttpException(HttpStatus::BadRequest, "Request body content type must be application/json");

    QJsonDocument jsonDocument = data->request->parseJsonBody();
    if (jsonDocument.isNull())
        throw HttpException(HttpStatus::BadRequest, "Invalid JSON body");

    QJsonObject object;
    object["test"] = 5;
    object["another test"] = "OK";

    data->response->setStatus(HttpStatus::Ok, QJsonDocument(object));
    return HttpPromise::resolve(data);
}

HttpPromise RequestHandler::handleGetUsername(HttpDataPtr data)
{
    auto match = data->state["match"].value<QRegularExpressionMatch>();
    QString username = match.captured(1);
    QJsonObject object;

    object["username"] = username;

    data->response->setStatus(HttpStatus::Ok, QJsonDocument(object));
    return HttpPromise::resolve(data);
}

HttpPromise RequestHandler::handleGzipTest(HttpDataPtr data)
{
    QString output = "read 24 bytes \
            read 24 bytes = 48 \
            read 48 bytes = 96 \
            read = \
            \
            \
            \
            1024 = min \
            128 * 1024 = max \
            \
            compression = next power of two chunk size \
            \
            decompression = next power of two chunk size (data * 2) \
            Just use that as the chunk size \
            \
            If only 16 bytes, then je";
    if (data->request->headerDefault("Content-Encoding", "") == "gzip")
    {
        qInfo() << data->request->parseBodyStr();
    }
    data->response->setStatus(HttpStatus::Ok, output, "text/plain");
    data->response->compressBody();
    return HttpPromise::resolve(data);
}

HttpPromise RequestHandler::handleFormTest(HttpDataPtr data)
{
    auto formFields = data->request->formFields();
    auto formFiles = data->request->formFiles();
    for (auto kv : formFields)
    {
        qInfo().noquote() << QString("Field %1: %2").arg(kv.first).arg(kv.second);
    }
    for (auto kv : formFiles)
    {
        QByteArray data = kv.second.file->readAll();
        qInfo().noquote() << QString("File %1 (%2) size=%3: %4").arg(kv.first).arg(kv.second.filename).arg(kv.second.file->size()).arg(QString(data));
        kv.second.file->copy(QString("%1/Desktop/output/%2").arg(QDir::homePath()).arg(kv.second.filename));
    }
    data->response->setStatus(HttpStatus::Ok);
    return HttpPromise::resolve(data);
}

HttpPromise RequestHandler::handleFileTest(HttpDataPtr data)
{
    auto match = data->state["match"].value<QRegularExpressionMatch>();
    int id = match.captured(1).toInt();

    switch (id)
    {
        case 1:
            data->response->sendFile("data/404.html", "text/html", "utf-8");
            break;

        case 2:
            data->response->sendFile("data/404.html", "text/html", "");
            break;

        case 3:
            data->response->sendFile("data/404.html", "text/html", "", -1, Z_DEFAULT_COMPRESSION);
            break;

        case 4:
            data->response->sendFile("data/colorPage.png", "image/png", "", -1, Z_DEFAULT_COMPRESSION, "colorPage.png");
            break;

        case 5:
            data->response->sendFile("data/colorPage.png", "image/png", "", -1, -2, "colorPage.png");
            break;

        case 6:
            data->response->sendFile("data/colorPage.png", "image/png", "", -1, -2, "", 3600);
            break;

        case 7:
            data->response->sendFile("data/colorPage.png", "image/png", "", -1, Z_DEFAULT_COMPRESSION, "", 3600);
            break;

        case 8:
            data->response->sendFile("data/404.html", "text/html", "utf-8", 100);
            break;

        case 9:
            data->response->sendFile("data/404.html");
            break;

        case 10:
            data->response->sendFile("data/404.html", "", "utf-8");
            break;

        case 11:
            data->response->sendFile("data/colorPage.png");
            break;

        case 12:
            data->response->sendFile("data/presentation.pptx");
            break;

        default:
            throw new HttpException(HttpStatus::BadRequest);
    }

    data->response->setStatus(HttpStatus::Ok);
    return HttpPromise::resolve(data);
}

HttpPromise RequestHandler::handleErrorTest(HttpDataPtr data)
{
    auto match = data->state["match"].value<QRegularExpressionMatch>();
    int statusCode = match.captured(1).toInt();
    HttpStatus status = (HttpStatus)statusCode;
    data->response->setError(status, "There was an error here. Details go here");
    return HttpPromise::resolve(data);
}

HttpPromise RequestHandler::handleAsyncTest(HttpDataPtr data)
{
    auto match = data->state["match"].value<QRegularExpressionMatch>();
    int delay = match.captured(1).toInt();
    return HttpPromise::resolve(data).delay(delay * 1000).then([](HttpDataPtr data) {
        qInfo() << "Timeout reached";
        data->checkFinished();

        data->response->setStatus(HttpStatus::Ok);
        return data;
    });
}
