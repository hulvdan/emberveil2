#pragma once

constexpr Color SCREEN_OVERLAY_COLOR     = BLACK;
constexpr Color MODAL_OVERLAY_COLOR      = BLACK;
constexpr f32   MODAL_OVERLAY_COLOR_FADE = 0.75f;

constexpr f32 UI_ASPECT_RATIO_MIN = 1172 / 1026.0f;  // From yandex screenshot.
constexpr f32 UI_ASPECT_RATIO_MAX = 20 / 9.0f;

constexpr int MAX_DEPTH = 4;

constexpr auto ANIMATION_0_FRAMES = lframe::FromSeconds(0.2f);
constexpr auto ANIMATION_1_FRAMES = lframe::FromSeconds(0.5f);
constexpr auto ANIMATION_2_FRAMES = lframe::FromSeconds(1.2f);
constexpr auto ANIMATION_3_FRAMES = lframe::FromSeconds(3.2f);
constexpr auto ANIMATION_4_FRAMES = lframe::FromSeconds(4.5f);
constexpr auto ANIMATION_5_FRAMES = lframe::FromSeconds(7.0f);
