// John Carmack on Inlined Code.
// http://number-none.com/blow/blog/programming/2014/09/26/carmack-on-inlined-code.html
//
// «The real enemy addressed by inlining is unexpected dependency and mutation of state.»
//
// If you are going to make a lot of state changes, having them all happen inline does
// have advantages; you should be made constantly aware of the full horror of what you are
// doing. When it gets to be too much to take, figure out how to factor blocks out into
// pure functions.
//
// To sum up:
//
// * If a function is only called from a single place, consider inlining it.
//
// * If a function is called from multiple places, see if it is possible to arrange
//   for the work to be done in a single place, perhaps with flags, and inline that.
//
// * If there are multiple versions of a function,
//   consider making a single function with more, possibly defaulted, parameters.
//
// * If the work is close to purely functional, with few references to global state,
//   try to make it completely functional.
//
// * Try to use const on both parameters and functions when the function
//   really must be used in multiple places.
//
// * Minimize control flow complexity and "area under ifs", favoring consistent
//   execution paths and times over "optimally" avoiding unnecessary work.

#pragma once

const char* GetWindowTitle() {  ///
  return "The Game"
#if BF_DEBUG
         " [DEBUG]"
#endif
#if BF_PROFILING && defined(DOCTEST_CONFIG_DISABLE)
         " [PROFILING]"
#endif
#if BF_ENABLE_ASSERTS
         " [ASSERTS]"
#endif
    ;
}

// g_gameVersion { ///
static const char* const g_gameVersion = BF_VERSION
#if BF_DEBUG
  " [DEBUG]"
#endif
#if BF_PROFILING && defined(DOCTEST_CONFIG_DISABLE)
  " [PROFILING]"
#endif
#if BF_ENABLE_ASSERTS
  " [ASSERTS]"
#endif
  ;
// }

struct GameData {
  struct Meta {
  } meta;
};

void GamePreInit() {  ///
}

void GameInit() {  ///
}

void GameFixedUpdate() {  ///
}

void GameDraw() {
  // Drawing debug text.
  if (ge.meta.debugEnabled) {  ///
    int y = 1;
    LAMBDA (void, DebugText, (const char* pattern, auto&&... args)) {
      bgfx::dbgTextPrintf(1, y++, 0x4f, pattern, args...);
    };

    DebugText("Close debug menu: hold F1 -> press F2");

    LAMBDA (void, debugTextArena, (const char* name, const Arena& arena)) {
      DebugText(
        "%s: %d %d (%.1f%%) (max: %d, %.1f%%)",
        name,
        arena.used,
        arena.size,
        100.0f * (f32)arena.used / (f32)arena.size,
        arena.maxUsed,
        100.0f * (f32)arena.maxUsed / (f32)arena.size
      );
    };

    debugTextArena("ge.meta._arena", ge.meta._arena);
  }
}

///
