
find_package(PkgConfig QUIET)
pkg_check_modules(PC_libzip QUIET libzip)

find_path(libzip_INCLUDE_DIR
  NAMES zip.h
  PATHS ${PC_libzip_INCLUDE_DIRS}
)
find_library(libzip_LIBRARY
  NAMES libzip
  PATHS ${PC_libzip_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libzip
  FOUND_VAR libzip_FOUND
  REQUIRED_VARS
    libzip_LIBRARY
    libzip_INCLUDE_DIR
  VERSION_VAR libzip_VERSION
)

if(libzip_FOUND)
  set(libzip_LIBRARIES ${libzip_LIBRARY})
  set(libzip_INCLUDE_DIRS ${libzip_INCLUDE_DIR})
  set(libzip_DEFINITIONS ${PC_libzip_CFLAGS_OTHER})
endif()

if(libzip_FOUND AND NOT TARGET libzip::libzip)
  add_library(libzip::libzip UNKNOWN IMPORTED)
  set_target_properties(libzip::libzip PROPERTIES
    IMPORTED_LOCATION "${libzip_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_libzip_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${libzip_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
    libzip_INCLUDE_DIR
    libzip_LIBRARY
)
