
add_subdirectory(driver)
add_subdirectory(playground)
if ( MSVC_IDE )
  add_subdirectory(molly-incrlink)
endif ( MSVC_IDE )
