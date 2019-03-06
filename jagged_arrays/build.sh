#!/bin/sh

. ./config.sh
${CXX_COMPILER} ${CXX_FLAGS} -std=c++11 -o exe main.cpp
