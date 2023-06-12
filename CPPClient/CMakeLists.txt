cmake_minimum_required(VERSION 3.16.0)
set(ProjName carclient)

project(${ProjName} VERSION 1.0.0 LANGUAGES CXX)

# C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/target)
message("${CMAKE_BINARY_DIR}")

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

# https support
add_compile_definitions(SIO_TLS)

# sioclient
find_library(SOCKET_IO_LIB
NAMES sioclient
HINTS "C:/Program\ Files\ (x86)/sioclient/lib")
message(STATUS "sioclient LIB: [${SOCKET_IO_LIB}]")
find_library(SOCKET_IO_TLS_LIB
NAMES sioclient_tls
HINTS "C:/Program\ Files\ (x86)/sioclient/lib")
message(STATUS "sioclient_tls LIB: [${SOCKET_IO_TLS_LIB}]")

# raylib
find_package(raylib QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
        FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 4.5.0
    )
    FetchContent_GetProperties(raylib)
    if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
        set(FETCHCONTENT_QUIET NO)
        FetchContent_Populate(raylib)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
        add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
    endif()
endif()

set(RAYLIB_CPP_IS_MAIN FALSE)

option(BUILD_RAYLIB_CPP_EXAMPLES "Examples" ${RAYLIB_CPP_IS_MAIN})

# Include Directory
add_subdirectory(include)

# include files
set(includes

)

# source files
set(source
main.cpp
)

# Executable
add_executable(${ProjName} ${includes} ${source})
target_link_libraries(${ProjName} PUBLIC raylib_cpp raylib ${SOCKET_IO_LIB} ${SOCKET_IO_TLS_LIB})
target_include_directories(${ProjName} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/lib/catch/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/socket.io-client-cpp/src"
)