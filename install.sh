#!/bin/bash

# Simple installation script for Browser--
# Checks for dependencies, installs them if possible, and builds the project.

set -e

# Define colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Starting installation for Browser--...${NC}"

# Function to check for commands
check_command() {
    if ! command -v "$1" &> /dev/null; then
        echo -e "${YELLOW}Warning: $1 is not installed.${NC}"
        return 1
    fi
    return 0
}

# Function to check for packages (using dpkg for debian-based)
check_package_deb() {
    if ! dpkg -l "$1" &> /dev/null; then
        echo -e "${YELLOW}Warning: package $1 is not installed.${NC}"
        return 1
    fi
    return 0
}

# Check for essential tools
MISSING_TOOLS=0
for tool in g++ cmake pkg-config; do
    check_command "$tool" || MISSING_TOOLS=$((MISSING_TOOLS + 1))
done

# Function to check for dev headers using pkg-config
check_lib() {
    if ! pkg-config --exists "$1"; then
        echo -e "${YELLOW}Warning: development headers for $1 are missing.${NC}"
        return 1
    fi
    return 0
}

# If tools are missing or headers are likely missing, try to install
NEED_INSTALL=0
if [ "$MISSING_TOOLS" -gt 0 ]; then NEED_INSTALL=1; fi
if ! check_lib libcurl; then NEED_INSTALL=1; fi
if ! check_lib lua5.4; then NEED_INSTALL=1; fi

if [ "$NEED_INSTALL" -eq 1 ]; then
    echo -e "${YELLOW}Missing essential tools or development headers. Attempting to install...${NC}"
    
    if [ -f /etc/debian_version ]; then
        echo -e "${GREEN}Detected Debian/Ubuntu system.${NC}"
        sudo apt update || echo -e "${RED}Failed to update package list. Proceeding anyway...${NC}"
        sudo apt install -y build-essential cmake pkg-config libcurl4-openssl-dev libsdl3-dev libsdl3-ttf-dev liblua5.4-dev lua5.4
    elif [ -f /etc/fedora-release ] || [ -f /etc/redhat-release ]; then
        echo -e "${GREEN}Detected Fedora/RHEL system.${NC}"
        sudo dnf install -y gcc-c++ cmake pkgconf-pkg-config libcurl-devel SDL3-devel SDL3_ttf-devel lua-devel
    else
        echo -e "${RED}Unsupported OS for auto-installation. Please install dependencies manually as listed in the README.${NC}"
        exit 1
    fi
fi

# Create build directory
echo -e "${GREEN}Configuring the project...${NC}"
mkdir -p build
cd build

# Run CMake
if ! cmake ..; then
    echo -e "${RED}Error: CMake configuration failed.${NC}"
    exit 1
fi

# Build the project
echo -e "${GREEN}Building the project...${NC}"
if ! cmake --build .; then
    echo -e "${RED}Error: Build failed.${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful! The executable is located at build/browser${NC}"
echo -e "${YELLOW}To run: ./build/browser assets/index.jsml${NC}"
