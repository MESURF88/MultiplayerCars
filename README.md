# NodeServer
```
contains the javascript socket.io code to communicates with the clients
```
# CPPClient
```
contains the c++ code for socket.io and raylab library to create graphics
```
# installing for client
```
need to install openssl for windows
get chocolatey with the instructions install chocolatey for individual use
https://chocolatey.org/install

then open an administrator terminal, and run
choco install openssl

and should see this text:
The install of openssl was successful.
Software installed to 'C:\Program Files\OpenSSL-Win64\

install latest x64 cmake
https://cmake.org/download/

add C:\Program Files\CMake\bin to the user environmental variable PATH

need at least visual studio 2019 and this path to vsvars64.bat
C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build

run administrator, setup_release_dependencies.bat in the CPPClient folder
the sioclient libraries should be installed in
-- Install configuration: "Release"
-- Installing: C:/Program Files (x86)/sioclient/include/sio_client.h
-- Installing: C:/Program Files (x86)/sioclient/include/sio_message.h
-- Installing: C:/Program Files (x86)/sioclient/include/sio_socket.h
-- Installing: C:/Program Files (x86)/sioclient/lib/sioclient.lib
-- Installing: C:/Program Files (x86)/sioclient/lib/sioclient_tls.lib
-- Installing: C:/Program Files (x86)/sioclient/lib/cmake/sioclient/sioclientTargets.cmake
-- Installing: C:/Program Files (x86)/sioclient/lib/cmake/sioclient/sioclientTargets-release.cmake
-- Installing: C:/Program Files (x86)/sioclient/lib/cmake/sioclient/sioclientConfig.cmake
-- Installing: C:/Program Files (x86)/sioclient/lib/cmake/sioclient/sioclientConfigVersion.cmake
-- Up-to-date: C:/Program Files (x86)/sioclient/lib/cmake/sioclient/sioclientTargets.cmake
```

# building client
```
once all the installation is done, or a source file for the client has been modified run either
build_release_client_nmake.bat
or
build_default_client.bat
to build an executable in nmakebuild or a solution in build respectively

NOTE: if cmakecache error when running scripts, delete the target folder in CPPClient

building with tls
Have built socket.io-client-cpp app with SIO_TLS in DEFINES (compiler flag: -DSIO_TLS) - connects via https fine now! This enables TLS support as mentioned here:
```

# running client and server locally
```
first run server in terminal:
cd .\NodeServer
npm start

in another terminal run client (for nmake builds)
.\CPPClient\nmakebuild\carclient.exe
(for visual studio builds)
.\CPPClient\build\carclient.exe
```

safe-depths-24899
