#! /bin/bash

CC=${HOME}/bin/mpi/bgclang/bin/mpiclang++11
CXXFLAGS="-fno-exceptions -O3 -g -DNDEBUG -std=gnu++11 -D__mollycc__ -c  -I../../include/mollyrt -Wno-error=gnu-array-member-paren-init"

${CC} ${CXXFLAGS} comm.cpp    -o comm.cpp.o 
${CC} ${CXXFLAGS} molly.cpp   -o molly.cpp.o 
${CC} ${CXXFLAGS} combuf.cpp  -o combuf.cpp.o 
${CC} ${CXXFLAGS} debug.cpp   -o debug.cpp.o 



ar cr molly.a molly.cpp.o comm.cpp.o debug.cpp.o combuf.cpp.o

