#include "requestHandler.h"

RequestHandler::RequestHandler()
{
    router.addRoute("GET", "^/users/(\\w*)/?$", this, &RequestHandler::handleGetUsername);
    // router.addRoute({"GET", "POST"}, "^/gzipTest/?$", this, &RequestHandler::handleGzipTest);
    // router.addRoute({"GET", "POST"}, "^/formTest/?$", this, &RequestHandler::handleFormTest);
    // router.addRoute("GET", "^/fileTest/(\\d*)/?$", this, &RequestHandler::handleFileTest);
    // router.addRoute("GET", "^/errorTest/(\\d*)/?$", this, &RequestHandler::handleErrorTest);
    // router.addRoute("GET", "^/asyncTest/(\\d*)/?$", this, &RequestHandler::handleAsyncTest);
}

QPromise<void> RequestHandler::handle(HttpRequest *request, HttpResponse *response)
{
    // If this is handled another way, then do nothing
    if (router.route(request, response))
        return;

    if (request->mimeType().compare("application/json", Qt::CaseInsensitive) != 0)
        return response->setError(HttpStatus::BadRequest, "Request body content type must be application/json");

    QJsonDocument jsonDocument = request->parseJsonBody();
    if (jsonDocument.isNull())
        return response->setError(HttpStatus::BadRequest, "Invalid JSON body");

    QJsonObject object;
    object["test"] = 5;
    object["another test"] = "OK";

    response->setStatus(HttpStatus::Ok, QJsonDocument(object));
}

QPromise<void> RequestHandler::handleGetUsername(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response)
{
    QString username = match.captured(1);
    QJsonObject object;

    object["username"] = username;

    response->setStatus(HttpStatus::Ok, QJsonDocument(object));
}

// QPromise<void> RequestHandler::handleGzipTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response)
// {
//     QString output = "read 24 bytes \
//             read 24 bytes = 48 \
//             read 48 bytes = 96 \
//             read = \
//             \
//             \
//             \
//             1024 = min \
//             128 * 1024 = max \
//             \
//             compression = next power of two chunk size \
//             \
//             decompression = next power of two chunk size (data * 2) \
//             Just use that as the chunk size \
//             \
//             If only 16 bytes, then je";

//     if (request->headerDefault("Content-Encoding", "") == "gzip")
//     {
//         qInfo() << request->parseBodyStr();
//     }

//     response->setStatus(HttpStatus::Ok, output, "text/plain");
//     response->compressBody();
// }

// QPromise<void> RequestHandler::handleFormTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response)
// {
//     auto formFields = request->formFields();
//     auto formFiles = request->formFiles();

//     for (auto kv : formFields)
//     {
//         qInfo().noquote() << QString("Field %1: %2").arg(kv.first).arg(kv.second);
//     }

//     for (auto kv : formFiles)
//     {
//         QByteArray data = kv.second.file->readAll();
//         qInfo().noquote() << QString("File %1 (%2) size=%3: %4").arg(kv.first).arg(kv.second.filename).arg(kv.second.file->size()).arg(QString(data));

//         kv.second.file->copy(QString("%1/Desktop/output/%2").arg(QDir::homePath()).arg(kv.second.filename));
//     }

//     response->setStatus(HttpStatus::Ok);
// }

// QPromise<void> RequestHandler::handleFileTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response)
// {
//     int id = match.captured(1).toInt();

//     switch (id)
//     {
//         case 1:
//             response->sendFile("data/404.html", "text/html", "utf-8");
//             break;

//         case 2:
//             response->sendFile("data/404.html", "text/html", "");
//             break;

//         case 3:
//             response->sendFile("data/404.html", "text/html", "", -1, Z_DEFAULT_COMPRESSION);
//             break;

//         case 4:
//             response->sendFile("data/colorPage.png", "image/png", "", -1, Z_DEFAULT_COMPRESSION, "colorPage.png");
//             break;

//         case 5:
//             response->sendFile("data/colorPage.png", "image/png", "", -1, -2, "colorPage.png");
//             break;

//         case 6:
//             response->sendFile("data/colorPage.png", "image/png", "", -1, -2, "", 3600);
//             break;

//         case 7:
//             response->sendFile("data/colorPage.png", "image/png", "", -1, Z_DEFAULT_COMPRESSION, "", 3600);
//             break;

//         case 8:
//             response->sendFile("data/404.html", "text/html", "utf-8", 100);
//             break;

//         case 9:
//             response->sendFile("data/404.html");
//             break;

//         case 10:
//             response->sendFile("data/404.html", "", "utf-8");
//             break;

//         case 11:
//             response->sendFile("data/colorPage.png");
//             break;

//         case 12:
//             response->sendFile("data/presentation.pptx");
//             break;

//         default:
//             response->setError(HttpStatus::BadRequest);
//             return;
//     }

//     response->setStatus(HttpStatus::Ok);
// }

// QPromise<void> RequestHandler::handleErrorTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response)
// {
//     int statusCode = match.captured(1).toInt();
//     HttpStatus status = (HttpStatus)statusCode;

//     response->setError(status, "There was an error here. Details go here");
// }

// QPromise<void> RequestHandler::handleAsyncTest(const QRegularExpressionMatch &match, HttpRequest *request, HttpResponse *response)
// {
//     int delay = match.captured(1).toInt();
//     QTimer *timer = new QTimer(this);

//     connect(response, &HttpResponse::cancelled, [=]() {
//         qInfo() << "Response was cancelled, stopping timer";

//         // Deleting timer will cancel it so it won't be called
//         delete timer;
//     });

//     connect(timer, &QTimer::timeout, [=]() {
//         qInfo() << "Timeout reached";

//         delete timer;
//         response->setStatus(HttpStatus::Ok);
//     });

//     timer->start(delay * 1000);
// }
