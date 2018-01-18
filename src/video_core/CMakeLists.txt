add_library(video_core STATIC
    renderer_base.cpp
    renderer_base.h
    renderer_opengl/gl_resource_manager.h
    renderer_opengl/gl_shader_util.cpp
    renderer_opengl/gl_shader_util.h
    renderer_opengl/gl_state.cpp
    renderer_opengl/gl_state.h
    renderer_opengl/renderer_opengl.cpp
    renderer_opengl/renderer_opengl.h
    utils.h
    video_core.cpp
    video_core.h
)

create_target_directory_groups(video_core)

target_link_libraries(video_core PUBLIC common core)
target_link_libraries(video_core PRIVATE glad)
