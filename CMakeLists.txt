cmake_minimum_required(VERSION 3.20)
project(camila VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(Canela/include)

file(GLOB_RECURSE CAMILA_SOURCES
    "Canela/include/*.cpp"
)

file(GLOB_RECURSE CAMILA_HEADERS
    "Canela/include/*.hpp"
)

add_executable(camila
    ${CAMILA_SOURCES}
    ${CAMILA_HEADERS}
)

target_compile_definitions(camila PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if (MSVC)
    target_compile_options(camila PRIVATE /W4 /permissive- /utf-8)
endif()
