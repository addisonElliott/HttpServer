#ifndef HTTP_SERVER_HTTP_REQUEST_H
#define HTTP_SERVER_HTTP_REQUEST_H

#include "httpCookie.h"
#include "httpResponse.h"
#include "httpServerConfig.h"
#include "util.h"

#include <algorithm>
#include <QByteArray>
#include <QDir>
#include <QJsonDocument>
#include <QList>
#include <QHostAddress>
#include <QMap>
#include <QMultiMap>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QTemporaryFile>
#include <QTextCodec>
#include <QUuid>
#include <QUrl>
#include <QUrlQuery>
#include <unordered_map>
#include <vector>


struct HTTPSERVER_EXPORT FormFile
{
    QTemporaryFile *file;
    QString filename;
};

struct TemporaryFormData
{
    QString name;
    QString filename;
    QTemporaryFile *file = nullptr;
};

class HTTPSERVER_EXPORT HttpRequest
{
    friend class HttpResponse;

public:
    enum class State
    {
        ReadRequestLine,
        ReadHeader,
        ReadBody,
        ReadMultiFormBodyData,
        ReadMultiFormBodyHeaders,
        Complete,
        Abort
    };

private:
    const std::vector<QString> allowedMethods = {"GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS"};

    HttpServerConfig *config;

    QByteArray buffer;
    int requestBytesSize;

    State state_;
    QHostAddress address_;
    QString method_;
    QUrl uri_;
    QUrlQuery uriQuery_;
    QString version_;

    std::unordered_map<QString, QString, QStringCaseInsensitiveHash, QStringCaseInSensitiveEqual> headers;
    // Note: Cookies ARE case sensitive, headers are not
    std::unordered_map<QString, QString> cookies;

    int expectedBodySize;
    QByteArray body_;

    QString mimeType_;
    QString charset_;
    QString boundary;

    TemporaryFormData *tmpFormData;

    std::unordered_map<QString, QString> formFields_;
    std::unordered_map<QString, FormFile> formFiles_;

    bool parseRequestLine(QTcpSocket *socket, HttpResponse *response);
    bool parseHeader(QTcpSocket *socket, HttpResponse *response);
    bool parseBody(QTcpSocket *socket, HttpResponse *response);
    bool parseMultiFormBody(QTcpSocket *socket, HttpResponse *response);

    void parseContentType();
    void parsePostFormBody();

public:
    HttpRequest(HttpServerConfig *config);
    ~HttpRequest();

    bool parseRequest(QTcpSocket *socket, HttpResponse *response);

    QString parseBodyStr() const;
    QJsonDocument parseJsonBody() const;

    State state() const;
    QHostAddress address() const;
    QString method() const;
    QUrl uri() const;
    QString uriStr() const;
    QUrlQuery uriQuery() const;
    QString version() const;

    bool hasParameter(QString name) const;
    QString parameter(QString name) const;
    bool hasFragment() const;
    QString fragment() const;

    template <class T>
    T headerDefault(QString key, T defaultValue, bool *ok = nullptr) const;
	
	QString headerDefault(QString key, const char *defaultValue, bool *ok = nullptr) const;

    template <class T>
    bool header(QString key, T *value) const;

    QString mimeType() const;
    QString charset() const;
    // Note: This function is useful if you want to override the given charset (or say if you know that the request doesn't contain a charset)
    void setCharset(QString charset);

    std::unordered_map<QString, QString> formFields() const;
    std::unordered_map<QString, FormFile> formFiles() const;
    QString formFile(QString key) const;
    FormFile formField(QString key) const;

    QByteArray body() const;
    QString cookie(QString name) const;
};

// Declarations for templates
template<> HTTPSERVER_EXPORT short HttpRequest::headerDefault(QString key, short defaultValue, bool *ok) const;
template<> HTTPSERVER_EXPORT unsigned short HttpRequest::headerDefault(QString key, unsigned short defaultValue, bool *ok) const;
template<> HTTPSERVER_EXPORT int HttpRequest::headerDefault(QString key, int defaultValue, bool *ok) const;
template<> HTTPSERVER_EXPORT unsigned int HttpRequest::headerDefault(QString key, unsigned int defaultValue, bool *ok) const;
template<> HTTPSERVER_EXPORT long HttpRequest::headerDefault(QString key, long defaultValue, bool *ok) const;
template<> HTTPSERVER_EXPORT unsigned long HttpRequest::headerDefault(QString key, unsigned long defaultValue, bool *ok) const;
template<> HTTPSERVER_EXPORT QString HttpRequest::headerDefault(QString key, QString defaultValue, bool *ok) const;
template<> HTTPSERVER_EXPORT QDateTime HttpRequest::headerDefault(QString key, QDateTime defaultValue, bool *ok) const;
template<> HTTPSERVER_EXPORT float HttpRequest::headerDefault(QString key, float defaultValue, bool *ok) const;
template<> HTTPSERVER_EXPORT double HttpRequest::headerDefault(QString key, double defaultValue, bool *ok) const;
template<> HTTPSERVER_EXPORT QUrl HttpRequest::headerDefault(QString key, QUrl defaultValue, bool *ok) const;

template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, short *value) const;
template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, unsigned short *value) const;
template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, int *value) const;
template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, unsigned int *value) const;
template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, long *value) const;
template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, unsigned long *value) const;
template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, QString *value) const;
template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, QDateTime *value) const;
template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, float *value) const;
template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, double *value) const;
template<> HTTPSERVER_EXPORT bool HttpRequest::header(QString key, QUrl *value) const;

#endif // HTTP_SERVER_HTTP_REQUEST_H
