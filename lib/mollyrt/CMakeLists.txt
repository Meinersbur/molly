
#set(LLVM_NO_RTTI 1)

find_package(MPI REQUIRED)
include_directories(${MPI_C_INCLUDE_PATH})
include_directories(${MPI_CXX_INCLUDE_PATH})
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MPI_C_COMPILE_FLAGS}")
#set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} ${MPI_C_LINK_FLAGS}")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS}")
#set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} ${MPI_CXX_LINK_FLAGS}")


### Standard binary Library
add_library(MollyRT molly.cpp ${MOLLY_SOURCE_DIR}/include/mollyrt/molly.h comm.cpp)
set_target_properties(MollyRT PROPERTIES FOLDER "Molly")
target_link_libraries(MollyRT ${MPI_CXX_LIBRARIES}) 


### Bitcode library (bc)
include(LLVMParseArguments)

macro(clang_compile object_file source)
  parse_arguments(SOURCE "CFLAGS;DEPS" "" ${ARGN})
  get_filename_component(source_rpath ${source} REALPATH)
  add_custom_command(
    OUTPUT ${object_file}
    COMMAND clang ${SOURCE_CFLAGS} -c -o "${object_file}" ${source_rpath}
    DEPENDS clang ${source} ${SOURCE_DEPS}
    COMMENT "Compiling ${source} using clang")
endmacro()


clang_compile(molly.bc molly.cpp
  CFLAGS -emit-llvm -c -target i686-pc-mingw32 -std=gnu++11 -fms-extensions -D__mollycc__ -IC:/Users/Meinersbur/src/molly/molly/include -I${MPI_CXX_INCLUDE_PATH}
  DEPS ${MOLLY_SOURCE_DIR}/include/mollyrt/molly.h)

add_custom_command(OUTPUT ${LLVM_BINARY_DIR}/lib/molly.bc
  DEPENDS molly.bc
  COMMAND ${CMAKE_COMMAND} -E copy_if_different molly.bc ${LLVM_BINARY_DIR}/lib/molly.bc
  COMMENT "Copying molly.bc...")

add_custom_target(MollyRT-bc ALL 
  DEPENDS ${LLVM_BINARY_DIR}/lib/molly.bc molly.ll MollyRT 
  COMMENT "Compiling Molly bitcode...")
set_target_properties(MollyRT-bc PROPERTIES FOLDER "Molly")

install(FILES molly.bc
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION lib${LLVM_LIBDIR_SUFFIX})


### LLVM IR library (ll)
clang_compile(${LLVM_BINARY_DIR}/lib/molly.ll molly.cpp 
  CFLAGS -emit-llvm -S -target i686-pc-mingw32 -std=gnu++11 -fms-extensions -D__mollycc__ -IC:/Users/Meinersbur/src/molly/molly/include -I${MPI_CXX_INCLUDE_PATH} 
  DEPS ${MOLLY_SOURCE_DIR}/include/mollyrt/molly.h)

add_custom_target(MollyRT-ll ALL
  DEPENDS ${LLVM_BINARY_DIR}/lib/molly.ll
  COMMENT "Built Molly Runtime Library (ll)...")
set_target_properties(MollyRT-ll PROPERTIES FOLDER "Molly")

install(FILES ${LLVM_BINARY_DIR}/lib/molly.ll
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DESTINATION lib${LLVM_LIBDIR_SUFFIX})

