cmake_minimum_required(VERSION 3.12)

project(tiff_test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-fpermissive -g3 -O0")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_target(exec
    COMMAND ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}
    DEPENDS ${CMAKE_PROJECT_NAME}
    )

add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
    )

target_include_directories(${CMAKE_PROJECT_NAME}
    PRIVATE inc
    )

