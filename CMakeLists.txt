cmake_minimum_required(VERSION 3.12)
project(WaermeBildStuff)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 17)

add_executable(WaermeBildStuff main.cpp)
target_link_libraries(WaermeBildStuff ${OpenCV_LIBS})