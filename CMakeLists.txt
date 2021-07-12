project(HttpServer)

set(CMAKE_CXX_COMPLIER g++)
set(CMAKE_C_COMPLIER gcc)

file(GLOB SOURCE_CPP *.cpp)
file(GLOB SOURCE_H *.h)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O0 -Wall -g -Wno-unknown-pragmas")

add_executable(HttpServer ${SOURCE_CPP})

target_link_libraries(HttpServer libevent.a)
target_link_libraries(HttpServer libevent_core.a)
target_link_libraries(HttpServer libevent_extra.a)
