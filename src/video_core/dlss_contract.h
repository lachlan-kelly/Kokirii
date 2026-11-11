// SPDX-FileCopyrightText: Copyright 2026 Kokirii Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include "video_core/surface.h"

namespace VideoCommon::DlssContract {

// Validation helpers for the temporal-upscale (FSR/DLSS replacement) input and output images.
constexpr bool ValidColorFormats(VideoCore::Surface::PixelFormat input,
                                  VideoCore::Surface::PixelFormat output, bool use_history) {
    using VideoCore::Surface::PixelFormat;
    return (input == PixelFormat::B10G11R11_FLOAT && output == input) ||
           (!use_history && input == PixelFormat::A8B8G8R8_SRGB &&
            output == PixelFormat::R16G16B16A16_FLOAT);
}

constexpr bool FitsExtent(std::array<std::uint32_t, 2> requested,
                           std::array<std::uint32_t, 2> available) {
    return requested[0] <= available[0] && requested[1] <= available[1];
}

inline bool ValidGeometry(std::array<std::uint32_t, 2> render,
                          std::array<std::uint32_t, 2> output,
                          std::array<float, 2> jitter, std::array<float, 2> motion_scale) {
    return render[0] >= 2 && render[1] >= 2 && FitsExtent(render, output) &&
           std::isfinite(jitter[0]) && std::isfinite(jitter[1]) &&
           std::isfinite(motion_scale[0]) && std::isfinite(motion_scale[1]);
}

} // namespace VideoCommon::DlssContract
