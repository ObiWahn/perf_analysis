#!/bin/bash

# get 3rdParty libs
./init_libs

# create dir
mkdir -p build
cd build || exit 1

#configure / compile
CXXFLAGS='-Wall -Werror -fsanitize=address -fno-omit-frame-pointer' cmake -DCMAKE_BUILD_TYPE=Debug ../src  || exit 1
make -j $(nproc) || exit 1

cd ../.. || exit 1
./pbench_evaluate perf.script
