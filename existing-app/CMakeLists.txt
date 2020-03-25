# CMakeList.txt : CMake project for existing-app, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("existing-app")

set(header_files
    ./device_capability.h
)

set(source_files
    ./device_capability.c
)

# Add source to this project's executable.
add_executable (existing-app "main.c" ${header_files} ${source_files})
