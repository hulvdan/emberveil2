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

#include "box2d/box2d.h"

#include "bf_constants.cpp"

b2Vec2 ToB2Vec2(Vector2 value) {  ///
  return {value.x, value.y};
}

Vector2 ToVector2(b2Vec2 value) {  ///
  return {value.x, value.y};
}

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

enum ControlsContext {  ///
  ControlsContext_INVALID,
  ControlsContext_COUNT,
};

struct GameData {
  struct Meta {
    Vector2 screenSizeUI       = {};
    Vector2 screenSizeUIMargin = {};

    bool playerUsesKeyboardOrController = false;
  } meta;
} g = {};

void GameLoad(const BFSave::Save* save) {  ///
}

void GameDumpStateForSaving(BFSave::SaveT& save) {  ///
}

void GamePreInit(GamePreInitOpts opts) {  ///
  ZoneScoped;

  ge.meta.logicRand = Random(SDL_GetPerformanceCounter());
}

void GameInit() {  ///
  ZoneScoped;
}

void GameInitAfterLoadingSavedata() {  ///
  ZoneScoped;

  LOGI("GameInitAfterLoadingSavedata...");
  DEFER {
    LOGI("GameInitAfterLoadingSavedata... Finished!");
  };
}

// NOTE: Logic must be executed only when `ge.meta._drawing` (`draw`) is false!
// e.g. updating mouse position, processing `clicked()`,
// logically reacting to `Clay_Hovered()`, changing game's state, etc.
void DoUI() {
#define BF_UI_PRE
#include "engine/bf_clay_ui.cpp"

#define BF_UI_POST
#include "engine/bf_clay_ui.cpp"
}

void GameFixedUpdate() {  ///
  ZoneScoped;

  DoUI();
}

void GameDraw() {
  ZoneScoped;

  DoUI();

  FlushDrawCommands();

  // Debug info.
  if (ge.meta.debugEnabled) {  ///
    if (0)
      IM::ShowDemoWindow();

    auto localizationEn = glib->localizations()->Get(1)->strings();

    if (IM::Begin("Debug") && IM::BeginTabBar("tabs")) {
      if (IM::BeginTabItem("info")) {
        IM::Text("Close debug menu: hold F1 -> press F2");

        IM::Text("");

        if (IM::Button("Reset Debug"))
          gdebug = {};

        IM::Checkbox("Gizmos", &gdebug.gizmos);
        IM::Checkbox("Emulating Mobile", &gdebug.emulatingMobile);
        IM::Checkbox("Hide UI For Video", &gdebug.hideUIForVideo);
        ge.meta.device
          = (gdebug.emulatingMobile ? DeviceType_MOBILE : DeviceType_DESKTOP);

        IM::Text("MarkGameplay: %d", ge.meta.markGameplay);

        IM::Text("F3 change localization");
        IM::Text("F4 change device");

        LAMBDA (void, debugTextArena, (const char* name, const Arena& arena)) {
          IM::Text(
            "%s: %d %d (%.1f%%) (max: %d, %.1f%%)",
            name,
            (int)arena.used,
            (int)arena.size,
            100.0f * (f32)arena.used / (f32)arena.size,
            (int)arena.maxUsed,
            100.0f * (f32)arena.maxUsed / (f32)arena.size
          );
        };

        debugTextArena("ge.meta._arena", ge.meta._arena);
        debugTextArena("ge.meta.trashArena", ge.meta.trashArena);
        debugTextArena("ge.meta._transientDataArena", ge.meta._transientDataArena);

        IM::EndTabItem();
      }

      IM::EndTabBar();
    }

    IM::End();
  }
}

///
