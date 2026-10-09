cmake_minimum_required(VERSION 3.20)
project(OS_Lab_2 C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SRC ${CMAKE_SOURCE_DIR}/src/main)

add_library(common STATIC ${SRC}/common/common.c)
target_include_directories(common PUBLIC ${SRC})

add_executable(parent ${SRC}/parent.c)
add_executable(child  ${SRC}/child.c)

target_link_libraries(parent PRIVATE common)
target_link_libraries(child  PRIVATE common)

# parent launches child.exe from its own directory, so build child first
add_dependencies(parent child)

if (MSVC)
    target_compile_options(common PRIVATE /W4 /utf-8)
    target_compile_options(parent PRIVATE /W4 /utf-8)
    target_compile_options(child  PRIVATE /W4 /utf-8)
else()
    target_compile_options(common PRIVATE -Wall -Wextra)
    target_compile_options(parent PRIVATE -Wall -Wextra)
    target_compile_options(child  PRIVATE -Wall -Wextra)
endif()