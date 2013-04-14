
find_package(MPI REQUIRED)

set(MOLLYCC_COMMON_FLAGS -std=c++11 -fms-extensions -D__mollycc__ -I${MOLLY_SOURCE_DIR}/include/mollyrt -mllvm -molly)
set(MOLLYCC_A_FLAGS $<TARGET_FILE:MollyRT>)
set(MOLLYCC_BC_FLAGS ${LLVM_BINARY_DIR}/lib/molly.bc)
set(MOLLYCC_LL_FLAGS ${LLVM_BINARY_DIR}/lib/molly.ll)
set(MOLLYCC_PLATFORM_FLAGS -target "i686-pc-mingw32" -fno-exceptions)


macro(mollyccll_compile outfile source)
  parse_arguments(PARM "FLAGS;DEPENDS;COMMENT" "" ${ARGN})
  get_filename_component(source_rpath ${source} REALPATH)
  add_custom_command(
    OUTPUT ${outfile}
    COMMAND mollycc ${MOLLYCC_COMMON_FLAGS} ${MOLLYCC_PLATFORM_FLAGS} ${PARM_FLAGS} ${source_rpath} ${LLVM_BINARY_DIR}/lib/molly.ll -o "${outfile}" ${MPI_C_LIBRARIES}
    DEPENDS mollycc ${LLVM_BINARY_DIR}/lib/molly.ll ${source_rpath}
#    IMPLICIT_DEPENDS ${source_rpath}
    COMMENT "${SOURCE_COMMENT}"
    VERBATIM)
endmacro()


if(0)
macro(mollycc_compile outfile source)
  parse_arguments(PARM "FLAGS;DEPENDS;COMMENT" "" ${ARGN})
  get_filename_component(source_rpath ${source} REALPATH)
  add_custom_command(
    OUTPUT ${outfile}
    COMMAND mollycc -D__mollycc__  -I${MOLLY_SOURCE_DIR}/include/mollyrt -L${MOLLY_BINARY_DIR}/lib -lMollyRT -mllvm -molly ${PARM_FLAGS} ${source_rpath} -o "${outfile}"  
    DEPENDS mollycc ${source_rpath} ${SOURCE_DEPENDS}
    IMPLICIT_DEPENDS ${source_rpath}
    COMMENT ${SOURCE_COMMENT} 
    VERBATIM)
endmacro()

macro(mollybc_compile outfilebc sourcebc)
  parse_arguments(SOURCEBC "CFLAGS;DEPENDS;COMMENT" "" ${ARGN})
  get_filename_component(sourcebc_rpath ${sourcebc} REALPATH)
#  message(STATUS "bc ${outfilebc}")
  add_custom_command(
    OUTPUT ${outfilebc}
    COMMAND mollycc -v -std=c++11 -O3 -g -fms-extensions -target "i686-pc-mingw32" -IC:/Users/Meinersbur/src/molly/molly/include/mollyrt -D__mollycc__ -mllvm -molly -o "${outfilebc}" ${LLVM_BINARY_DIR}/lib/molly.bc ${SOURCEBC_CFLAGS} ${sourcebc_rpath}
#    MAIN_DEPENDENCY ${sourcebc_rpath}
    DEPENDS mollycc ${SOURCEBC_DEPENDS}
    IMPLICIT_DEPENDS ${sourcebc_rpath}
    COMMENT ${SOURCEBC_COMMENT} 
    VERBATIM)
endmacro()
endif()

add_subdirectory(islpp)
add_subdirectory(stencil)
