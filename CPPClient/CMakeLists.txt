cmake_minimum_required(VERSION 3.16.0)
set(ProjName carclient)

project(${ProjName} VERSION 1.0.0 LANGUAGES CXX)

# C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(DEBUGLOCAL EQUAL 1)
add_compile_definitions(DEBUG_CLIENT)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Debug")
    set(CMAKE_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/targetdbg")
	set(Boost_USE_DEBUG_LIBS ON) 
else()
    message("Release")
    set(CMAKE_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/target")
	set(Boost_USE_RELEASE_LIBS ON) 
endif()
message("${CMAKE_BINARY_DIR}")

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if (MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif(MSVC)

if(Boost_FOUND)   
    message("Boost 1.82 found")
endif()

find_package(OpenSSL)

if (OPENSSL_FOUND)
message("OpenSSL found")
message("${OPENSSL_INCLUDE_DIR}")
else()
if (WIN32)
message("OpenSSL not Found on Windows, attempt to manually set paths")
set(OPENSSL_INCLUDE_DIR "C:/Program\ Files/OpenSSL-Win64/include")
else()
message("OpenSSL not Found, attempt to reinstall the package for openssl")
endif()
endif()

# boost (websockets)
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and 
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs 
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF) 
find_package(Boost 1.82.0 COMPONENTS date_time system random thread)

if(Boost_FOUND)   
    message("Boost 1.82 found")
endif()

#cpr (http)
find_library(CPR_LIB
NAMES cpr
HINTS "C:/Program\ Files\ (x86)/cpr/lib")
message(STATUS "cpr LIB: [${CPR_LIB}]")
#json
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(json)

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

# include directory
set(includes_dir
${CMAKE_CURRENT_SOURCE_DIR}/http
${CMAKE_CURRENT_SOURCE_DIR}/websocket
${CMAKE_CURRENT_SOURCE_DIR}/gui
)

# include files
set(inc
${CMAKE_CURRENT_SOURCE_DIR}/http/postRequest.hpp
${CMAKE_CURRENT_SOURCE_DIR}/websocket/websocketConnect.hpp
${CMAKE_CURRENT_SOURCE_DIR}/websocket/websocketSession.hpp
${CMAKE_CURRENT_SOURCE_DIR}/gui/event.hpp
${CMAKE_CURRENT_SOURCE_DIR}/gui/uuidGenerator.hpp
${CMAKE_CURRENT_SOURCE_DIR}/gui/threadSafeQueue.hpp
${CMAKE_CURRENT_SOURCE_DIR}/gui/windowContext.hpp
${CMAKE_CURRENT_SOURCE_DIR}/gui/carClass.hpp
)

# source files
set(source
${CMAKE_CURRENT_SOURCE_DIR}/http/postRequest.cpp
${CMAKE_CURRENT_SOURCE_DIR}/websocket/websocketConnect.cpp
${CMAKE_CURRENT_SOURCE_DIR}/websocket/websocketSession.cpp
${CMAKE_CURRENT_SOURCE_DIR}/gui/windowContext.cpp
${CMAKE_CURRENT_SOURCE_DIR}/gui/carClass.cpp
main.cpp
)

# Executable
add_executable(${ProjName} ${inc} ${source})

if(WIN32)
target_link_libraries(${ProjName} PUBLIC raylib_cpp raylib 
${CPR_LIB} 
nlohmann_json::nlohmann_json 
${OPENSSL_INCLUDE_DIR}/../lib/libssl.lib
${OPENSSL_INCLUDE_DIR}/../lib/libcrypto.lib
"$<$<CONFIG:Debug>:${Boost_INCLUDE_DIRS}/../../lib/libboost_date_time-vc142-mt-gd-x64-1_82.lib>"
"$<$<CONFIG:Debug>:${Boost_INCLUDE_DIRS}/../../lib/libboost_system-vc142-mt-gd-x64-1_82.lib>"
"$<$<CONFIG:Debug>:${Boost_INCLUDE_DIRS}/../../lib/libboost_random-vc142-mt-gd-x64-1_82.lib>"
"$<$<CONFIG:Debug>:${Boost_INCLUDE_DIRS}/../../lib/libboost_thread-vc142-mt-gd-x64-1_82.lib>"
"$<$<CONFIG:Debug>:${Boost_INCLUDE_DIRS}/../../lib/libboost_chrono-vc142-mt-gd-x64-1_82.lib>"
"$<$<CONFIG:Release>:${Boost_INCLUDE_DIRS}/../..//lib/libboost_date_time-vc142-mt-x64-1_82.lib>"
"$<$<CONFIG:Release>:${Boost_INCLUDE_DIRS}/../../lib/libboost_system-vc142-mt-x64-1_82.lib>"
"$<$<CONFIG:Release>:${Boost_INCLUDE_DIRS}/../../lib/libboost_random-vc142-mt-x64-1_82.lib>"
"$<$<CONFIG:Release>:${Boost_INCLUDE_DIRS}/../../lib/libboost_thread-vc142-mt-x64-1_82.lib>"
"$<$<CONFIG:Release>:${Boost_INCLUDE_DIRS}/../../lib/libboost_chrono-vc142-mt-x64-1_82.lib>"
)
else()
target_link_libraries(${ProjName} PUBLIC raylib_cpp raylib 
${CPR_LIB} 
nlohmann_json::nlohmann_json 
libssl.a
libcrypto.a
${Boost_LIBRARIES}
)
endif()

target_include_directories(${ProjName} PRIVATE
    ${includes_dir}
    "${CMAKE_CURRENT_SOURCE_DIR}/json/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpr/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/curl/include"
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)
target_compile_features(${ProjName} PRIVATE cxx_std_17)


if (WIN32)
file(COPY "C:/Program\ Files\ (x86)/cpr/include/cpr/cprver.h" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/cpr/include/cpr")
endif()

if (WIN32)
add_custom_command(TARGET ${ProjName} POST_BUILD       
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/cpr/$<$<CONFIG:Release>:cpr-build/bin/Release/libcurl.dll>$<$<CONFIG:Debug>:cpr-builddbg/bin/Debug/libcurl-d.dll>"
        $<TARGET_FILE_DIR:${ProjName}>)
		
add_custom_command(TARGET ${ProjName} POST_BUILD       
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/cpr/$<$<CONFIG:Release>:cpr-build/bin/Release>$<$<CONFIG:Debug>:cpr-builddbg/bin/Debug>/cpr.dll"
        $<TARGET_FILE_DIR:${ProjName}>)

add_custom_command(TARGET ${ProjName} POST_BUILD       
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/cpr/$<$<CONFIG:Release>:cpr-build/bin/Release>$<$<CONFIG:Debug>:cpr-builddbg/bin/Debug>/zlib.dll"
        $<TARGET_FILE_DIR:${ProjName}>)
		
add_custom_command(TARGET ${ProjName} POST_BUILD       
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${OPENSSL_INCLUDE_DIR}/../libcrypto-1_1-x64.dll"
		$<TARGET_FILE_DIR:${ProjName}>) 
		
add_custom_command(TARGET ${ProjName} POST_BUILD       
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${OPENSSL_INCLUDE_DIR}/../libssl-1_1-x64.dll"
		$<TARGET_FILE_DIR:${ProjName}>)
endif()

add_custom_command(TARGET ${ProjName} POST_BUILD       
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/server.crt"
        $<TARGET_FILE_DIR:${ProjName}>) 
		
add_custom_command(TARGET ${ProjName} POST_BUILD       
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		"${CMAKE_CURRENT_SOURCE_DIR}/.env"
		$<TARGET_FILE_DIR:${ProjName}>) 
		
#add_custom_command(TARGET ${ProjName} POST_BUILD       
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#       "${CMAKE_CURRENT_SOURCE_DIR}/target/_deps/raylib-build/raylib/raylib.dll"      # <--this is in-file TODO make raylib dll
#        $<TARGET_FILE_DIR:${ProjName}>)                 # <--this is out-file path
