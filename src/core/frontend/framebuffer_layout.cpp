// Copyright 2018 Yuzu Emulator Team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "core/frontend/framebuffer_layout.h"

namespace Layout {

// Finds the largest size subrectangle contained in window area that is confined to the aspect ratio
template <class T>
static MathUtil::Rectangle<T> maxRectangle(MathUtil::Rectangle<T> window_area,
                                           float screen_aspect_ratio) {
    float scale = std::min(static_cast<float>(window_area.GetWidth()),
                           window_area.GetHeight() / screen_aspect_ratio);
    return MathUtil::Rectangle<T>{0, 0, static_cast<T>(std::round(scale)),
                                  static_cast<T>(std::round(scale * screen_aspect_ratio))};
}

FramebufferLayout DefaultFrameLayout(unsigned width, unsigned height) {
    ASSERT(width > 0);
    ASSERT(height > 0);
    // The drawing code needs at least somewhat valid values for both screens
    // so just calculate them both even if the other isn't showing.
    FramebufferLayout res{width, height};

    const float emulation_aspect_ratio{static_cast<float>(ScreenUndocked::Height) /
                                       ScreenUndocked::Width};
    MathUtil::Rectangle<unsigned> screen_window_area{0, 0, width, height};
    MathUtil::Rectangle<unsigned> screen = maxRectangle(screen_window_area, emulation_aspect_ratio);

    float window_aspect_ratio = static_cast<float>(height) / width;

    if (window_aspect_ratio < emulation_aspect_ratio) {
        screen = screen.TranslateX((screen_window_area.GetWidth() - screen.GetWidth()) / 2);
    } else {
        screen = screen.TranslateY((height - screen.GetHeight()) / 2);
    }
    res.screen = screen;
    return res;
}

} // namespace Layout
