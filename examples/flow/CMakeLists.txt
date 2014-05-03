
### Compile using standard compiler (without WITH_MOLLY being defined)
add_molly_example(flow flow.cpp)

add_custom_target(run-flow
   COMMAND "$<TARGET_FILE:flow>" 
   COMMENT "Running ref flow..."
)


### Compile using mollycc, link to MollyRT-a
get_filename_component(flow_a_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}flow-mollycc-a${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
mollycc_a_compile("${flow_a_path}"
  SOURCES "flow.cpp"
  FLAGS -mllvm -shape=2 -mllvm -polly-only-marked
  COMMENT "mollycc molly.a flow.cpp..."
)

add_custom_target(flow-mollycc-a
  DEPENDS "${flow_a_path}"
  COMMENT "Compiling flow using mollycc-a"
  VERBATIM
)
set_target_properties(flow-mollycc-a PROPERTIES FOLDER "Molly")

add_custom_target(run-flow-mollycc-a
  COMMAND "${MPIEXEC}" ${MPIEXEC_NUMPROC_FLAG} 2 ${MPIEXEC_PREFLAGS} "${flow_a_path}" ${MPIEXEC_POSTFLAGS} ""
  DEPENDS "${flow_a_path}"
  COMMENT "Running flow-mollycc-a..."
  VERBATIM
)
set_target_properties(run-flow-mollycc-a PROPERTIES FOLDER "Molly")




add_custom_target(flow-all)

#set (BGQ_MPIFLAGS="-I/bgsys/drivers/V1R2M1/ppc64/comm/lib/gnu -I/bgsys/drivers/V1R2M1/ppc64 -I/bgsys/drivers/V1R2M1/ppc64/comm/sys/include -I/bgsys/drivers/V1R2M1/ppc64/spi/include -I/bgsys/drivers/V1R2M1/ppc64/spi/include/kernel/cnk -L/bgsys/drivers/V1R2M1/ppc64/comm/lib -L/bgsys/drivers/V1R2M1/ppc64/comm/lib -L/bgsys/drivers/V1R2M1/ppc64/comm/lib64 -L/bgsys/drivers/V1R2M1/ppc64/comm/lib -L/bgsys/drivers/V1R2M1/ppc64/spi/lib -L/bgsys/drivers/V1R2M1/ppc64/comm/sys/lib -L/bgsys/drivers/V1R2M1/ppc64/spi/lib -L/bgsys/drivers/V1R2M1/ppc64/comm/sys/lib -L/bgsys/drivers/V1R2M1/ppc64/comm/lib64 -L/bgsys/drivers/V1R2M1/ppc64/comm/lib -L/bgsys/drivers/V1R2M1/ppc64/spi/lib -I/bgsys/drivers/V1R2M1/ppc64/comm/include -L/bgsys/drivers/V1R2M1/ppc64/comm/lib -lmpichcxx-gcc -Wl,-rpath -Wl,/bgsys/drivers/V1R2M1/ppc64/comm/sys/lib -Wl,-rpath -Wl,/bgsys/drivers/V1R2M1/ppc64/comm/lib -lmpich-gcc -lopa-gcc -lmpl-gcc -lpami-gcc -lSPI -lSPI_cnk -lrt -lpthread -lstdc++ -lpthread"
#set (BGQ_OPTFLAGS="-O3 -DNDEBUG -g"




function (add_example_config src volume proc block)
  get_filename_component(src_path "${src}" ABSOLUTE)

  get_filename_component(src_we "${src}" NAME_WE)
  set(target_name ${src_we}_L${volume}_P${proc}_B${block}-mollycc-a)
  get_filename_component(dst_path "${LLVM_EXAMPLES_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}${target_name}${CMAKE_EXECUTABLE_SUFFIX}" ABSOLUTE)
  string(REPLACE "x" ";" volume_list ${volume})
  string(REPLACE "x" ";" proc_list ${proc})
  string(REPLACE "x" ";" block_list ${block})
#  string(REGEX MATCHALL "([^\ ]+x|[^\ ]+$)" volume_list "${volume}")
#  string(REGEX MATCHALL "([^\ ]+x|[^\ ]+$)" proc_list "${proc}")
#  string(REGEX MATCHALL "([^\ ]+x|[^\ ]+$)" block_list "${block}")

  list(LENGTH proc_list procdims)
#  list(LENGTH volume_list dims)
#  if (dims==4)
#    set(dimnames "T X Y Z")
#  else ()
#    set(dimnames "X Y Z")
#  endif ()

  set(i 0)
  set(defines )
  foreach (_dimname IN LISTS dimnames)
    list(GET volume_list ${i} _l)
    list(APPEND defines "-DL${_dimname}=${_l}")
    list(GET block_list ${i} _p)
    list(APPEND defines "-DB${_dimname}=${_p}")

    if (i LESS ${procdims})
      list(GET proc_list ${i} _p)
    else ()
      set (_p 1)
    endif ()
    list(APPEND defines "-DP${_dimname}=${_p}")

    math(EXPR i "${i}+1")
  endforeach ()


  mollycc_a_compile(${dst_path}  
    SOURCES ${src}
    FLAGS -mllvm "-shape=${proc}" -mllvm -polly-only-marked ${defines}
  )

add_custom_target(${target_name}
  DEPENDS "${dst_path}"
  VERBATIM
)


add_custom_target(submit-${target_name}
  COMMAND "${MOLLY_SOURCE_DIR}/examples/submit.py" ${dst_path} ${src_path} --jobfile "${MOLLY_SOURCE_DIR}/examples/flow/flow.ll" 
)

endfunction ()


set(dimnames X Y)
add_example_config(flow.cpp 8x8 2x1 4x8)
add_example_config(flow.cpp 8x8 2x2 4x4)

