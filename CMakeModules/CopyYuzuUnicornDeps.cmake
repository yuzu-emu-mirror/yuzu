function(copy_yuzu_unicorn_deps target_dir)
    include(WindowsCopyFiles)
    set(DLL_DEST "${CMAKE_BINARY_DIR}/bin/")
    windows_copy_files(${target_dir} ${UNICORN_DLL_DIR} ${DLL_DEST}
        libgcc_s_seh-1.dll
        libwinpthread-1.dll
        unicorn.dll
    )
endfunction(copy_yuzu_unicorn_deps)
