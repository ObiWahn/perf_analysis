#!/bin/bash

# get 3rdParty libs
./init_libs

# create dir
mkdir -p build
cd build || exit 1

#configure / compile
CXXFLAGS='-Wall -Werror' cmake -DCMAKE_BUILD_TYPE=Release ../src  || exit 1
make -j $(nproc)
