cmake_minimum_required(VERSION 3.24.0)

set(ProjName carclient)

project(${ProjName} VERSION 1.0.0 LANGUAGES CXX)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DEBUGLOCAL "Connect the client to a local development server" OFF)

if(DEBUGLOCAL)
    add_compile_definitions(DEBUG_CLIENT)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring Debug client")
else()
    message(STATUS "Configuring Release client")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(RAYLIB_VERSION 6.0)
set(RAYLIB_CPP_VERSION v6.0.2)
set(CPR_VERSION 1.14.0)
set(NLOHMANN_JSON_VERSION 3.12.0)
set(NLOHMANN_JSON_TAG v3.12.0)
set(SIMDJSON_VERSION 4.6.4)
set(SIMDJSON_TAG v4.6.4)

find_package(OpenSSL REQUIRED)

option(MULTIPLAYER_CARS_USE_STATIC_BOOST "Link Boost static libraries" OFF)
set(Boost_USE_STATIC_LIBS ${MULTIPLAYER_CARS_USE_STATIC_BOOST})
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
    if(WIN32)
        set(CPR_USE_SYSTEM_CURL OFF CACHE BOOL "Use cpr's managed curl dependency on Windows" FORCE)
        set(CPR_CURL_USE_LIBPSL OFF CACHE BOOL "Avoid Meson-only libpsl dependency for bundled curl on Windows" FORCE)
    else()
        set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "Use the distro libcurl package" FORCE)
    endif()
    set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CPR_BUILD_TESTS_SSL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG ${CPR_VERSION}
        GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(cpr)
    if(NOT cpr_POPULATED)
        FetchContent_Populate(cpr)
        if(MSVC)
            set(CPR_SSE_SOURCE "${cpr_SOURCE_DIR}/cpr/sse.cpp")
            file(READ "${CPR_SSE_SOURCE}" CPR_SSE_CONTENT)
            string(REPLACE
                "std::from_chars(sv.begin(), sv.end(), retry_value)"
                "std::from_chars(sv.data(), sv.data() + sv.size(), retry_value)"
                CPR_SSE_CONTENT
                "${CPR_SSE_CONTENT}"
            )
            file(WRITE "${CPR_SSE_SOURCE}" "${CPR_SSE_CONTENT}")
        endif()
        add_subdirectory("${cpr_SOURCE_DIR}" "${cpr_BINARY_DIR}")
    endif()
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
find_package(raylib_cpp 6.0.2 QUIET)
if(NOT raylib_cpp_FOUND)
    FetchContent_Declare(
        raylib_cpp
        GIT_REPOSITORY https://github.com/RobLoach/raylib-cpp.git
        GIT_TAG ${RAYLIB_CPP_VERSION}
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib_cpp)
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/courseMap.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/generatedCourses.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raceSession.hpp
)

set(source
    ${CMAKE_CURRENT_SOURCE_DIR}/http/postRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/websocketConnect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/websocket/websocketSession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/windowContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/carClass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/courseMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/generatedCourses.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raceSession.cpp
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

if(WIN32)
    add_custom_command(TARGET ${ProjName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${ProjName}>
            $<TARGET_FILE_DIR:${ProjName}>
        COMMAND_EXPAND_LISTS)
endif()

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
