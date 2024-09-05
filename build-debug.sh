#!/bin/bash

# Check if g++ is installed and the version is at least 12.2
check_gpp_version() {
    if ! command -v g++ &> /dev/null; then
        echo "g++ is not installed. Please install g++."
        exit 1
    fi

    gpp_version=$(g++ --version | head -n 1 | awk '{print $4}')
    major_version=$(echo "$gpp_version" | cut -d. -f1)
    minor_version=$(echo "$gpp_version" | cut -d. -f2)
    patch_version=$(echo "$gpp_version" | cut -d. -f3)

    if [[ "$major_version" -lt 12 ]] || { [[ "$major_version" -eq 12 ]] && [[ "$minor_version" -lt 2 ]]; } || { [[ "$major_version" -eq 12 ]] && [[ "$minor_version" -eq 2 ]] && [[ "$patch_version" -lt 0 ]]; }; then
        echo "g++ version is too old. Minimum required version is 12.2.0."
        exit 1
    fi
}

# Check if libpci-dev is installed
check_libpci_dev() {
    if ! dpkg -s libpci-dev &> /dev/null; then
        echo "libpci-dev is not installed. Please install libpci-dev."
        exit 1
    fi
}

# Compile the program
compile_program() {
    mkdir -p build
    g++ src/main.cpp src/fort.c -o build/saiki -I src -L /usr/lib/x86_64-linux-gnu -lpci -std=c++23 -O0 -g
    if [ $? -eq 0 ]; then
        echo "Compilation successful."
    else
        echo "Compilation failed."
        exit 1
    fi
}

# Main function to run the checks and compile the program
main() {
    check_gpp_version
    check_libpci_dev
    compile_program
}

main
