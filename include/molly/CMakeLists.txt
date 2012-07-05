if( MSVC_IDE OR XCODE )
  # Creates a dummy target containing all headers for the benefit of
  # Visual Studio users.
  file(GLOB_RECURSE headers *.h)
  add_library(molly_headers_do_not_build EXCLUDE_FROM_ALL
    # We need at least one source file:
    ${MOLLY_SOURCE_DIR}/lib/dummy.cpp
    ${headers})
endif()
