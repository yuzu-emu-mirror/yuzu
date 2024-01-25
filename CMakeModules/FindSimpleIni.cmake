# SPDX-FileCopyrightText: 2023 Alexandre Bouvier <contact@amb.tf>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_path(SimpleIni_INCLUDE_DIR NAMES SimpleIni.h
    PATHS
    /usr/include
    /usr/include/simpleini
    )

find_library(SimpleIni_LIBRARIES NAMES simpleini
    PATHS
    /usr/lib
    /usr/local/lib
    )

include(FindPackageHandleStandardArgs)
if(SimpleIni_LIBRARIES)
    find_package_handle_standard_args(SimpleIni
        REQUIRED_VARS SimpleIni_INCLUDE_DIR SimpleIni_LIBRARIES
    )
else()
    find_package_handle_standard_args(SimpleIni
        REQUIRED_VARS SimpleIni_INCLUDE_DIR
    )
endif()

if (SimpleIni_FOUND AND NOT TARGET SimpleIni::SimpleIni)
    add_library(SimpleIni::SimpleIni INTERFACE IMPORTED)
    set_target_properties(SimpleIni::SimpleIni PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SimpleIni_INCLUDE_DIR}"
    )
    if(SimpleIni_LIBRARIES)
        set_target_properties(SimpleIni::SimpleIni PROPERTIES
            INTERFACE_LINK_LIBRARIES "${SimpleIni_LIBRARIES}"
            IMPORTED_LOCATION "${SimpleIni_LIBRARIES}"
        )
    endif()
endif()

mark_as_advanced(SimpleIni_INCLUDE_DIR)
