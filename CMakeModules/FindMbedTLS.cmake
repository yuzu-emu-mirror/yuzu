# SPDX-FileCopyrightText: 2021 Andrea Pappacoda <andrea@pappacoda.it>
# SPDX-License-Identifier: GPL-2.0-or-later

# MbedTLS 3.0.0 will ship with a CMake package config file,
# see ARMmbed/mbedtls@d259e347e6e3a630acfc1a811709ca05e5d3b92e,
# so when yuzu will switch to that version this won't be required anymore.
#
# yuzu only uses mbedcrypto, searching for mbedtls and mbedx509 is not
# needed.

find_path(MbedTLS_INCLUDE_DIR mbedtls/cipher.h)

find_library(MbedTLS_LIBRARY mbedcrypto)

if (MbedTLS_INCLUDE_DIR AND MbedTLS_LIBRARY)
    # Check for CMAC support
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_LIBRARIES ${MbedTLS_LIBRARY})
    check_symbol_exists(mbedtls_cipher_cmac ${MbedTLS_INCLUDE_DIR}/mbedtls/cmac.h mbedcrypto_HAS_CMAC)
    unset(CMAKE_REQUIRED_LIBRARIES)

    # Check if version 2.x is available
    file(READ "${MbedTLS_INCLUDE_DIR}/mbedtls/version.h" MbedTLS_VERSION_FILE)
    string(REGEX MATCH "#define[ ]+MBEDTLS_VERSION_STRING[ ]+\"([0-9.]+)\"" _ ${MbedTLS_VERSION_FILE})
    set(MbedTLS_VERSION "${CMAKE_MATCH_1}")

    if (NOT TARGET MbedTLS::mbedcrypto)
        add_library(MbedTLS::mbedcrypto UNKNOWN IMPORTED GLOBAL)
        set_target_properties(MbedTLS::mbedcrypto PROPERTIES
            IMPORTED_LOCATION "${MbedTLS_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MbedTLS_INCLUDE_DIR}"
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MbedTLS
    REQUIRED_VARS
        MbedTLS_LIBRARY
        MbedTLS_INCLUDE_DIR
        mbedcrypto_HAS_CMAC
    VERSION_VAR MbedTLS_VERSION
)
