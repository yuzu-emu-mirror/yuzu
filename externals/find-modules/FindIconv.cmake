
find_package(PkgConfig QUIET)
pkg_check_modules(PC_iconv QUIET iconv)

find_path(iconv_INCLUDE_DIR
  NAMES iconv.h
  PATHS ${PC_iconv_INCLUDE_DIRS} ${CONAN_INCLUDE_DIRS_LIBICONV}
)
find_library(iconv_LIBRARY
  NAMES iconv
  PATHS ${PC_iconv_LIBRARY_DIRS} ${CONAN_LIB_DIRS_LIBICONV}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(iconv
  FOUND_VAR iconv_FOUND
  REQUIRED_VARS
    iconv_LIBRARY
    iconv_INCLUDE_DIR
  VERSION_VAR iconv_VERSION
)

if(iconv_FOUND)
  set(iconv_LIBRARIES ${iconv_LIBRARY})
  set(iconv_INCLUDE_DIRS ${iconv_INCLUDE_DIR})
  set(iconv_DEFINITIONS ${PC_iconv_CFLAGS_OTHER})
endif()

if(iconv_FOUND AND NOT TARGET iconv::iconv)
  add_library(iconv::iconv UNKNOWN IMPORTED)
  set_target_properties(iconv::iconv PROPERTIES
    IMPORTED_LOCATION "${iconv_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_iconv_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${iconv_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
    iconv_INCLUDE_DIR
    iconv_LIBRARY
)
