set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

set(SRCS
            emu_window/emu_window_sdl2.cpp
            config.cpp
            yuzu.cpp
            yuzu.rc
            )
set(HEADERS
            emu_window/emu_window_sdl2.h
            config.h
            default_ini.h
            resource.h
            )

create_directory_groups(${SRCS} ${HEADERS})

add_executable(yuzu-cmd ${SRCS} ${HEADERS})
target_link_libraries(yuzu-cmd PRIVATE common core input_common)
target_link_libraries(yuzu-cmd PRIVATE inih glad)
if (MSVC)
    target_link_libraries(yuzu-cmd PRIVATE getopt)
endif()
target_link_libraries(yuzu-cmd PRIVATE ${PLATFORM_LIBRARIES} SDL2 Threads::Threads)

if(UNIX AND NOT APPLE)
    install(TARGETS yuzu-cmd RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
endif()

if (MSVC)
    include(CopyYuzuSDLDeps)
    include(CopyYuzuUnicornDeps)
    copy_yuzu_SDL_deps(yuzu-cmd)
    copy_yuzu_unicorn_deps(yuzu-cmd)
endif()
