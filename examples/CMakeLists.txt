
macro(mollycc_compile outfile source)
  parse_arguments(SOURCE "CFLAGS;DEPS" "" ${ARGN})
  get_filename_component(source_rpath ${source} REALPATH)
  add_custom_command(
    OUTPUT ${outfile}
    COMMAND mollycc ${SOURCE_CFLAGS} -std=c++11 -O3 -g -fms-extensions -target "i686-pc-mingw32" -IC:/Users/Meinersbur/src/molly/molly/include/mollyrt -D__mollycc__ -mllvm -molly -c -o "${outfile}" ${source_rpath}
    MAIN_DEPENDENCY ${source}
    DEPENDS mollycc ${MOLLY_SOURCE_DIR}/include/mollyrt/molly.h ${SOURCE_DEPS})
endmacro()


add_subdirectory(islpp)
add_subdirectory(stencil)
