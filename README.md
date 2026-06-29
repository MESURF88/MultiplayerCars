# GoServer

```
contains the Go websocket code to communicates with the clients
```

# CPPClient

```
contains the C++ code for Boost.Beast websockets, cpr/libcurl HTTP, and raylib graphics
```

# installing for client windows

Recommended Windows setup from the repository root. Open PowerShell in the root of your `MultiplayerCars` checkout:

```powershell
...\MultiplayerCars
```

Then run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/windows_setup_system.ps1
```

This checks for the machine-level tools:
- Visual Studio 2022 Build Tools with C++ tools
- CMake
- Ninja (optional, useful for alternate CMake workflows)
- Git
- Go

To install missing tools with `winget`, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/windows_setup_system.ps1 -Install
```

Open a new PowerShell terminal after installing tools so PATH updates are visible.

# configuring client windows dependencies

Project-level setup is separate from system setup:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/windows_setup_project.ps1
```

This script:
- clones and bootstraps `vcpkg` into ignored `.tools/vcpkg`
- restores Windows native libraries from `CPPClient/vcpkg.json`
- installs Boost.Beast, Boost.System, and OpenSSL for the CMake toolchain
- lets CMake/FetchContent restore code libraries such as raylib, cpr, nlohmann/json, and simdjson
- creates `CPPClient/.env` if missing
- copies/generates local TLS files when `openssl` is available
- configures the `windows-debug-local` CMake preset

To restore dependencies and local runtime files without configuring CMake, add `-SkipConfigure`.

The CMake presets are defined in `CMakePresets.json`, so Visual Studio and the VS Code CMake Tools extension can open the repository root and build directly.

# building client windows

Command-line debug/local-server build:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/windows_build_client.ps1
```

Release/online-server build:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/windows_setup_project.ps1 -Configuration Release
powershell -ExecutionPolicy Bypass -File scripts/windows_build_client.ps1 -Configuration Release
```

Visual Studio:
- Open the repository folder.
- Select the `windows-debug-local` preset.
- Build `carclient`.

VS Code:
- Install the CMake Tools extension.
- Open the repository folder.
- Select the `windows-debug-local` configure preset.
- Build from the CMake sidebar or run `CMake: Build`.

The old `CPPClient/*.bat` scripts remain for reference, but the recommended Windows flow is the PowerShell/CMake preset path.

# running client and server locally windows

Run the local server:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/windows_run_server.ps1
```

Run the debug client:

```powershell
.\CPPClient\builddbg\Debug\carclient.exe
```

# installing for client linux

```
Recommended one-command setup from the repository root:

bash scripts/linux_bootstrap.sh

This script:
- installs the native Linux build packages through apt
- uses CMake 4.3.4 locally if the system CMake is older
- uses Go 1.26.4 locally if the system Go is older
- generates local TLS files for GoServer/keys and CPPClient/server.crt
- creates CPPClient/.env if it is missing
- runs go mod tidy for the server
- configures the debug C++ client in CPPClient/builddbg

The C++ library versions are now pinned in CPPClient/CMakeLists.txt:
- raylib 6.0
- raylib-cpp v6.0.2
- cpr 1.14.0
- nlohmann/json v3.12.0
- simdjson v4.6.4

Boost, OpenSSL, curl, X11/OpenGL, and audio dependencies are installed as distro
packages. CMake links them through package targets instead of relying on
/usr/local/lib or manually cloned dependency folders.
```

# building client linux

```
Debug/local server build:

bash CPPClient/build_default_debug_client.sh

Release/online server build:

bash CPPClient/build_default_release_client.sh

If CMake cache errors occur after changing generators or compilers, remove the
corresponding build folder and rerun the build script:

rm -rf CPPClient/builddbg CPPClient/build

```

# generating courses

```
py tools/course_generator.py
```

The generator writes:
- `CPPClient/resources/courses/simple_circuit.json` for runtime loading
- `CPPClient/resources/courses/simple_circuit.svg` as a quick top-down preview
- `CPPClient/generatedCourses.cpp` as the compiled fallback course data

In WSL/Linux, run the same utility with `python3 tools/course_generator.py`.

# running client and server locally linux

```
first run server in terminal:
bash scripts/linux_run_server.sh

in another terminal from the repository root, run the debug client:
./CPPClient/builddbg/carclient

likewise for release (this will connect to the online server):
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

On Linux, scripts/linux_bootstrap.sh creates these local development files if
they are missing. CPPClient/.env is intentionally ignored by git because it
contains login credentials.
```

# testing server
```
use piesockettester extension for google chrome
```

# online server at
```
https://multiplayercars.onrender.com/
```
