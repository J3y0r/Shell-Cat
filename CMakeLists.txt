cmake_minimum_required(VERSION 3.14)
project(shell_cat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build test programs" ON)

add_library(${PROJECT_NAME}
    src/renderer.cpp
    src/cat.cpp
    src/profile.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_executable(${PROJECT_NAME}_cli src/main.cpp)
target_link_libraries(${PROJECT_NAME}_cli PRIVATE ${PROJECT_NAME})

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
