
set(MOLLY_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MOLLY_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(POLLY_SOURCE_DIR ${MOLLY_SOURCE_DIR}/../polly) #TODO: Make Polly dir configurable
set(POLLY_BINARY_DIR ${MOLLY_BINARY_DIR}/../polly) #TODO: Make Polly dir configurable

# Add path for custom modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${MOLLY_SOURCE_DIR}/cmake" "${POLLY_SOURCE_DIR}/cmake")


FIND_PACKAGE(Gmp REQUIRED)
INCLUDE_DIRECTORIES( ${GMP_INCLUDE_DIR} )

FIND_PACKAGE(Isl REQUIRED)
INCLUDE_DIRECTORIES( ${ISL_INCLUDE_DIR} )

#FIND_PACKAGE(Cloog REQUIRED)
#INCLUDE_DIRECTORIES( ${CLOOG_INCLUDE_DIR} )

#FIND_PACKAGE(Barvinok REQUIRED)
#INCLUDE_DIRECTORIES( ${BARVINOK_INCLUDE_DIR} )

#FIND_PACKAGE(Polylib REQUIRED)
#INCLUDE_DIRECTORIES( ${POLYLIB_INCLUDE_DIR} )

#FIND_PACKAGE(Ntl REQUIRED)
#INCLUDE_DIRECTORIES( ${NTL_INCLUDE_DIR} )

FIND_PACKAGE(Libgcc REQUIRED)

include_directories(
  ${MOLLY_SOURCE_DIR}/include
  ${MOLLY_SOURCE_DIR}/../clang/include  #TODO: Make Clang dir configurable
  ${MOLLY_BINARY_DIR}/../clang/include  #TODO: Make Clang dir configurable
  ${POLLY_SOURCE_DIR}/include  
  ${POLLY_BINARY_DIR}/include 
)

if (CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif (CMAKE_COMPILER_IS_GNUCXX)


macro(add_molly_executable name)
  add_llvm_executable( ${name} ${ARGN} )
  set_target_properties(${name} PROPERTIES FOLDER "Molly")
endmacro(add_molly_executable)

macro(add_molly_library name)
  llvm_process_sources(srcs ${ARGN})
  if(MSVC_IDE OR XCODE)
    # Add public headers
    file( GLOB headers
      ${MOLLY_SOURCE_DIR}/include/molly/*.h
      ${MOLLY_SOURCE_DIR}/lib/*.h
    )
    set_source_files_properties(${headers} PROPERTIES HEADER_FILE_ONLY ON)
    set(srcs ${srcs} ${headers})
  endif(MSVC_IDE OR XCODE)
  
  add_llvm_library( ${name} ${srcs} )
  set_target_properties(${name} PROPERTIES FOLDER "Molly")
endmacro(add_molly_library)

add_subdirectory(lib)
add_subdirectory(tools)
