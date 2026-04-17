#!/bin/sh

# x - Print all commands in script.
# e - stop script running when error.
set -xe

# main.cpp - route solver entry point

CFlags="-Wall -Wextra -ggdb -O0 -Wno-sign-compare -Wno-writable-strings -Wno-missing-field-initializers -Wno-unused-variable -Wno-unused-function"
SourceFiles="main.cpp"

# Create folder "build", if not exists. Change it later.
mkdir -p ../../build

clang++ $CFlags $SourceFiles -o ../../build/route_solver
