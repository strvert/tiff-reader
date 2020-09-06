cmake_minimum_required(VERSION 3.12)

project(tiff_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-fpermissive -g3 -O0")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_target(exec
    COMMAND ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}
    DEPENDS ${CMAKE_PROJECT_NAME}
    )

add_custom_target(copy_compile_commands ALL
    COMMAND cp ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_SOURCE_DIR}
    DEPENDS ${CMAKE_PROJECT_NAME}
    )

add_executable(${CMAKE_PROJECT_NAME}
    src/main.cpp
    src/tiff_reader.cpp
    )

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE ./inc
    )

