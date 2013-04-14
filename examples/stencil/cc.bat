
set DRIVER_ARGS=-v
set CLANG_ARGS_WIN32=-std=c++11 -O3 -g -fno-exceptions -fno-cxx-exceptions -fno-use-cxa-atexit -IC:\Users\Meinersbur\src\molly\molly\include\mollyrt -target "i686-pc-win32"
set CLANG_ARGS_MINGW=-std=c++11 -O3 -g -IC:\Users\Meinersbur\src\molly\molly\include\mollyrt -target "i686-pc-mingw32"
set LLVM_ARGS=-mllvm -debug-pass=Details -mllvm -debug 
set POLLY_ARGS=-mllvm -polly-report
set MOLLY_ARGS=-mllvm -molly -mllvm -shape=4 -DNDEBUG -D__mollycc__ -fms-extensions -LC:\Users\Meinersbur\src\molly\build32_mingw\lib

clang %CD%/stencil.cpp %DRIVER_ARGS% %CLANG_ARGS_WIN32% -S -emit-llvm -o %CD%/stencil.ll   2> clangArgs.txt

mollycc %DRIVER_ARGS% %CLANG_ARGS_MINGW% %LLVM_ARGS% -mllvm -polly-report %MOLLY_ARGS% -g0 %CD%/stencil.cpp -o stencil.exe 2> mollyArgs.txt

rem Using mingw
rem molly-incrlink mollytest.cpp -o mollytest.exe -target "i686-pc-mingw32" -std=c++11 -g -O3 -v -mllvm -molly -mllvm -shape=4 

rem $ "C:/MinGW/bin/gcc.exe" -std=c++11 -O3 -v -L/c/Users/Meinersbur/src/molly/build64_vc11/lib/Debug/ -lMollyRT mollytest.cpp -I/c/Users/Meinersbur/src/molly/molly/include/mollyrt/


mollycc  %CD%/stencil.cpp -target "i686-pc-win32" -o phitest.exe -O3 -v  -mllvm -debug  -fno-exceptions -fno-cxx-exceptions -fno-use-cxa-atexit    -mllvm -polly -mllvm -polly-report  2> pollyArgs.txt

clang %DRIVER_ARGS% %CLANG_ARGS_WIN32% %LLVM_ARGS% %CD%/hello.c  -o hello.exe 2> llvmArgs.txt

mollycc %DRIVER_ARGS% %CD%/stencil.cpp -target "i686-pc-mingw32" -o stencil.exe -O3 -v -mllvm -debug  -fno-exceptions -fno-cxx-exceptions -fno-use-cxa-atexit -std=c++11 %LLVM_ARGS% %POLLY_ARGS% %MOLLY_ARGS%   2> mingwArgs.txt
