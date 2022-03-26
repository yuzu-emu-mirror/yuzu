# SPDX-FileCopyrightText: 2022 Andrea Pappacoda <andrea@pappacoda.it>
#
# SPDX-License-Identifier: GPL-2.0-or-later

# This find module looks for SoundTouch with both a CMake Config file and a
# pkg-config. If the library is found, it checks if it enforces 32 bit
# floating point samples, and if not it defines SOUNDTOUCH_INTEGER_SAMPLES,
# just like in the bundled library.

find_package(SoundTouch CONFIG)
if (SoundTouch_FOUND)
    set(_st_real_name "SoundTouch::SoundTouch")
else()
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_search_module(SoundTouch IMPORTED_TARGET soundtouch)
        if (SoundTouch_FOUND)
            set_target_properties(PkgConfig::SoundTouch PROPERTIES IMPORTED_GLOBAL True)
            add_library(SoundTouch::SoundTouch ALIAS PkgConfig::SoundTouch)
            # Need to set this variable because CMake doesn't allow to add
            # compile definitions to ALIAS targets
            set(_st_real_name "PkgConfig::SoundTouch")
        endif()
    endif()
endif()

if (SoundTouch_FOUND)
    find_path(_st_include_dir "soundtouch/soundtouch_config.h")
    file(READ "${_st_include_dir}/soundtouch/soundtouch_config.h" _st_config_file)

    # Check if the config file defines SOUNDTOUCH_FLOAT_SAMPLES
    string(REGEX MATCH "#define[ ]+SOUNDTOUCH_FLOAT_SAMPLES[ ]+1" SoundTouch_FLOAT_SAMPLES ${_st_config_file})

    if (NOT SoundTouch_FLOAT_SAMPLES)
        target_compile_definitions(${_st_real_name} INTERFACE "SOUNDTOUCH_INTEGER_SAMPLES=1")
        set(SoundTouch_INTEGER_SAMPLES True)
    else()
        # Check if SoundTouch supports SOUNDTOUCH_NO_CONFIG
        file(READ "${_st_include_dir}/soundtouch/STTypes.h" _st_types_file)

        string(FIND "${_st_types_file}" "SOUNDTOUCH_NO_CONFIG" SoundTouch_NO_CONFIG)

        # if found
        if (NOT SoundTouch_NO_CONFIG EQUAL "-1")
            target_compile_definitions(${_st_real_name} INTERFACE "SOUNDTOUCH_NO_CONFIG" "SOUNDTOUCH_INTEGER_SAMPLES=1")
            set(SoundTouch_INTEGER_SAMPLES True)
        endif()

        unset(_st_types_file)
    endif()

    unset(_st_real_name)
    unset(_st_include_dir)
    unset(_st_config_file)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SoundTouch
    REQUIRED_VARS
        SoundTouch_FOUND
        SoundTouch_INTEGER_SAMPLES
    VERSION_VAR SoundTouch_VERSION
)
