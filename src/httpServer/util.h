#ifndef HTTP_SERVER_UTIL_H
#define HTTP_SERVER_UTIL_H

#include <algorithm>
#include <functional>
#include <map>
#include <QString>
#include <QHash>
#include <QtCore/qglobal.h>
#include <QtMath>
#include <zlib.h>

#if defined(HTTPSERVER_LIBRARY)
#  define HTTPSERVER_EXPORT Q_DECL_EXPORT
#else
#  define HTTPSERVER_EXPORT Q_DECL_IMPORT
#endif


enum class HttpStatus
{
    None = 0,

    // 1xx: Informational
    Continue = 100,
    SwitchingProtocols,
    Processing,

    // 2xx: Success
    Ok = 200,
    Created,
    Accepted,
    NonAuthoritativeInformation,
    NoContent,
    ResetContent,
    PartialContent,
    MultiStatus,
    AlreadyReported,
    IMUsed = 226,

    // 3xx: Redirection
    MultipleChoices = 300,
    MovedPermanently,
    Found,
    SeeOther,
    NotModified,
    UseProxy,
    // 306: not used, was proposed as "Switch Proxy" but never standardized
    TemporaryRedirect = 307,
    PermanentRedirect,

    // 4xx: Client Error
    BadRequest = 400,
    Unauthorized,
    PaymentRequired,
    Forbidden,
    NotFound,
    MethodNotAllowed,
    NotAcceptable,
    ProxyAuthenticationRequired,
    RequestTimeout,
    Conflict,
    Gone,
    LengthRequired,
    PreconditionFailed,
    PayloadTooLarge,
    UriTooLong,
    UnsupportedMediaType,
    RequestRangeNotSatisfiable,
    ExpectationFailed,
    ImATeapot,
    MisdirectedRequest = 421,
    UnprocessableEntity,
    Locked,
    FailedDependency,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests,
    RequestHeaderFieldsTooLarge = 431,
    UnavailableForLegalReasons = 451,

    // 5xx: Server Error
    InternalServerError = 500,
    NotImplemented,
    BadGateway,
    ServiceUnavailable,
    GatewayTimeout,
    HttpVersionNotSupported,
    VariantAlsoNegotiates,
    InsufficientStorage,
    LoopDetected,
    NotExtended = 510,
    NetworkAuthenticationRequired,
    NetworkConnectTimeoutError = 599,
};

/* Status Codes */

static const std::map<int, QString> httpStatusStrs {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {102, "Processing"},

    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},

    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},

    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},

    {500, "Internal Server Error"},
    {501, "Not implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}
};

struct HttpException : public std::exception
{
    const HttpStatus status;
    const QString message;

    HttpException(HttpStatus status) : status(status), message() {}
    HttpException(HttpStatus status, QString message) : status(status), message(message) {}
};

struct QStringCaseSensitiveHash
{
    size_t operator()(const QString str) const
    {
        static const unsigned int seed = (unsigned int)qGlobalQHashSeed();

        // Case-sensitive QString hash
        return qHash(str, seed);
    }
};

struct QStringCaseSensitiveEqual
{
    bool operator()(const QString str1, const QString str2) const
    {
        return str1.compare(str2, Qt::CaseSensitive) == 0;
    }
};

struct QStringCaseInsensitiveHash
{
    size_t operator()(const QString str) const
    {
        static const unsigned int seed = (unsigned int)qGlobalQHashSeed();

        // Case-insensitive QString hash
        return qHash(str.toLower(), seed);
    }
};

struct QStringCaseInSensitiveEqual
{
    bool operator()(const QString str1, const QString str2) const
    {
        return str1.compare(str2, Qt::CaseInsensitive) == 0;
    }
};

namespace std
{
    #if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    // Default hash and comparator for QString is case-sensitive
    template<> struct hash<QString>
    {
        size_t operator()(const QString str) const
        {
            static const unsigned int seed = (unsigned int)qGlobalQHashSeed();

            return qHash(str, seed);
        }
    };
    #endif
    template<> struct equal_to<QString>
    {
        bool operator()(const QString str1, const QString str2) const
        {
            return str1.compare(str2, Qt::CaseSensitive) == 0;
        }
    };
}

HTTPSERVER_EXPORT QString getHttpStatusStr(HttpStatus status);

QByteArray gzipCompress(QByteArray &data, int compressionLevel = Z_DEFAULT_COMPRESSION);
QByteArray gzipUncompress(QByteArray &data);

#endif // HTTP_SERVER_UTIL_H
