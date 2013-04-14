
set(BARVINOK_DEFINITIONS)

find_library(LIBGCC_LIBRARY NAMES gcc)
mark_as_advanced(LIBGCC_LIBRARY)
set(LIBGCC_LIBRARIES ${LIBGCC_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libgcc DEFAULT_MSG LIBGCC_LIBRARY LIBGCC_INCLUDE_DIR)
