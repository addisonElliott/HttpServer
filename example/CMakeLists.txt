project(HTTPSERVER VERSION 1.0.0 LANGUAGES CXX)

add_executable(example
  main.cpp 
  requestHandler.cpp
)

target_compile_definitions(example PRIVATE
  "-DROOTDIR=\"${CMAKE_CURRENT_LIST_DIR}/root\"")

target_link_libraries(example PUBLIC
  httpserver
  fmt
)

