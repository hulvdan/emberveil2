#pragma once

constexpr int _BF_LOGICAL_FPS       = 30;
constexpr int _BF_LOGICAL_FPS_SCALE = 4;

constexpr int FIXED_FPS = _BF_LOGICAL_FPS * _BF_LOGICAL_FPS_SCALE;
constexpr f32 FIXED_DT  = 1.0f / (f32)FIXED_FPS;
