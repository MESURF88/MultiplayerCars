#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tools_dir="$repo_root/.tools"
cmake_version="4.3.4"
go_version="1.26.4"

version_ge() {
    [ "$(printf '%s\n' "$2" "$1" | sort -V | head -n1)" = "$2" ]
}

install_apt_packages() {
    if ! command -v apt-get >/dev/null 2>&1; then
        echo "apt-get was not found. Install the packages listed in README.md for your distro, then rerun this script."
        return
    fi

    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        ca-certificates \
        curl \
        git \
        libasound2-dev \
        libboost-system-dev \
        libcurl4-openssl-dev \
        libgl1-mesa-dev \
        libssl-dev \
        libx11-dev \
        libxcursor-dev \
        libxi-dev \
        libxinerama-dev \
        libxrandr-dev \
        ninja-build \
        openssl \
        perl \
        pkg-config
}

ensure_cmake() {
    if command -v cmake >/dev/null 2>&1; then
        local found_version
        found_version="$(cmake --version | head -n1 | awk '{print $3}')"
        if version_ge "$found_version" "$cmake_version"; then
            echo "Using CMake $found_version"
            return
        fi
    fi

    mkdir -p "$tools_dir"
    local arch package_name package_dir package_url
    arch="$(uname -m)"
    case "$arch" in
        x86_64|amd64)
            package_name="cmake-${cmake_version}-linux-x86_64"
            ;;
        aarch64|arm64)
            package_name="cmake-${cmake_version}-linux-aarch64"
            ;;
        *)
            echo "Unsupported CMake bootstrap architecture: $arch"
            exit 1
            ;;
    esac

    package_dir="$tools_dir/$package_name"
    package_url="https://github.com/Kitware/CMake/releases/download/v${cmake_version}/${package_name}.tar.gz"

    if [ ! -x "$package_dir/bin/cmake" ]; then
        echo "Installing CMake $cmake_version into $package_dir"
        curl -fsSL "$package_url" -o "$tools_dir/$package_name.tar.gz"
        tar -xzf "$tools_dir/$package_name.tar.gz" -C "$tools_dir"
        rm -f "$tools_dir/$package_name.tar.gz"
    fi

    export PATH="$package_dir/bin:$PATH"
}

ensure_go() {
    if command -v go >/dev/null 2>&1; then
        local found_version
        found_version="$(go version | awk '{print $3}' | sed 's/^go//')"
        if version_ge "$found_version" "$go_version"; then
            echo "Using Go $found_version"
            return
        fi
    fi

    mkdir -p "$tools_dir"
    local arch package_name install_dir package_url
    arch="$(uname -m)"
    case "$arch" in
        x86_64|amd64)
            package_name="go${go_version}.linux-amd64.tar.gz"
            install_dir="$tools_dir/go${go_version}.linux-amd64"
            ;;
        aarch64|arm64)
            package_name="go${go_version}.linux-arm64.tar.gz"
            install_dir="$tools_dir/go${go_version}.linux-arm64"
            ;;
        *)
            echo "Unsupported Go bootstrap architecture: $arch"
            exit 1
            ;;
    esac

    package_url="https://go.dev/dl/$package_name"

    if [ ! -x "$install_dir/bin/go" ]; then
        echo "Installing Go $go_version into $install_dir"
        curl -fsSL "$package_url" -o "$tools_dir/$package_name"
        rm -rf "$install_dir"
        mkdir -p "$install_dir"
        tar -xzf "$tools_dir/$package_name" -C "$install_dir" --strip-components=1
        rm -f "$tools_dir/$package_name"
    fi

    export PATH="$install_dir/bin:$PATH"
}

ensure_local_files() {
    mkdir -p "$repo_root/GoServer/keys"

    if [ ! -f "$repo_root/GoServer/keys/server.crt" ] || [ ! -f "$repo_root/GoServer/keys/server.key" ]; then
        echo "Generating local TLS certificate for localhost"
        openssl req -x509 -newkey rsa:2048 -nodes \
            -keyout "$repo_root/GoServer/keys/server.key" \
            -out "$repo_root/GoServer/keys/server.crt" \
            -days 365 \
            -subj "/CN=localhost" \
            -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"
    fi

    cp "$repo_root/GoServer/keys/server.crt" "$repo_root/CPPClient/server.crt"

    if [ ! -f "$repo_root/CPPClient/.env" ]; then
        cat > "$repo_root/CPPClient/.env" <<'EOF'
{
  "username": "hill",
  "password": "1995"
}
EOF
    fi
}

bootstrap_go_server() {
    (cd "$repo_root/GoServer" && go mod tidy)
}

configure_client() {
    cmake -S "$repo_root/CPPClient" -B "$repo_root/CPPClient/builddbg" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDEBUGLOCAL=ON
}

install_apt_packages
ensure_cmake
ensure_go
ensure_local_files
bootstrap_go_server
configure_client

cat <<EOF

Linux bootstrap complete.

Run the local server:
  bash "$repo_root/scripts/linux_run_server.sh"

Build and run the debug client:
  bash "$repo_root/CPPClient/build_default_debug_client.sh"
  "$repo_root/CPPClient/builddbg/carclient"
EOF
