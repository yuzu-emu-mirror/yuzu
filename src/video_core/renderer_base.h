// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "common/assert.h"
#include "common/common_types.h"

class EmuWindow;

class RendererBase : NonCopyable {
public:
    /// Used to reference a framebuffer
    enum kFramebuffer { kFramebuffer_VirtualXFB = 0, kFramebuffer_EFB, kFramebuffer_Texture };

    /**
     * Struct describing framebuffer metadata
     * TODO(bunnei): This struct belongs in the GPU code, but we don't have a good place for it yet.
     */
    struct FramebufferInfo {
        enum class PixelFormat : u32 {
            ABGR8 = 1,
        };

        /**
         * Returns the number of bytes per pixel.
         */
        static u32 BytesPerPixel(PixelFormat format) {
            switch (format) {
            case PixelFormat::ABGR8:
                return 4;
            }

            UNREACHABLE();
        }

        VAddr address;
        u32 offset;
        u32 width;
        u32 height;
        u32 stride;
        PixelFormat pixel_format;
    };

    virtual ~RendererBase() {}

    /// Swap buffers (render frame)
    virtual void SwapBuffers(const FramebufferInfo& framebuffer_info) = 0;

    /**
     * Set the emulator window to use for renderer
     * @param window EmuWindow handle to emulator window to use for rendering
     */
    virtual void SetWindow(EmuWindow* window) = 0;

    /// Initialize the renderer
    virtual bool Init() = 0;

    /// Shutdown the renderer
    virtual void ShutDown() = 0;

    // Getter/setter functions:
    // ------------------------

    f32 GetCurrentFPS() const {
        return m_current_fps;
    }

    int GetCurrentFrame() const {
        return m_current_frame;
    }

    void RefreshRasterizerSetting();

protected:
    f32 m_current_fps = 0.0f; ///< Current framerate, should be set by the renderer
    int m_current_frame = 0;  ///< Current frame, should be set by the renderer

private:
    bool opengl_rasterizer_active = false;
};
