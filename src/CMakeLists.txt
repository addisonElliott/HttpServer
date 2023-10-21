project(HTTPSERVER VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

add_library(httpserver
  httpServer/httpConnection.cpp 
  httpServer/httpData.cpp 
  httpServer/httpRequest.cpp 
  httpServer/httpRequestRouter.cpp 
  httpServer/httpResponse.cpp 
  httpServer/httpServer.cpp 
  httpServer/httpRequestHandler.h
  httpServer/middleware/CORS.cpp 
  httpServer/middleware/auth.cpp 
  httpServer/middleware/getArray.cpp 
  httpServer/middleware/getObject.cpp 
  httpServer/middleware/verifyJson.cpp 
  httpServer/util.cpp
)

target_link_libraries(httpserver PUBLIC
  Qt5::Core
  Qt5::Network
  qtpromise
  z
)

target_include_directories(httpserver PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR})
