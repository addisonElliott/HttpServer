#include <QCoreApplication>

#include "httpServer/httpServer.h"
#include "requestHandler.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    HttpServerConfig config;
    config.port = 44387;
    config.requestTimeout = 20;
    config.verbosity = HttpServerConfig::Verbose::All;
    config.maxMultipartSize = 512 * 1024 * 1024;
    config.errorDocumentMap[HttpStatus::NotFound] = "data/404_2.html";
    config.errorDocumentMap[HttpStatus::InternalServerError] = "data/404_2.html";
    config.errorDocumentMap[HttpStatus::BadGateway] = "data/404_2.html";

    RequestHandler *handler = new RequestHandler();
    HttpServer *server = new HttpServer(config, handler);
    server->listen();

    return a.exec();
}
