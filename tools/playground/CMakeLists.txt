
add_molly_executable(playground playground.cpp)

target_link_libraries( playground 
#  LLVMMolly
  Islpp
  LLVMSupport
)

#target_link_libraries( playground ${LIBGCC_LIBRARY} )
target_link_libraries( playground ${ISL_LIBRARY} )
target_link_libraries( playground ${GMP_LIBRARY} )
