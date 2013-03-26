
set DRIVER_ARGS=-v
set CLANG_ARGS=-O3 -g -fno-exceptions -fno-cxx-exceptions -fno-use-cxa-atexit -IC:\Users\Meinersbur\src\molly\molly\include\mollyrt -target "i686-pc-win32"
set LLVM_ARGS=-mllvm -debug-pass=Details
set POLLY_ARGS=-mllvm -polly-report
set MOLLY_ARGS=-mllvm -molly -mllvm -shape=4 

clang %CD%/stencil.cpp %DRIVER_ARGS% %CLANG_ARGS% -S -emit-llvm -o %CD%/stencil.ll   2> clangArgs.txt

mollycc %LLVM_ARGS% %CD%/stencil.cpp -target "i686-pc-win32" -o stencil.exe -O3 -v  -mllvm -debug  -fno-exceptions -fno-cxx-exceptions -fno-use-cxa-atexit %POLLY_ARGS% %MOLLY_ARGS%   2> mollyArgs.txt

rem Using mingw
rem molly-incrlink mollytest.cpp -o mollytest.exe -target "i686-pc-mingw32" -std=c++11 -g -O3 -v -mllvm -molly -mllvm -shape=4 

rem $ "C:/MinGW/bin/gcc.exe" -std=c++11 -O3 -v -L/c/Users/Meinersbur/src/molly/build64_vc11/lib/Debug/ -lMollyRT mollytest.cpp -I/c/Users/Meinersbur/src/molly/molly/include/mollyrt/

mollycc  %CD%/stencil.c -target "i686-pc-win32" -o phitest.exe -O3 -v  -mllvm -debug  -fno-exceptions -fno-cxx-exceptions -fno-use-cxa-atexit    -mllvm -polly -mllvm -polly-report  2> pollyArgs.txt

clang %DRIVER_ARGS% %CLANG_ARGS% %LLVM_ARGS% %CD%/hello.c  -o hello.exe 2> llvmArgs.txt

mollycc %LLVM_ARGS% %CD%/stencil.cpp -target "i686-pc-mingw32" -o stencil.exe -O3 -v  -mllvm -debug  -fno-exceptions -fno-cxx-exceptions -fno-use-cxa-atexit -std=c++11 %POLLY_ARGS% %MOLLY_ARGS%   2> mingwArgs.txt
