
set(NTL_DEFINITIONS)

find_path(NTL_INCLUDE_DIR NTL/mat_ZZ.h)
mark_as_advanced(NTL_INCLUDE_DIR)
set(NTL_INCLUDE_DIRS ${NTL_INCLUDE_DIR})

find_library(NTL_LIBRARY NAMES ntl)
mark_as_advanced(NTL_LIBRARY)
set(NTL_LIBRARIES ${NTL_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Ntl DEFAULT_MSG NTL_LIBRARY NTL_INCLUDE_DIR)
