cmake_minimum_required (VERSION 2.8)

set(CMAKE_MODULE_PATH "/usr/local/lib/cmake/${CMAKE_MODULE_PATH}")

find_package(OpenCV)
find_package(Threads)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(main main.cpp)
add_library(iniParser STATIC lib/iniParser.cpp)

target_link_libraries(main Threads::Threads)
target_link_libraries(main PUBLIC iniParser)
target_link_libraries(main ${OpenCV_LIBS})
 
