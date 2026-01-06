#pragma once

constexpr Color SCREEN_OVERLAY_COLOR     = BLACK;
constexpr Color MODAL_OVERLAY_COLOR      = BLACK;
constexpr f32   MODAL_OVERLAY_COLOR_FADE = 0.75f;

constexpr f32 UI_ASPECT_RATIO_MIN = 1172 / 1026.0f;  // From yandex screenshot.
constexpr f32 UI_ASPECT_RATIO_MAX = 20 / 9.0f;

constexpr f32 BODY_LINEAR_DAMPING = 5;
// constexpr f32 BODY_LINEAR_DAMPING_SPEED_SCALE = 7.0f / 12.78f * 10.0f;
// constexpr f32 BODY_LINEAR_DAMPING             = 10;
// constexpr f32 BODY_LINEAR_DAMPING_SPEED_SCALE = 7.0f / 6.78f * 10.0f;
// constexpr f32 BODY_LINEAR_DAMPING             = 20;
// constexpr f32 BODY_LINEAR_DAMPING_SPEED_SCALE = 7.0f / 3.29f * 10.0f;
// constexpr f32 BODY_LINEAR_DAMPING             = 30;
// constexpr f32 BODY_LINEAR_DAMPING_SPEED_SCALE = 7.0f / 2.13f * 10.0f;
// constexpr f32 BODY_LINEAR_DAMPING             = 40;
// constexpr f32 BODY_LINEAR_DAMPING_SPEED_SCALE = 7.0f / 1.55f * 10.0f;
// constexpr f32 BODY_LINEAR_DAMPING             = 50;
// constexpr f32 BODY_LINEAR_DAMPING_SPEED_SCALE = 7.0f / 1.2f * 10.0f;

constexpr Vector2 PLAYER_COLLIDER_SIZE{1.5f, 1.5f};

constexpr f32 METER_LOGICAL_SIZE = 80;
