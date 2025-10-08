#pragma once

constexpr int _BF_LOGICAL_FPS       = 30;
constexpr int _BF_LOGICAL_FPS_SCALE = 4;

constexpr int FIXED_FPS = _BF_LOGICAL_FPS * _BF_LOGICAL_FPS_SCALE;
constexpr f32 FIXED_DT  = 1.0f / (f32)FIXED_FPS;

// If player's PC gives <= _BF_MIN_TARGET_FPS FPS,
// then engine would skip simulation frames. Game would run slower.
constexpr int _BF_MIN_TARGET_FPS = 20;
