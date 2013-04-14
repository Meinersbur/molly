
set(BARVINOK_DEFINITIONS)

find_path(BARVINOK_INCLUDE_DIR barvinok/barvinok.h)
mark_as_advanced(BARVINOK_INCLUDE_DIR)
set(BARVINOK_INCLUDE_DIRS ${BARVINOK_INCLUDE_DIR})

find_library(BARVINOK_LIBRARY NAMES barvinok)
mark_as_advanced(BARVINOK_LIBRARY)
set(BARVINOK_LIBRARIES ${BARVINOK_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Barvinok DEFAULT_MSG BARVINOK_LIBRARY BARVINOK_INCLUDE_DIR)
