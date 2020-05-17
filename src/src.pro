#-------------------------------------------------
#
# Project created by QtCreator 2019-07-31T12:47:36
#
#-------------------------------------------------

QT       -= gui
QT       += network

TARGET = httpServer
TEMPLATE = lib

DEFINES += HTTPSERVER_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        httpServer/httpConnection.cpp \
        httpServer/httpData.cpp \
        httpServer/httpRequest.cpp \
        httpServer/httpRequestRouter.cpp \
        httpServer/httpResponse.cpp \
        httpServer/httpServer.cpp \
        httpServer/middleware/CORS.cpp \
        httpServer/middleware/auth.cpp \
        httpServer/middleware/getArray.cpp \
        httpServer/middleware/getObject.cpp \
        httpServer/middleware/verifyJson.cpp \
        httpServer/util.cpp

HEADERS += \
        httpServer/const.h \
        httpServer/httpConnection.h \
        httpServer/httpCookie.h \
        httpServer/httpData.h \
        httpServer/httpRequest.h \
        httpServer/httpRequestHandler.h \
        httpServer/httpRequestRouter.h \
        httpServer/httpResponse.h \
        httpServer/httpServer.h \
        httpServer/httpServerConfig.h \
        httpServer/middleware.h \
        httpServer/util.h

include(../common.pri)

win32: LIBS += -lzlib
unix: LIBS += -lz

unix {
    QMAKE_STRIP =

    headers.path = /usr/local/include/httpServer
    headers.files = $$HEADERS
    target.path = /usr/local/lib
    strip.path = /usr/local/lib
    strip.commands = strip --strip-unneeded /usr/local/lib/$(TARGET)
    strip.depends = install_headers install_target
    INSTALLS += headers target strip

    CONFIG(debug, debug|release) {
        mkpath($$PWD/build/debug)

        DESTDIR = $$PWD/build/debug
        OBJECTS_DIR = $$PWD/build/debug
    }

    CONFIG(release, debug|release) {
        mkpath($$PWD/build/release)

        DESTDIR = $$PWD/build/release
        OBJECTS_DIR = $$PWD/build/release
    }
}
