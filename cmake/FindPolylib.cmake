
set(POLYLIB_DEFINITIONS)

find_path(POLYLIB_INCLUDE_DIR barvinok/polylib.h)
mark_as_advanced(POLYLIB_INCLUDE_DIR)
set(POLYLIB_INCLUDE_DIRS ${POLYLIB_INCLUDE_DIR})

find_library(POLYLIB_LIBRARY NAMES polylib)
mark_as_advanced(POLYLIB_LIBRARY)
set(NTL_LIBRARIES ${POLYLIB_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Polylib DEFAULT_MSG POLYLIB_LIBRARY POLYLIB_INCLUDE_DIR)
