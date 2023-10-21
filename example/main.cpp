//======================================================================
//  An example file retrieval http server
//
// 2023-10-21 Sat
// Dov Grobgeld <dov.grobgeld@gmail.com>
//----------------------------------------------------------------------

#include <QCoreApplication>

#include "httpServer/httpServer.h"
#include "requestHandler.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    HttpServerConfig config;
    config.port = 44388;
    config.requestTimeout = 20;
    config.responseTimeout = 5;
    config.verbosity = HttpServerConfig::Verbose::All;

    RequestHandler *handler = new RequestHandler(ROOTDIR); 
    HttpServer *server = new HttpServer(config, handler);
    server->listen();

    return a.exec();
}
