cmake_minimum_required(VERSION 3.22)

project(asio-based-runtime)

set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})

find_package(asio REQUIRED)

add_executable(asio-based-runtime "src/main.cpp" "src/runtime.hpp"  "src/task_queue_interface.hpp")
set_property(TARGET asio-based-runtime PROPERTY CXX_STANDARD 20)
target_link_libraries(asio-based-runtime PRIVATE asio::asio)
target_compile_definitions(asio-based-runtime
  PUBLIC ASIO_NO_DEPRECATED
  PUBLIC _WIN32_WINDOWS=0x0A00
)