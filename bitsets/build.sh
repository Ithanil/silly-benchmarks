#!/bin/sh

. ./config.sh
#${CXX_COMPILER} ${CXX_FLAGS} -std=c++14 -o exe main.cpp
${CXX_COMPILER} ${CXX_FLAGS} -std=c++14 -o test test.cpp
