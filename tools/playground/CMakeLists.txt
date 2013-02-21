
file( GLOB_RECURSE mollyfiles
  ${MOLLY_SOURCE_DIR}/lib/*.cpp
  ${MOLLY_SOURCE_DIR}/lib/islpp/*.cpp
)

file( GLOB_RECURSE headers
  ${MOLLY_SOURCE_DIR}/include/molly/*.h
  ${MOLLY_SOURCE_DIR}/include/islpp/*.h
  ${MOLLY_SOURCE_DIR}/lib/*.h
  ${MOLLY_SOURCE_DIR}/lib/islpp/*.h
)

add_molly_executable(playground playground.cpp ${mollyfiles} ${headers})

target_link_libraries( playground 
#  LLVMMolly
  LLVMSupport
)
