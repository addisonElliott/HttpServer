project(HTTPSERVER VERSION 1.0.0 LANGUAGES CXX)

add_executable(test-http-server
  main.cpp 
  requestHandler.cpp
)

target_link_libraries(test-http-server PUBLIC
  httpserver
)

