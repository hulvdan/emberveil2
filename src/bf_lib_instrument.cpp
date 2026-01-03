#pragma once

// !banner: GAME
//  ██████╗  █████╗ ███╗   ███╗███████╗
// ██╔════╝ ██╔══██╗████╗ ████║██╔════╝
// ██║  ███╗███████║██╔████╔██║█████╗
// ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝
// ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗
//  ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝

#define BF_DISABLE_AUDIO (0)

constexpr int _BF_LOGICAL_FPS       = 30;
constexpr int _BF_LOGICAL_FPS_SCALE = 4;

constexpr int FIXED_FPS = _BF_LOGICAL_FPS * _BF_LOGICAL_FPS_SCALE;
constexpr f32 FIXED_DT  = 1.0f / (f32)FIXED_FPS;

// If player's PC gives <= _BF_MIN_TARGET_FPS FPS,
// then engine would skip simulation frames. Game would run slower.
constexpr int _BF_MIN_TARGET_FPS = 20;

constexpr int BF_MAX_FONT_ATLAS_SIZE = 4096;

constexpr int BF_MAX_SOUNDS = 200;

constexpr f32 BF_SOUND_VOLUME_BOOST_FROM_DB            = -6;
constexpr f32 BF_SOUND_VOLUME_BOOST_TO_DB              = -3;
constexpr int BF_SOUND_VOLUME_BOOST_STEPS              = 3;
constexpr int BF_SOUND_VOLUME_BOOST_MAX_LATENCY_FRAMES = 2;

// !banner: other
//  ██████╗ ████████╗██╗  ██╗███████╗██████╗
// ██╔═══██╗╚══██╔══╝██║  ██║██╔════╝██╔══██╗
// ██║   ██║   ██║   ███████║█████╗  ██████╔╝
// ██║   ██║   ██║   ██╔══██║██╔══╝  ██╔══██╗
// ╚██████╔╝   ██║   ██║  ██║███████╗██║  ██║
//  ╚═════╝    ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝

#if defined(SDL_PLATFORM_WIN32)
#  define BF_PROFILING (BF_DEBUG && 0)
#elif defined(BF_PLATFORM_Web)
#  define BF_PROFILING (1)
#else
#  define BF_PROFILING (0)
#endif

#define BF_ENABLE_ASSERTS (1 && BF_DEBUG)
#define BF_ENABLE_SLOW_ASSERTS (1 && BF_ENABLE_ASSERTS)

#ifdef BF_PLATFORM_WebYandex
#  define BF_SHOW_VERSION (0)
#else
#  define BF_SHOW_VERSION (BF_RELEASE)
#endif

#define BF_DEBUG_VIGNETTE_AND_STRIPS (0)

// !banner: UNMAPPING
// ██╗   ██╗███╗   ██╗███╗   ███╗ █████╗ ██████╗ ██████╗ ██╗███╗   ██╗ ██████╗
// ██║   ██║████╗  ██║████╗ ████║██╔══██╗██╔══██╗██╔══██╗██║████╗  ██║██╔════╝
// ██║   ██║██╔██╗ ██║██╔████╔██║███████║██████╔╝██████╔╝██║██╔██╗ ██║██║  ███╗
// ██║   ██║██║╚██╗██║██║╚██╔╝██║██╔══██║██╔═══╝ ██╔═══╝ ██║██║╚██╗██║██║   ██║
// ╚██████╔╝██║ ╚████║██║ ╚═╝ ██║██║  ██║██║     ██║     ██║██║ ╚████║╚██████╔╝
//  ╚═════╝ ╚═╝  ╚═══╝╚═╝     ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝     ╚═╝╚═╝  ╚═══╝ ╚═════╝

// !banner: ALLOCATOR
//  █████╗ ██╗     ██╗      ██████╗  ██████╗ █████╗ ████████╗ ██████╗ ██████╗
// ██╔══██╗██║     ██║     ██╔═══██╗██╔════╝██╔══██╗╚══██╔══╝██╔═══██╗██╔══██╗
// ███████║██║     ██║     ██║   ██║██║     ███████║   ██║   ██║   ██║██████╔╝
// ██╔══██║██║     ██║     ██║   ██║██║     ██╔══██║   ██║   ██║   ██║██╔══██╗
// ██║  ██║███████╗███████╗╚██████╔╝╚██████╗██║  ██║   ██║   ╚██████╔╝██║  ██║
// ╚═╝  ╚═╝╚══════╝╚══════╝ ╚═════╝  ╚═════╝╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝

// Set to 1 to enable testing buffer overruns (Windows only).
#if 1 && BF_DEBUG
#  define BF_ALLOC unmapped_alloc
#  define BF_FREE unmapped_free
#else
#  define BF_ALLOC malloc
#  define BF_FREE free
#endif

// To test buffer overruns on right side (eg `char buffer[10]; x = buffer[10]`), set to 1.
// To test on left side (eg buffer[-1]), set to 0.
#define UNMAPPING_ALLOCATOR_ERROR_ON_RIGHT 1

#define UNMAPPING_ALLOCATOR_PAGES_MARGIN 4
