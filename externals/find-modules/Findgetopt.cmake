
find_package(PkgConfig QUIET)
pkg_check_modules(PC_getopt QUIET getopt)

find_path(getopt_INCLUDE_DIR
  NAMES getopt.h
  PATHS ${PC_getopt_INCLUDE_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(getopt
  FOUND_VAR getopt_FOUND
  REQUIRED_VARS
    getopt_INCLUDE_DIR
  VERSION_VAR getopt_VERSION
)

if(getopt_FOUND)
  set(getopt_INCLUDE_DIRS ${getopt_INCLUDE_DIR})
  set(getopt_DEFINITIONS ${PC_getopt_CFLAGS_OTHER})
endif()

if(getopt_FOUND AND NOT TARGET getopt::getopt)
  add_library(getopt::getopt UNKNOWN IMPORTED)
  set_target_properties(getopt::getopt PROPERTIES
    INTERFACE_COMPILE_OPTIONS "${PC_getopt_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${getopt_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
    getopt_INCLUDE_DIR
)
