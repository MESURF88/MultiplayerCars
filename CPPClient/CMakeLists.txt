cmake_minimum_required(VERSION 3.24.0)

set(ProjName carclient)

project(${ProjName} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DEBUGLOCAL "Connect the client to a local development server" OFF)

if(DEBUGLOCAL)
    add_compile_definitions(DEBUG_CLIENT)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring Debug client")
    set(CMAKE_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/targetdbg")
else()
    message(STATUS "Configuring Release client")
    set(CMAKE_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/target")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(RAYLIB_VERSION 6.0)
set(CPR_VERSION 1.14.0)
set(NLOHMANN_JSON_VERSION 3.12.0)
set(NLOHMANN_JSON_TAG v3.12.0)
set(SIMDJSON_VERSION 4.6.4)
set(SIMDJSON_TAG v4.6.4)

find_package(OpenSSL REQUIRED)

if(WIN32)
    set(Boost_USE_STATIC_LIBS ON)
else()
    set(Boost_USE_STATIC_LIBS OFF)
endif()
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.74.0 REQUIRED COMPONENTS system)

find_package(simdjson ${SIMDJSON_VERSION} QUIET)
if(NOT simdjson_FOUND)
    FetchContent_Declare(
        simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG ${SIMDJSON_TAG}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(simdjson)
endif()

find_package(nlohmann_json ${NLOHMANN_JSON_VERSION} QUIET)
if(NOT nlohmann_json_FOUND)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG ${NLOHMANN_JSON_TAG}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(cpr ${CPR_VERSION} QUIET)
if(NOT cpr_FOUND)
    set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use the distro libcurl package" FORCE)
    set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CPR_BUILD_TESTS_SSL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG ${CPR_VERSION}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(cpr)
endif()

find_package(raylib ${RAYLIB_VERSION} QUIET)
if(NOT raylib_FOUND)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG ${RAYLIB_VERSION}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
endif()

set(RAYLIB_CPP_IS_MAIN FALSE)
option(BUILD_RAYLIB_CPP_EXAMPLES "Build raylib-cpp examples" ${RAYLIB_CPP_IS_MAIN})

add_subdirectory(include)

set(includes_dir
    ${CMAKE_CURRENT_SOURCE_DIR}/http
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
)

set(inc
    ${CMAKE_CURRENT_SOURCE_DIR}/http/postRequest.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/websocketConnect.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/websocketSession.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/version.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/event.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/uuidGenerator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/threadSafeQueue.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/windowContext.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/carClass.hpp
)

set(source
    ${CMAKE_CURRENT_SOURCE_DIR}/http/postRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/websocketConnect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/websocketSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/windowContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/carClass.cpp
    main.cpp
)

add_executable(${ProjName} ${inc} ${source})

target_link_libraries(${ProjName} PRIVATE
    raylib_cpp
    raylib
    simdjson::simdjson
    cpr::cpr
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
)

target_include_directories(${ProjName} PRIVATE
    ${includes_dir}
    ${Boost_INCLUDE_DIRS}
)

target_compile_features(${ProjName} PRIVATE cxx_std_17)

add_custom_command(TARGET ${ProjName} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:${ProjName}>/resources")

foreach(runtime_file IN ITEMS server.crt .env)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${runtime_file}")
        add_custom_command(TARGET ${ProjName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/${runtime_file}"
                "$<TARGET_FILE_DIR:${ProjName}>")
    else()
        message(WARNING "${runtime_file} is missing; run scripts/linux_bootstrap.sh or create it before running the client.")
    endif()
endforeach()
