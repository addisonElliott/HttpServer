#ifndef HTTP_SERVER_CONFIG_H
#define HTTP_SERVER_CONFIG_H

#include <QHostAddress>

#include "util.h"


struct HTTPSERVER_EXPORT HttpServerConfig
{
    enum class Verbose
    {
        None = 0,
        Critical,
        Warning,
        Info,
        Debug,
        All
    };

    QHostAddress host = QHostAddress::AnyIPv4;
    unsigned short port = 80;

    int maxConnections = 100;
    int maxPendingConnections = 100;

    int maxRequestSize = 16 * 1024;
    int maxMultipartSize = 1 * 1024 * 1024;

    // Timeout time in seconds to receive a request
    // The request timeout is applied for the first request and will usually be set higher. If a request is not
    // received by this time, an error response will be sent back.
    // The keep alive timeout is the amount of time to keep the connection alive for future requests. If this timeout
    // is met, the socket is closed and the client must open a new connection.
    // The response timeout is only started and used if a response is not returned immediately. This is the number of
    // seconds that a handler has to finish processing asynchronously before a timeout will occur
    int requestTimeout = 10;
    int keepAliveTimeout = 5;
    int responseTimeout = 10;

    QString defaultContentType = "application/octet-stream";
    QString defaultCharset = "utf-8";

    Verbose verbosity = Verbose::None;

    QString sslKeyPath = "";
	QByteArray sslKeyPassPhrase;
    QString sslCertPath = "";

    // Format errors as JSON,
    // If true, then do that, if no error message is given, then do not put anything
    //
    // If false, search for document in the document map, if available, then send that filename and set status
    // If not available, then fallback to JSON, otherwise fallback to no message at all if message isn't given

    std::map<HttpStatus, QString> errorDocumentMap;
    int errorDocumentCacheTime = 60 * 60 * 24; // 1 day cache time

    HttpServerConfig() {}
};

#endif // HTTP_SERVER_CONFIG_H
