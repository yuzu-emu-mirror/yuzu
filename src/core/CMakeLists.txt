# SPDX-FileCopyrightText: 2018 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(core STATIC
    arm/arm_interface.h
    arm/arm_interface.cpp
    arm/exclusive_monitor.cpp
    arm/exclusive_monitor.h
    arm/symbols.cpp
    arm/symbols.h
    constants.cpp
    constants.h
    core.cpp
    core.h
    core_timing.cpp
    core_timing.h
    cpu_manager.cpp
    cpu_manager.h
    crypto/aes_util.cpp
    crypto/aes_util.h
    crypto/encryption_layer.cpp
    crypto/encryption_layer.h
    crypto/key_manager.cpp
    crypto/key_manager.h
    crypto/partition_data_manager.cpp
    crypto/partition_data_manager.h
    crypto/ctr_encryption_layer.cpp
    crypto/ctr_encryption_layer.h
    crypto/xts_encryption_layer.cpp
    crypto/xts_encryption_layer.h
    debugger/debugger_interface.h
    debugger/debugger.cpp
    debugger/debugger.h
    debugger/gdbstub_arch.cpp
    debugger/gdbstub_arch.h
    debugger/gdbstub.cpp
    debugger/gdbstub.h
    device_memory.cpp
    device_memory.h
    file_sys/fssystem/fs_i_storage.h
    file_sys/fssystem/fssystem_aes_ctr_counter_extended_storage.cpp
    file_sys/fssystem/fssystem_aes_ctr_counter_extended_storage.h
    file_sys/fssystem/fssystem_aes_ctr_storage.cpp
    file_sys/fssystem/fssystem_aes_ctr_storage.h
    file_sys/fssystem/fssystem_aes_xts_storage.cpp
    file_sys/fssystem/fssystem_aes_xts_storage.h
    file_sys/fssystem/fssystem_alignment_matching_storage.h
    file_sys/fssystem/fssystem_alignment_matching_storage_impl.cpp
    file_sys/fssystem/fssystem_alignment_matching_storage_impl.h
    file_sys/fssystem/fssystem_bucket_tree.cpp
    file_sys/fssystem/fssystem_bucket_tree.h
    file_sys/fssystem/fssystem_bucket_tree_utils.h
    file_sys/fssystem/fssystem_compressed_storage.h
    file_sys/fssystem/fssystem_compression_common.h
    file_sys/fssystem/fssystem_compression_configuration.cpp
    file_sys/fssystem/fssystem_compression_configuration.h
    file_sys/fssystem/fssystem_crypto_configuration.cpp
    file_sys/fssystem/fssystem_crypto_configuration.h
    file_sys/fssystem/fssystem_hierarchical_integrity_verification_storage.cpp
    file_sys/fssystem/fssystem_hierarchical_integrity_verification_storage.h
    file_sys/fssystem/fssystem_hierarchical_sha256_storage.cpp
    file_sys/fssystem/fssystem_hierarchical_sha256_storage.h
    file_sys/fssystem/fssystem_indirect_storage.cpp
    file_sys/fssystem/fssystem_indirect_storage.h
    file_sys/fssystem/fssystem_integrity_romfs_storage.cpp
    file_sys/fssystem/fssystem_integrity_romfs_storage.h
    file_sys/fssystem/fssystem_integrity_verification_storage.cpp
    file_sys/fssystem/fssystem_integrity_verification_storage.h
    file_sys/fssystem/fssystem_memory_resource_buffer_hold_storage.h
    file_sys/fssystem/fssystem_nca_file_system_driver.cpp
    file_sys/fssystem/fssystem_nca_file_system_driver.h
    file_sys/fssystem/fssystem_nca_header.cpp
    file_sys/fssystem/fssystem_nca_header.h
    file_sys/fssystem/fssystem_nca_reader.cpp
    file_sys/fssystem/fssystem_pooled_buffer.cpp
    file_sys/fssystem/fssystem_pooled_buffer.h
    file_sys/fssystem/fssystem_sparse_storage.cpp
    file_sys/fssystem/fssystem_sparse_storage.h
    file_sys/fssystem/fssystem_switch_storage.h
    file_sys/fssystem/fssystem_utility.cpp
    file_sys/fssystem/fssystem_utility.h
    file_sys/fssystem/fs_types.h
    file_sys/bis_factory.cpp
    file_sys/bis_factory.h
    file_sys/card_image.cpp
    file_sys/card_image.h
    file_sys/common_funcs.h
    file_sys/content_archive.cpp
    file_sys/content_archive.h
    file_sys/control_metadata.cpp
    file_sys/control_metadata.h
    file_sys/directory.h
    file_sys/errors.h
    file_sys/fsmitm_romfsbuild.cpp
    file_sys/fsmitm_romfsbuild.h
    file_sys/ips_layer.cpp
    file_sys/ips_layer.h
    file_sys/kernel_executable.cpp
    file_sys/kernel_executable.h
    file_sys/mode.h
    file_sys/nca_metadata.cpp
    file_sys/nca_metadata.h
    file_sys/partition_filesystem.cpp
    file_sys/partition_filesystem.h
    file_sys/patch_manager.cpp
    file_sys/patch_manager.h
    file_sys/program_metadata.cpp
    file_sys/program_metadata.h
    file_sys/registered_cache.cpp
    file_sys/registered_cache.h
    file_sys/romfs.cpp
    file_sys/romfs.h
    file_sys/romfs_factory.cpp
    file_sys/romfs_factory.h
    file_sys/savedata_factory.cpp
    file_sys/savedata_factory.h
    file_sys/sdmc_factory.cpp
    file_sys/sdmc_factory.h
    file_sys/submission_package.cpp
    file_sys/submission_package.h
    file_sys/system_archive/data/font_chinese_simplified.cpp
    file_sys/system_archive/data/font_chinese_simplified.h
    file_sys/system_archive/data/font_chinese_traditional.cpp
    file_sys/system_archive/data/font_chinese_traditional.h
    file_sys/system_archive/data/font_extended_chinese_simplified.cpp
    file_sys/system_archive/data/font_extended_chinese_simplified.h
    file_sys/system_archive/data/font_korean.cpp
    file_sys/system_archive/data/font_korean.h
    file_sys/system_archive/data/font_nintendo_extended.cpp
    file_sys/system_archive/data/font_nintendo_extended.h
    file_sys/system_archive/data/font_standard.cpp
    file_sys/system_archive/data/font_standard.h
    file_sys/system_archive/mii_model.cpp
    file_sys/system_archive/mii_model.h
    file_sys/system_archive/ng_word.cpp
    file_sys/system_archive/ng_word.h
    file_sys/system_archive/shared_font.cpp
    file_sys/system_archive/shared_font.h
    file_sys/system_archive/system_archive.cpp
    file_sys/system_archive/system_archive.h
    file_sys/system_archive/system_version.cpp
    file_sys/system_archive/system_version.h
    file_sys/system_archive/time_zone_binary.cpp
    file_sys/system_archive/time_zone_binary.h
    file_sys/vfs.cpp
    file_sys/vfs.h
    file_sys/vfs_cached.cpp
    file_sys/vfs_cached.h
    file_sys/vfs_concat.cpp
    file_sys/vfs_concat.h
    file_sys/vfs_layered.cpp
    file_sys/vfs_layered.h
    file_sys/vfs_offset.cpp
    file_sys/vfs_offset.h
    file_sys/vfs_real.cpp
    file_sys/vfs_real.h
    file_sys/vfs_static.h
    file_sys/vfs_types.h
    file_sys/vfs_vector.cpp
    file_sys/vfs_vector.h
    file_sys/xts_archive.cpp
    file_sys/xts_archive.h
    frontend/applets/cabinet.cpp
    frontend/applets/cabinet.h
    frontend/applets/controller.cpp
    frontend/applets/controller.h
    frontend/applets/error.cpp
    frontend/applets/error.h
    frontend/applets/general_frontend.cpp
    frontend/applets/general_frontend.h
    frontend/applets/mii_edit.cpp
    frontend/applets/mii_edit.h
    frontend/applets/profile_select.cpp
    frontend/applets/profile_select.h
    frontend/applets/software_keyboard.cpp
    frontend/applets/software_keyboard.h
    frontend/applets/web_browser.cpp
    frontend/applets/web_browser.h
    frontend/emu_window.cpp
    frontend/emu_window.h
    frontend/framebuffer_layout.cpp
    frontend/framebuffer_layout.h
    frontend/graphics_context.h
    hid/emulated_console.cpp
    hid/emulated_console.h
    hid/emulated_controller.cpp
    hid/emulated_controller.h
    hid/emulated_devices.cpp
    hid/emulated_devices.h
    hid/hid_core.cpp
    hid/hid_core.h
    hid/hid_types.h
    hid/input_converter.cpp
    hid/input_converter.h
    hid/input_interpreter.cpp
    hid/input_interpreter.h
    hid/irs_types.h
    hid/motion_input.cpp
    hid/motion_input.h
    api_version.h
    ipc.h
    result.h
    internal_network/network.cpp
    internal_network/network.h
    internal_network/network_interface.cpp
    internal_network/network_interface.h
    internal_network/sockets.h
    internal_network/socket_proxy.cpp
    internal_network/socket_proxy.h
    loader/deconstructed_rom_directory.cpp
    loader/deconstructed_rom_directory.h
    loader/kip.cpp
    loader/kip.h
    loader/loader.cpp
    loader/loader.h
    loader/nax.cpp
    loader/nax.h
    loader/nca.cpp
    loader/nca.h
    loader/nro.cpp
    loader/nro.h
    loader/nso.cpp
    loader/nso.h
    loader/nsp.cpp
    loader/nsp.h
    loader/xci.cpp
    loader/xci.h
    memory/cheat_engine.cpp
    memory/cheat_engine.h
    memory/dmnt_cheat_types.h
    memory/dmnt_cheat_vm.cpp
    memory/dmnt_cheat_vm.h
    memory.cpp
    memory.h
    perf_stats.cpp
    perf_stats.h
    precompiled_headers.h
    reporter.cpp
    reporter.h
    telemetry_session.cpp
    telemetry_session.h
    tools/freezer.cpp
    tools/freezer.h
    tools/renderdoc.cpp
    tools/renderdoc.h
)

if (MSVC)
    target_compile_options(core PRIVATE
        /we4242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )
else()
    target_compile_options(core PRIVATE
        -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )
endif()

create_target_directory_groups(core)

target_link_libraries(core PUBLIC common PRIVATE audio_core kernel network service video_core nx_tzdb)
target_link_libraries(core PUBLIC Boost::headers PRIVATE fmt::fmt nlohmann_json::nlohmann_json mbedtls RenderDoc::API)

if (MINGW)
    target_link_libraries(core PRIVATE ${MSWSOCK_LIBRARY})
endif()

if (ENABLE_WEB_SERVICE)
    target_compile_definitions(core PRIVATE -DENABLE_WEB_SERVICE)
    target_link_libraries(core PRIVATE web_service)
endif()

if (ARCHITECTURE_x86_64 OR ARCHITECTURE_arm64)
    target_sources(core PRIVATE
        arm/dynarmic/arm_dynarmic.h
        arm/dynarmic/arm_dynarmic_64.cpp
        arm/dynarmic/arm_dynarmic_64.h
        arm/dynarmic/arm_dynarmic_32.cpp
        arm/dynarmic/arm_dynarmic_32.h
        arm/dynarmic/dynarmic_cp15.cpp
        arm/dynarmic/dynarmic_cp15.h
        arm/dynarmic/dynarmic_exclusive_monitor.cpp
        arm/dynarmic/dynarmic_exclusive_monitor.h
    )
    target_link_libraries(core PRIVATE dynarmic::dynarmic)
endif()

if (YUZU_USE_PRECOMPILED_HEADERS)
    target_precompile_headers(core PRIVATE precompiled_headers.h)
endif()

if (YUZU_ENABLE_LTO)
  set_property(TARGET core PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
