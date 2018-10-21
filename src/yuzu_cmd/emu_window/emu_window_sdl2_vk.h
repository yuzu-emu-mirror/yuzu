// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <vulkan/vulkan.h>
#include "core/frontend/emu_window.h"
#include "yuzu_cmd/emu_window/emu_window_sdl2.h"

class EmuWindow_SDL2_VK final : public EmuWindow_SDL2 {
public:
    explicit EmuWindow_SDL2_VK(bool fullscreen);
    ~EmuWindow_SDL2_VK();

    /// Swap buffers to display the next frame
    void SwapBuffers() override;

    /// Makes the graphics context current for the caller thread
    void MakeCurrent() override;

    /// Releases the GL context from the caller thread
    void DoneCurrent() override;

    /// Retrieves Vulkan specific handlers from the window
    void RetrieveVulkanHandlers(void** instance, void** surface) const override;

private:
    /// Vulkan instance
    VkInstance instance{};

    /// Vulkan surface
    VkSurfaceKHR surface{};

    /// Vulkan debug callback
    VkDebugReportCallbackEXT debug_report{};

    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr{};
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT{};
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT{};

    /// Enable Vulkan validations layers
    static constexpr bool enable_layers = true;
};
