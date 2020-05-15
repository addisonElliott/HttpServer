#include "httpRequest.h"

HttpRequest::HttpRequest(HttpServerConfig *config) : config(config), buffer(), requestBytesSize(0),
    state_(State::ReadRequestLine), method_(), uri_(), version_(), expectedBodySize(0), body_(), mimeType_(),
    charset_(), boundary(), tmpFormData(nullptr)
{
}

bool HttpRequest::parseRequest(QTcpSocket *socket, HttpResponse *response)
{
    while (true)
    {
        switch (state_)
        {
            case State::ReadRequestLine:
                if (!parseRequestLine(socket, response))
                    return false;

                break;

            case State::ReadHeader:
                if (!parseHeader(socket, response))
                    return false;

                break;

            case State::ReadBody:
                if (!parseBody(socket, response))
                    return false;

                break;

            case State::ReadMultiFormBodyData:
            case State::ReadMultiFormBodyHeaders:
                if (!parseMultiFormBody(socket, response))
                    return false;

                break;

            case State::Complete:
                return true;

            case State::Abort:
                // This state should only be reached when a fatal error occurs where the server cannot parse the
                // request successfully. Any minor errors or issues where the request can still be parsed successfully
                // will go to the complete state still
                //
                // It is unclear how much of the data in the socket buffer is meant for this request (HTTP/1.1 supports
                // pipelining so multiple requests could be present). We take a gamble by just clearing all buffers
                socket->readAll();
                buffer.clear();
                return true;
        }
    }
}

bool HttpRequest::parseRequestLine(QTcpSocket *socket, HttpResponse *response)
{
    // Return false if no more data is available
    QByteArray chunk = socket->readLine(4096);
    if (chunk.isEmpty())
        return false;

    requestBytesSize += chunk.size();
    if (requestBytesSize > config->maxRequestSize)
    {
        if (config->verbosity >= HttpServerConfig::Verbose::Info)
        {
            qInfo().noquote() << QString("Maximum size for request was reached for %1 (%2)")
                .arg(socket->peerAddress().toString()).arg(config->maxRequestSize);
        }

        response->setError(HttpStatus::RequestHeaderFieldsTooLarge, QString("The request line was too large to parse "
            "(max size: %1").arg(config->maxRequestSize));
        state_ = State::Abort;
        return true;
    }

    // Line does not end with newline, we didn't actually read the entire line (line is greater than 4096 bytes, keep reading)
    buffer += chunk;
    if (!buffer.endsWith('\n'))
        return true;

    // Remove whitespace from either ends of buffer
    buffer = buffer.trimmed();

    // RFC2616 section 4.1 states that servers SHOULD ignore all empty lines since some buggy clients send extra
    // lines after POST requests
    if (buffer.isEmpty())
        return true;

    auto parts = buffer.split(' ');
    buffer.clear();

    // RFC7230 section 2.6 states that version must start with HTTP, case-sensitive
    if (parts.length() != 3 || !parts[2].startsWith("HTTP"))
    {
        if (config->verbosity >= HttpServerConfig::Verbose::Info)
        {
            qInfo().noquote() << QString("Invalid HTTP request line received from %1: %2").arg(socket->peerAddress().toString())
                .arg(QString(parts.join(' ')));
        }

        response->setError(HttpStatus::BadRequest, "Invalid HTTP request, invalid request line");
        state_ = State::Abort;
        return true;
    }

    // RFC7230 section 3 states that a subset of ASCII character set must be used
    // The obsolete RFC2616 required Latin1 character set but that has since been changed. The Latin1 requirement was
    // a problem for header values when specifying certain characters. The implementation used here will be to utilize
    // Latin1 when it's known that non-ASCII characters won't be used, and to read UTF-8 if non-ASCII characters are
    // possible.
    //
    // In the future, a configuration setting could be added to allow customization of the heading character set
    method_ = QString::fromLatin1(parts[0]);
    uri_ = QUrl(parts[1]);
    uriQuery_ = QUrlQuery(uri_);
    version_ = QString::fromLatin1(parts[2]);
    address_ = socket->peerAddress();
    state_ = State::ReadHeader;

    // Make sure the method specified is allowed
    if (std::find(allowedMethods.begin(), allowedMethods.end(), method_) == allowedMethods.end())
    {
        if (config->verbosity >= HttpServerConfig::Verbose::Info)
            qInfo().noquote() << QString("Invalid method received from %1: %2").arg(address_.toString()).arg(method_);

        response->setError(HttpStatus::MethodNotAllowed);
        return true;
    }

    if (!uri_.isValid())
    {
        if (config->verbosity >= HttpServerConfig::Verbose::Info)
            qInfo().noquote() << QString("Invalid URI received from %1: %2").arg(address_.toString()).arg(QString(parts[1]));

        response->setError(HttpStatus::BadRequest, "Invalid URI");
        return true;
    }

    // HTTP versions 0.9 and 1.0 are deprecated and unsafe, do not allow clients that do not support HTTP/1.1
    // Any HTTP versions besides these should be supported (any future versions of HTTP should be backwards compatible)
    QString versionStr = version_.mid(4);
    if (versionStr == "0.9" || versionStr == "1.0")
    {
        response->setError(HttpStatus::HttpVersionNotSupported, "HTTP version must be at least 1.1");
        return true;
    }

    return true;
}

bool HttpRequest::parseHeader(QTcpSocket *socket, HttpResponse *response)
{
    // Return false if no more data is available
    QByteArray chunk = socket->readLine(4096);
    if (chunk.isEmpty())
        return false;

    requestBytesSize += chunk.size();
    if (requestBytesSize > config->maxRequestSize)
    {
        if (config->verbosity >= HttpServerConfig::Verbose::Info)
        {
            qInfo().noquote() << QString("Maximum size for request was reached for %1 (%2)").arg(address_.toString())
                .arg(config->maxRequestSize);
        }

        response->setError(HttpStatus::RequestHeaderFieldsTooLarge, QString("The headers were too large to parse "
            "(max size: %1").arg(config->maxRequestSize));
        state_ = State::Abort;
        return true;
    }

    // Line does not end with newline, we didn't actually read the entire line (line is greater than 4096 bytes, keep reading)
    buffer += chunk;
    if (!buffer.endsWith('\n'))
        return true;

    // Remove whitespace from either ends of buffer
    buffer = buffer.trimmed();

    // Empty line signifies end of headers
    if (buffer.isEmpty())
    {
        // Parse expected body size
        expectedBodySize = headerDefault<int>("Content-Length", 0);

        // Parse content-type header
        parseContentType();

        // No body expected, we're done
        if (expectedBodySize == 0)
        {
            state_ = State::Complete;
            return true;
        }

        // Check if the body size is going to be larger than allowed (we have a different max for multipart data)
        if (mimeType_ == "multipart/form-data")
        {
            if (requestBytesSize + expectedBodySize > config->maxMultipartSize)
            {
                if (config->verbosity >= HttpServerConfig::Verbose::Info)
                {
                    qInfo().noquote() << QString("Maximum size for request was reached for %1 (%2)")
                        .arg(address_.toString()).arg(config->maxMultipartSize);
                }

                response->setError(HttpStatus::PayloadTooLarge, QString("The body is too large to parse (max size: %1)")
                    .arg(config->maxMultipartSize));
                state_ = State::Abort;
                return true;
            }

            // We use this to keep track of how much of the body has been read
            requestBytesSize = 0;
            state_ = State::ReadMultiFormBodyData;
        }
        else
        {
            if (requestBytesSize + expectedBodySize > config->maxRequestSize)
            {
                if (config->verbosity >= HttpServerConfig::Verbose::Info)
                {
                    qInfo().noquote() << QString("Maximum size for request was reached for %1 (%2)")
                        .arg(address_.toString()).arg(config->maxRequestSize);
                }

                response->setError(HttpStatus::PayloadTooLarge, QString("The body is too large to parse (max size: %1)")
                    .arg(config->maxRequestSize));
                state_ = State::Abort;
                return true;
            }

            state_ = State::ReadBody;
        }

        buffer.clear();
        return true;
    }

    int index = buffer.indexOf(':');
    if (index == -1)
    {
        if (config->verbosity >= HttpServerConfig::Verbose::Info)
        {
            qInfo().noquote() << QString("Invalid headers in request for %1 (%2)").arg(address_.toString())
                .arg(QString(buffer));
        }

        response->setError(HttpStatus::BadRequest, "Invalid headers in request, must contain a field name and value");
        buffer.clear();
        return true;
    }

    // In RFC7230 multi-line folding headers are deprecated, will assume headers are contained on one line
    QString field = QString::fromLatin1(buffer.left(index));
    QString value = buffer.mid(index + 1).trimmed();
    buffer.clear();

    // Save cookies in separate map
    if (field.compare("Cookie", Qt::CaseInsensitive) == 0)
    {
        // Split cookies by semicolons, get the key/value pair and add to map
        auto parts = value.split(';');
        for (QString part : parts)
        {
            auto kvPart = part.split('=');
            if (kvPart.length() != 2)
            {
                if (config->verbosity >= HttpServerConfig::Verbose::Info)
                    qInfo().noquote() << QString("Invalid cookie header for %1: %2").arg(address_.toString()).arg(value);

                continue;
            }

            QString key = kvPart[0].trimmed();
            QString value = kvPart[1];

            // This will overwrite any existing cookies
            cookies[key] = value;
        }
    }
    else
    {
        // Insert new field if it doesnt exist, otherwise append to existing content with comma separator
        // RFC7230 section 3.2.2 states that multiple headers with the same field MUST be able to be appended via comma
        auto it = headers.find(field);
        if (it == headers.end())
            headers[field] = value;
        else
            it->second += ", " + value;
    }

    return true;
}

bool HttpRequest::parseBody(QTcpSocket *socket, HttpResponse *response)
{
    // Read as much data as is available
    // Return false if no more data is available
    QByteArray chunk = socket->read(expectedBodySize - buffer.size());
    if (chunk.isEmpty())
        return false;

    requestBytesSize += chunk.size();
    buffer += chunk;

    // If the buffer size is not equal to expected body size, then we will need to return and wait for more data
    if (buffer.size() == expectedBodySize)
    {
        body_ = buffer;
        state_ = State::Complete;
        buffer.clear();

        // Decompress gzip requests
        if (expectedBodySize > 0 && headerDefault("Content-Encoding", "") == "gzip")
        {
            body_ = gzipUncompress(body_);

            if (body_.size() == 0 && config->verbosity >= HttpServerConfig::Verbose::Info)
                qInfo() << "Unable to decompress GZIP request";
        }

        // Since multipart/form-data requests are automatically buffered and parsed, we will parse URL encoded ones just
        // to be consistent
        if (mimeType_ == "application/x-www-form-urlencoded")
            parsePostFormBody();

        return true;
    }

    return true;
}

bool HttpRequest::parseMultiFormBody(QTcpSocket *socket, HttpResponse *response)
{
    // Read as much data as is available
    QByteArray chunk = socket->read(expectedBodySize - requestBytesSize);

    // Keep track of the number of bytes read in total and append the chunk to the buffer
    // Note that the requestBytesSize is defined to be ONLY the bytes read from the body
    // This is used to know when we've read the expected amount of bytes
    requestBytesSize += chunk.size();
    buffer += chunk;

    if (state_ == State::ReadMultiFormBodyData)
    {
        // Construct boundary delimiter and search for it in the buffer
        // Note that the delimiterSize is two bytes more than the delimiter because the boundary will be suffixed by either
        //      "\r\n" - Indicates end of current boundary, starting new one
        //      "--" - Indicates end of current boundary, that was the last boundary
        const QString delimiter = "--" + boundary;
        const int delimiterSize = delimiter.size() + 2;
        const int index = buffer.indexOf(delimiter.toUtf8());

        // Check if we found a match for the boundary delimiter
        if (index != -1)
        {
            // If there was existing format data, then we finish that up first
            // If there was no existing form data, then this is the beginning of the multipart data, and if the index
            // is not zero, this signals that the body did not start with a valid boundary
            if (tmpFormData)
            {
                if (tmpFormData->file)
                {
                    // If the form data is a file, write the remainder of the file and set the position back to
                    // the beginning
                    QByteArray remainingData = buffer.left(index - 2);
                    tmpFormData->file->write(remainingData);
                    tmpFormData->file->seek(0);

                    // Store to form files
                    formFiles_.emplace(tmpFormData->name, FormFile {tmpFormData->file, tmpFormData->filename});
                }
                else
                {
                    // For non-file form data, the data is stored in the entire buffer, convert to UTF-8 string and save
                    // Note: We are assuming that the charset is UTF-8, majority of cases this will be true
                    QString data = QString::fromUtf8(buffer.left(index - 1));

                    // Store to form fields
                    formFields_.emplace(tmpFormData->name, data);
                }

                // Delete the temporary form data and reset it
                delete tmpFormData;
                tmpFormData = nullptr;
            }
            else if (index != 0)
            {
                // We're at the start of the multipart data but it didn't start with a boundary
                response->setError(HttpStatus::BadRequest, "Invalid multipart form data");
                state_ = State::Abort;
                buffer.clear();
            }

            // Check two bytes following the delimiter
            // If it's CRLF, then that means there's another boundary, start reading its headers
            // Otherwise if it's a --, that means we're at the end of the data
            QString suffix = buffer.mid(index + delimiter.size(), 2);
            if (suffix == "\r\n")
            {
                state_ = State::ReadMultiFormBodyHeaders;
                buffer = buffer.mid(index + delimiterSize);
            }
            else if (suffix == "--")
            {
                state_ = State::Complete;
                buffer.clear();
            }
            else
            {
                response->setError(HttpStatus::BadRequest, "Invalid multipart form data");
                state_ = State::Abort;
                buffer.clear();
            }
        }
        else if ((!tmpFormData && buffer.size() > delimiterSize) || requestBytesSize == expectedBodySize)
        {
            // This will occur in two instances:
            //      1. Beginning of multipart data did not start with delimiter
            //      2. The end of the data did not have an end boundary delimiter
            response->setError(HttpStatus::BadRequest, "Invalid multipart form data");
            state_ = State::Abort;
            buffer.clear();
        }
        else if (tmpFormData && tmpFormData->file && buffer.size() > delimiterSize)
        {
            // Append this data to the temporary file
            // Note: Write entire buffer minus the delimiter size because the delimiter could be split into two packets
            // So this guarantees that we will never miss a delimiter
            tmpFormData->file->write(buffer.left(buffer.size() - delimiterSize));
            buffer = buffer.right(delimiterSize);
            return !chunk.isEmpty();
        }
        else
        {
            return !chunk.isEmpty();
        }
    }
    else if (state_ == State::ReadMultiFormBodyHeaders)
    {
        // Search for empty newline to indicate end of headers
        int index = buffer.indexOf("\r\n\r\n");
        if (index == -1)
            return !chunk.isEmpty();

        // Grab the header data and convert to a string (UTF-8 encoding assumed)
        QString headers = buffer.left(index);

        // Complex looking regex, but it's basically searching for the following options:
        // Content-Disposition: form-data; name="<name_here>"
        // Content-Disposition: form-data; name="<name_here>"; filename="<filename>"
        QRegularExpression regex("Content-Disposition: form-data; name=\"?([^;\"]*)\"?(?:; filename=\"?([^;\"]*)\"?)?");
        auto match = regex.match(headers);
        if (!match.hasMatch())
        {
            response->setError(HttpStatus::BadRequest, "Invalid multipart form data");
            state_ = State::Abort;
            buffer.clear();
            return true;
        }

        // Construct temporary form data
        tmpFormData = new TemporaryFormData();
        tmpFormData->name = match.captured(1);
        tmpFormData->filename = match.captured(2);
        if (!tmpFormData->filename.isEmpty())
        {
            tmpFormData->file = new QTemporaryFile();
            tmpFormData->file->open();
        }

        // Remove the headers from the buffer (+4 because of the two CRLF's)
        buffer = buffer.mid(index + 4);
        state_ = State::ReadMultiFormBodyData;
    }
    else
    {
        response->setError(HttpStatus::BadRequest, "Invalid multipart form data");
        state_ = State::Abort;
        buffer.clear();
    }

    return true;
}

void HttpRequest::parseContentType()
{
    // No content-type header, use the default content type and charset
    QString contentType;
    if (!header("Content-Type", &contentType))
    {
        mimeType_ = config->defaultContentType;
        charset_ = config->defaultCharset;
        return;
    }

    // Attempt to match syntax for multipart/form-data content type (specifies a boundary instead of a charset)
    QRegularExpression formDataRegex("^multipart/form-data;\\s*boundary=\"?(.*)\"?$");
    auto match = formDataRegex.match(contentType);
    if (match.hasMatch())
    {
        mimeType_ = "multipart/form-data";
        charset_ = config->defaultCharset;
        boundary = match.captured(1);
        return;
    }

    // Attempt to match syntax for content type with charset
    // If not valid, then set the default charset and the entire content-type string is the content-type
    QRegularExpression regex("^(.*);\\s*[cC]harset=\"?(.*)\"?$");
    auto match2 = regex.match(contentType);
    if (!match2.hasMatch())
    {
        mimeType_ = contentType;
        charset_ = config->defaultCharset;
        return;
    }

    // Matched syntax, first group is content-type, second is charset
    mimeType_ = match2.captured(1);
    charset_ = match2.captured(2);
}

void HttpRequest::parsePostFormBody()
{
    // Clear the two maps just in case there is old data
    formFields_.clear();
    formFiles_.clear();

    QString bodyStr = parseBodyStr();
    QUrlQuery query = QUrlQuery(bodyStr);

    // Add each query item to the form data fields
    for (auto item : query.queryItems(QUrl::FullyDecoded))
         formFields_.emplace(item.first, item.second);

    // Clear the body since it has been parsed
    body_.clear();
}

QString HttpRequest::parseBodyStr() const
{
    // Manually parse the most common encodings first
    // Otherwise, fallback to QTextCodec which has an extensive list of charsets that are supported
    if (charset_.compare("US-ASCII", Qt::CaseInsensitive) == 0 || charset_.compare("ISO-8859-1", Qt::CaseInsensitive) == 0)
        return QString::fromLatin1(body_);
    else if (charset_.compare("UTF-8", Qt::CaseInsensitive) == 0)
        return QString::fromUtf8(body_);
    else
    {
        QTextCodec *codec = QTextCodec::codecForName(charset_.toLatin1());
        if (!codec)
        {
            // Unknown codec, fallback to UTF-8 and log warning
            if (config->verbosity >= HttpServerConfig::Verbose::Warning)
                qWarning().noquote() << QString("Unknown charset when parsing body: %1. Falling back to UTF-8").arg(charset_);

            return QString::fromUtf8(body_);
        }
        else
            return codec->toUnicode(body_);
    }
}

QJsonDocument HttpRequest::parseJsonBody() const
{
    QString bodyStr = parseBodyStr();

    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(bodyStr.toUtf8(), &error);

    if (config->verbosity >= HttpServerConfig::Verbose::Warning && error.error != QJsonParseError::NoError)
        qWarning().noquote() << QString("Unable to parse JSON document: %1").arg(error.errorString());

    return document;
}

HttpRequest::State HttpRequest::state() const
{
    return state_;
}

QHostAddress HttpRequest::address() const
{
    return address_;
}

QString HttpRequest::method() const
{
    return method_;
}

QUrl HttpRequest::uri() const
{
    return uri_;
}

QString HttpRequest::uriStr() const
{
    return uri_.path();
}

QUrlQuery HttpRequest::uriQuery() const
{
    return uriQuery_;
}

QString HttpRequest::version() const
{
    return version_;
}

bool HttpRequest::hasParameter(QString name) const
{
    return uriQuery_.hasQueryItem(name);
}

QString HttpRequest::parameter(QString name) const
{
    return uriQuery_.queryItemValue(name);
}

bool HttpRequest::hasFragment() const
{
    return uri_.hasFragment();
}

QString HttpRequest::fragment() const
{
    return uri_.fragment();
}

QString HttpRequest::mimeType() const
{
    return mimeType_;
}

QString HttpRequest::charset() const
{
    return charset_;
}

void HttpRequest::setCharset(QString charset)
{
    charset_ = charset;
}

std::unordered_map<QString, QString> HttpRequest::formFields() const
{
    return formFields_;
}

std::unordered_map<QString, FormFile> HttpRequest::formFiles() const
{
    return formFiles_;
}

QString HttpRequest::formFile(QString key) const
{
    auto it = formFields_.find(key);
    return it != formFields_.end() ? it->second : "";
}

FormFile HttpRequest::formField(QString key) const
{
    auto it = formFiles_.find(key);
    return it != formFiles_.end() ? it->second : FormFile();
}

QByteArray HttpRequest::body() const
{
    return body_;
}

QString HttpRequest::cookie(QString name) const
{
    auto it = cookies.find(name);
    return it == cookies.end() ? "" : it->second;
}

// Template specializations for header
template <>
short HttpRequest::headerDefault(QString key, short defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    return it->second.toShort(ok);
}

template <>
unsigned short HttpRequest::headerDefault(QString key, unsigned short defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    return it->second.toUShort(ok);
}

template <>
int HttpRequest::headerDefault(QString key, int defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    return it->second.toInt(ok);
}

template <>
unsigned int HttpRequest::headerDefault(QString key, unsigned int defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    return it->second.toUInt(ok);
}

template <>
long HttpRequest::headerDefault(QString key, long defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    return it->second.toLong(ok);
}

template <>
unsigned long HttpRequest::headerDefault(QString key, unsigned long defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    return it->second.toULong(ok);
}

template <>
QString HttpRequest::headerDefault(QString key, QString defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    if (ok) *ok = true;
    return it->second;
}

QString HttpRequest::headerDefault(QString key, const char *defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    if (ok) *ok = true;
    return it->second;
}

template <>
QDateTime HttpRequest::headerDefault(QString key, QDateTime defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    if (ok) *ok = true;
    return QDateTime::fromString(it->second, Qt::RFC2822Date);
}

template <>
float HttpRequest::headerDefault(QString key, float defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    return it->second.toFloat(ok);
}

template <>
double HttpRequest::headerDefault(QString key, double defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    return it->second.toDouble(ok);
}

template <>
QUrl HttpRequest::headerDefault(QString key, QUrl defaultValue, bool *ok) const
{
    auto it = headers.find(key);
    if (it == headers.end())
    {
        if (ok) *ok = false;
        return defaultValue;
    }

    QUrl ret = QUrl(it->second);
    if (ok) *ok = ret.isValid();
    return ret;
}

template <>
bool HttpRequest::header(QString key, short *value) const
{
    auto it = headers.find(key);
    if (it == headers.end())
        return false;

    bool ok;
    *value = it->second.toShort(&ok);
    return ok;
}

template <>
bool HttpRequest::header(QString key, unsigned short *value) const
{
    auto it = headers.find(key);
    if (it == headers.end())
        return false;

    bool ok;
    *value = it->second.toUShort(&ok);
    return ok;
}

template <>
bool HttpRequest::header(QString key, int *value) const
{
    auto it = headers.find(key);
    if (it == headers.end())
        return false;

    bool ok;
    *value = it->second.toInt(&ok);
    return ok;
}

template <>
bool HttpRequest::header(QString key, unsigned int *value) const
{
    auto it = headers.find(key);
    if (it == headers.end())
        return false;

    bool ok;
    *value = it->second.toUInt(&ok);
    return ok;
}

template <>
bool HttpRequest::header(QString key, long *value) const
{
    auto it = headers.find(key);
    if (it == headers.end())
        return false;

    bool ok;
    *value = it->second.toLong(&ok);
    return ok;
}

template <>
bool HttpRequest::header(QString key, unsigned long *value) const
{
    auto it = headers.find(key);
    if (it == headers.end())
        return false;

    bool ok;
    *value = it->second.toULong(&ok);
    return ok;
}

template <>
bool HttpRequest::header(QString key, QString *value) const
{
    auto it = headers.find(key);
    if (it == headers.end())
        return false;

    *value = it->second;
    return true;
}

template <>
bool HttpRequest::header(QString key, float *value) const
{
    auto it = headers.find(key);
    if (it == headers.end())
        return false;

    bool ok;
    *value = it->second.toFloat(&ok);
    return ok;
}

template <>
bool HttpRequest::header(QString key, double *value) const
{
    auto it = headers.find(key);
    if (it == headers.end())
        return false;

    bool ok;
    *value = it->second.toDouble(&ok);
    return ok;
}

template <>
bool HttpRequest::header(QString key, QUrl *value) const
{
    auto it = headers.find(key);
    if (it == headers.end())
        return false;

    *value = QUrl(it->second);
    return value->isValid();
}

HttpRequest::~HttpRequest()
{
    // Delete each temporary file (will automatically close it)
    for (auto kv : formFiles_)
        delete kv.second.file;
    formFiles_.clear();

    // Delete temporary form data (should always be deleted while parsing, but this may occur if an error happens)
    if (tmpFormData)
    {
        delete tmpFormData;
        tmpFormData = nullptr;
    }
}
