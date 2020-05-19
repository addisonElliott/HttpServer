Introduction
=================
HttpServer is a C++ library that uses the Qt platform to setup a feature rich, easy-to-use HTTP server.

Two existing Qt HTTP server libraries exist out there already, but the licenses are more restrictive (GPL & LGPL), so I decided to create my own:

1. [QtWebApp](https://github.com/fffaraz/QtWebApp)
2. [qthttpserver](https://github.com/qt-labs/qthttpserver)

Features
=================
* Single-threaded with asynchronous callbacks
* HTTP/1.1
* TLS support
* Compression & decompression (GZIP-only)
* Easy URL router with regex matching
* Form parsing (multi-part and www-form-urlencoded)
* Sending files
* JSON sending or receiving support
* Custom error responses (e.g. HTML page or JSON response)

Promises Support
=================
There are two variants of this library, one with and without promise support. Promises allow for easier & cleaner development with asynchronous logic. The two variants are supported via separate branches:

1. **master** - No promise support
2. **promises** - Promise support

**Note:** The variant without promise support is considered deprecated and will only be supported via bug fixes in the future. For new development, promises are encouraged. The code will remain in two separate branches until sufficient unit testing & documentation is provided for promises support. If you would like to help, please contribute!

Installing
=================
Prerequisites
-------------
* Qt & Qt Creator for IDE
* zlib
* OpenSSL binaries for TLS support (see [here](https://doc.qt.io/qt-5/ssl.html#enabling-and-disabling-ssl-support))
* QtPromise for promise support (see [here](https://qtpromise.netlify.app/qtpromise/getting-started.html#installation) for installation instructions)

Building HttpServer
-------------------------
1. Open `HttpServer.pro` in Qt Creator.
2. Create a `common.pri` file in the top-level directory. This will store any specific include & library paths on a per-machine basis.
   * Append paths to your `zlib` build with `INCLUDEPATH` and `LIBS`
   * Append paths to your `qtpromise` directory with `INCLUDEPATH` (QtPromise is a header-only library)
      * Note: You can include the provided `qtpromise.pri` to do this for your. Alternatively, you can install the headers to a system-configured path in which case you don't need to do anything.
   * Make sure on Windows that the compiled zlib DLL is in your environment `PATH` variable
3. Build and run the application
   * Building the application will build the shared library as well as the test application. When you press run, it will run the test application in which you can experiment with the library via the provided URLs

**Note:** Since this is just a normal Qt project with a `pro` file, you can compile the project via the command-line with `qmake` and your platform-specific compiler (i.e. `make` for Linux or `nmake` for Windows).

Example
=================
See [here](https://github.com/addisonElliott/HttpServer/blob/master/test/requestHandler.cpp) for example code using HttpServer.

Roadmap & Bugs
=================
* TLS session resumption is not supported by Qt currently

Pull requests are welcome (and encouraged) for any or all issues!

License
=================
HttpServer has an MIT-based [license](https://github.com/addisonElliott/HttpServer/blob/master/LICENSE).