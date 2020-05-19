#include "../middleware.h"

namespace middleware
{

HttpPromise checkAuthBasic(HttpDataPtr data, QString validUsername, QString validPassword)
{
    QString auth;
    if (data->request->header("Authorization", &auth))
    {
        if (auth.startsWith("Basic"))
        {
            QString credentials = QByteArray::fromBase64(auth.mid(6).toLatin1());
            int colonIndex = credentials.indexOf(':');
            if (colonIndex != -1)
            {
                QString username = credentials.left(colonIndex);
                QString password = credentials.mid(colonIndex + 1);

                // Verify username and password are correct
                if (username == validUsername && password == validPassword)
                {
                    data->state["authUsername"] = username;
                    data->state["authPassword"] = password;
                    return HttpPromise::resolve(data);
                }
            }
        }
    }

    throw HttpException(HttpStatus::Unauthorized, "Access denied");
}

}
