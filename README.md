# GoServer

```
contains the Go websocket code to communicates with the clients
```

# CPPClient

```
contains the c++ code for boost cpr (curlib) for websockets and raylab library to create graphics
```

# install Go windows

```
https://go.dev/doc/install
```

# install Perl windows

```
need to install perl x64
https://strawberryperl.com/

It should place the exe in
C:/Strawberry/perl/bin/perl.exe
```

# installing for client windows

```

1. need to install openssl for windows
get chocolatey with the instructions install chocolatey for individual use
https://chocolatey.org/install

then open an administrator terminal, and run
choco install openssl

(choose Y yes to all)

and should see this text:
The install of openssl was successful.
Software installed to 'C:\Program Files\OpenSSL-Win64\

2. then install curl for 64-bit
https://curl.se/windows/

this will be installed in C:\ProgramData\chocolatey\lib\curl\tools

3. install boost following the instructions
https://github.com/ErisExchange/socket.io-client-cpp/blob/eris-sio_tls_support_non_tls_uris/BOOST.md
after downloading and extracting run
bootstrap.bat
then open a administrator prompt and run this command in the folder:
.\b2 install --prefix="C:\Program Files\boost" --with-system --with-date_time --with-random --with-thread --with-chrono link=static runtime-link=shared threading=multi

4. install latest x64 cmake
https://cmake.org/download/

5. Add library paths
add C:\Program Files\CMake\bin to the user environmental variable PATH
add C:\Program Files\OpenSSL-Win64\include to the user environmental variable PATH

6. check to see, at least visual studio 2019 and this path to vsvars64.bat
C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build

7. run setup_release_dependencies.bat as administator then setup_debug_dependencies.bat as administator

for curl
-- Install configuration: "Release"
-- Installing: C:/Program Files (x86)/CURL/lib/libcurl-d_imp.lib
-- Installing: C:/Program Files (x86)/CURL/bin/libcurl-d.dll
-- Installing: C:/Program Files (x86)/CURL/bin/curl.exe
-- Installing: C:/Program Files (x86)/CURL/bin/curl-config
-- Installing: C:/Program Files (x86)/CURL/lib/pkgconfig/libcurl.pc
-- Installing: C:/Program Files (x86)/CURL/include/curl
-- Installing: C:/Program Files (x86)/CURL/include/curl/curl.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/curlver.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/easy.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/header.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/mprintf.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/multi.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/options.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/stdcheaders.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/system.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/typecheck-gcc.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/urlapi.h
-- Installing: C:/Program Files (x86)/CURL/include/curl/websockets.h
-- Installing: C:/Program Files (x86)/CURL/lib/cmake/CURL/CURLTargets.cmake
-- Installing: C:/Program Files (x86)/CURL/lib/cmake/CURL/CURLTargets-debug.cmake
-- Installing: C:/Program Files (x86)/CURL/lib/cmake/CURL/CURLConfigVersion.cmake
-- Installing: C:/Program Files (x86)/CURL/lib/cmake/CURL/CURLConfig.cmake

for cpr
-- Install configuration: "Release"
-- Installing: C:/Program Files (x86)/cpr/lib/zlib.lib
-- Installing: C:/Program Files (x86)/cpr/bin/zlib.dll
-- Installing: C:/Program Files (x86)/cpr/include/zlib.h
-- Installing: C:/Program Files (x86)/cpr/include/zconf.h
-- Installing: C:/Program Files (x86)/cpr/lib/pkgconfig/zlib.pc
-- Installing: C:/Program Files (x86)/cpr/lib/libcurl_imp.lib
-- Installing: C:/Program Files (x86)/cpr/bin/libcurl.dll
-- Installing: C:/Program Files (x86)/cpr/bin/curl-config
-- Installing: C:/Program Files (x86)/cpr/lib/pkgconfig/libcurl.pc
-- Installing: C:/Program Files (x86)/cpr/include/curl
-- Installing: C:/Program Files (x86)/cpr/include/curl/curl.h
-- Installing: C:/Program Files (x86)/cpr/include/curl/curlver.h
-- Installing: C:/Program Files (x86)/cpr/include/curl/easy.h
-- Installing: C:/Program Files (x86)/cpr/include/curl/mprintf.h
-- Installing: C:/Program Files (x86)/cpr/include/curl/multi.h
-- Installing: C:/Program Files (x86)/cpr/include/curl/options.h
-- Installing: C:/Program Files (x86)/cpr/include/curl/stdcheaders.h
-- Installing: C:/Program Files (x86)/cpr/include/curl/system.h
-- Installing: C:/Program Files (x86)/cpr/include/curl/typecheck-gcc.h
-- Installing: C:/Program Files (x86)/cpr/include/curl/urlapi.h
-- Installing: C:/Program Files (x86)/cpr/lib/cmake/CURL/CURLTargets.cmake
-- Installing: C:/Program Files (x86)/cpr/lib/cmake/CURL/CURLTargets-release.cmake
-- Installing: C:/Program Files (x86)/cpr/lib/cmake/CURL/CURLConfigVersion.cmake
-- Installing: C:/Program Files (x86)/cpr/lib/cmake/CURL/CURLConfig.cmake
-- Installing: C:/Program Files (x86)/cpr/lib/cpr.lib
-- Installing: C:/Program Files (x86)/cpr/bin/cpr.dll
-- Installing: C:/Program Files (x86)/cpr/include/cpr
-- Installing: C:/Program Files (x86)/cpr/include/cpr/accept_encoding.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/api.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/async.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/async_wrapper.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/auth.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/bearer.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/body.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/buffer.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/callback.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/cert_info.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/connect_timeout.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/cookies.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/cpr.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/cprtypes.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/curlholder.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/curlmultiholder.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/curl_container.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/error.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/file.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/filesystem.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/http_version.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/interceptor.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/interface.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/limit_rate.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/local_port.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/local_port_range.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/low_speed.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/multipart.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/multiperform.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/parameters.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/payload.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/proxies.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/proxyauth.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/range.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/redirect.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/reserve_size.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/resolve.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/response.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/session.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/singleton.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/ssl_ctx.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/ssl_options.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/status_codes.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/threadpool.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/timeout.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/unix_socket.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/user_agent.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/util.h
-- Installing: C:/Program Files (x86)/cpr/include/cpr/verbose.h
-- Up-to-date: C:/Program Files (x86)/cpr/include/cpr
-- Installing: C:/Program Files (x86)/cpr/include/cpr/cprver.h

8. add curl to the PATH
add C:\Program Files (x86)\CURL\lib to the user environmental variable PATH
add C:\Program Files (x86)\CURL\include to the user environmental variable PATH

9. add cpr to the PATH
add C:\Program Files (x86)\cpr\lib to the user environmental variable PATH
add C:\Program Files (x86)\cpr\include to the user environmental variable PATH
```

# building client windows

```
once all the installation is done, or a source file for the client has been modified run either
build_debug_client_nmake.bat
or
build_default_debug_client.bat
to build an executable in nmakebuilddbg or a solution in builddbg respectively

to build release, must first run the setup_release_dependencies.bat before building with the corresponding scripts,
build_release_client_nmake.bat
or
build_default_release_client.bat
to build an executable in nmakebuild or a solution in build respectively

if need to rebuild the dependencies (curl, cpr, nlohmann/json), then run the clean_dependencies.bat before the setup_debug_dependencies or setup_release_dependencies script

NOTE: if cmakecache error when running scripts, delete the target (targetdbg for Debug) (target for Release) folder in CPPClient
and re run build script. This occurs when switching compilers or generators.

```

# running client and server locally windows

```
first run server in terminal:
cd .\GoServer
go run .

For the client it must be compiled with the preprocessor definition:
DEBUG_CLIENT
this can be done in visual studio Project Settings
for the nmake build script, it will compile the debug executable with the DEBUG_CLIENT definition automatically.

in the root directory, in another terminal run client (for nmake builds)
.\CPPClient\nmakebuild\carclient.exe
(for visual studio builds)
.\CPPClient\build\carclient.exe

or use visual studio if (default builds)
```


# installing for client linux

```
run the setup_dependencies.sh script in CPPClient
```

# building client linux

```
once all the installation is done, or a source file for the client has been modified run either
build_default_debug_client.sh
to build an executable in builddbg

to build release, run
build_default_debug_client.sh
to build an executable in build

if need to rebuild the dependencies (curl, cpr, nlohmann/json), the run the clean_dependencies.sh

NOTE: if cmakecache error when running scripts, delete the target (targetdbg for Debug) (target for Release) folder in CPPClient
and re run build script

```

# running client and server locally linux

```
first run server in terminal:
cd GoServer
go run .

in the root directory, run the debug client from a terminal use the following two commands
export LD_LIBRARY_PATH=/usr/local/lib
./CPPClient/builddbg/carclient

likewise for release (this will connect to the online server)
export LD_LIBRARY_PATH=/usr/local/lib
./CPPClient/build/carclient
```

# certificates for running client and server
```
the GoServer Folder needs a directory named keys and containing the following:
- server.crt
- server.key
the CPPClient needs the following in its directory:
- .env
- server.crt
```

# testing server
```
use piesockettester extension for google chrome
```

# online server at
```
https://onlinecarsimgame-ab2533447e53.herokuapp.com/
```
