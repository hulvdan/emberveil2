#include "basisu_transcoder.h"

#include "doctest.h"

#if !defined(SDL_PLATFORM_EMSCRIPTEN)
#  include "GameAnalytics/GameAnalytics.h"
#endif

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#if defined(SDL_PLATFORM_WIN32)  /**/ \
  || defined(SDL_PLATFORM_MACOS) /**/ \
  || defined(SDL_PLATFORM_LINUX)
#  define SDL_PLATFORM_DESKTOP
#endif

#include <bgfx/bgfx.h>
#include "glm/glm.hpp"

using Vector2    = glm::vec2;
using Vector3    = glm::vec3;
using Vector4    = glm::vec4;
using Vector2Int = glm::ivec2;
using Vector3Int = glm::ivec3;
using Vector4Int = glm::ivec4;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#if defined(SDL_PLATFORM_EMSCRIPTEN)
#  include <emscripten.h>
#endif

#include "miniaudio.h"

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/bf_gamelib_generated.h"

#include "shaders/quad_fs_100_es.bin"
#include "shaders/quad_vs_100_es.bin"
#include "shaders/quad_tex_fs_100_es.bin"
#include "shaders/quad_tex_vs_100_es.bin"

#define LOGI(...) SDL_Log(__VA_ARGS__)
#define LOGW(...) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#define LOGE(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

#include "bf_lib.cpp"

#if BF_PROFILING && defined(DOCTEST_CONFIG_DISABLE)
#  define TRACY_ONLY_LOCALHOST
#  define TRACY_NO_BROADCAST
#  include "tracy/Tracy.hpp"
#else
#  define ZoneScoped
#  define ZoneScopedN(_)
#  define FrameMark
#endif

#include "bf_version.cpp"
#include "hands/bf_codegen.cpp"
#include "engine/bf_engine_config.cpp"
#include "engine/bf_engine.cpp"
#include "game/bf_game.cpp"

struct EngineAppState {
  SDL_Window* window = {};
} g_appstate;

#if defined(SDL_PLATFORM_EMSCRIPTEN)
extern "C" {
#  ifdef BF_PLATFORM_WebYandex
EMSCRIPTEN_KEEPALIVE void mark_ysdk_loaded_from_js() {  ///
  ge.meta.ysdkLoaded = true;
}

EMSCRIPTEN_KEEPALIVE void pause_from_js() {  ///
  ge.meta.paused = true;
}

EMSCRIPTEN_KEEPALIVE void resume_from_js() {  ///
  ge.meta.paused = false;
}
#  endif

EMSCRIPTEN_KEEPALIVE void set_localization_from_js(int localization) {  ///
  ge.meta.localization = localization;
}

EMSCRIPTEN_KEEPALIVE void resize_from_js(int w, int h) {  ///
  if (g_appstate.window)
    SDL_SetWindowSize(g_appstate.window, w, h);
}

EMSCRIPTEN_KEEPALIVE void set_device_type_from_js(int type) {  ///
  ge.meta.deviceType = (DeviceType)type;
  LOGI("Set device %d", type);
}
}

EM_JS(void, js_TriggerOnResize, (), { onResize(); });

EM_JS(void, js_LogWebGLVersion, (), {  ///
  let canvas = document.createElement('canvas');
  let gl     = canvas.getContext('webgl2') || canvas.getContext('webgl');
  if (gl) {
    console.log("GL_VERSION: " + gl.getParameter(gl.VERSION));
  }
  else {
    console.log("No WebGL context available.");
  }
});
#endif

class BGFXCallbackHandler : public bgfx::CallbackI {  ///
  public:
  void fatal(const char* filePath, uint16_t line, bgfx::Fatal::Enum code, const char* str)
    override {
    LOGE("bgfx fatal [%s:%d]: %s\n", filePath, line, str);
    INVALID_PATH;
    exit(EXIT_FAILURE);
  }

  // Optional: override other methods if needed
  void traceVargs(
    const char* filePath,
    uint16_t    line,
    const char* format,
    va_list     argList
  ) override {}

  void profilerBegin(const char* name, uint32_t abgr, const char* filePath, uint16_t line)
    override {}

  void profilerBeginLiteral(
    const char* name,
    uint32_t    abgr,
    const char* filePath,
    uint16_t    line
  ) override {}

  void profilerEnd() override {}

  uint32_t cacheReadSize(uint64_t id) override {
    return 0;
  }

  bool cacheRead(uint64_t id, void* data, uint32_t size) override {
    return false;
  }

  void cacheWrite(uint64_t id, const void* data, uint32_t size) override {}

  void screenShot(
    const char* filePath,
    uint32_t    width,
    uint32_t    height,
    uint32_t    pitch,
    const void* data,
    uint32_t    size,
    bool        yflip
  ) override {}

  void captureBegin(
    uint32_t                  width,
    uint32_t                  height,
    uint32_t                  pitch,
    bgfx::TextureFormat::Enum format,
    bool                      yflip
  ) override {}

  void captureEnd() override {}

  void captureFrame(const void* data, uint32_t size) override {}
};

SDL_AppResult SDL_AppInit(void** /* appstate */, int argc, char** argv) {  ///
  ZoneScopedN("SDL_AppInit");

  GamePreInit();

#if defined(SDL_PLATFORM_EMSCRIPTEN)
  js_LogWebGLVersion();
#endif

  {
    ZoneScopedN("SDL. SDL_Init()");
    if (!SDL_Init(0)) {
      LOGE("SDL_Init failed!");
      return SDL_APP_FAILURE;
    }
  }

  SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;

  ge.meta.screenSize = LOGICAL_RESOLUTION;

  SDL_Window* window = nullptr;
  {
    ZoneScopedN("SDL. SDL_CreateWindow()");
    window = SDL_CreateWindow(
      GetWindowTitle(), ge.meta.screenSize.x, ge.meta.screenSize.y, flags
    );
    if (!window) {
      LOGE("SDL_CreateWindow failed!");
      return SDL_APP_FAILURE;
    }
    g_appstate.window = window;
  }

  {
    ZoneScopedN("bgfx. Initializing bgfx");
    bgfx::PlatformData pd{};

#if defined(SDL_PLATFORM_WIN32)
    pd.nwh = SDL_GetPointerProperty(
      SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr
    );
    pd.ndt = nullptr;
#elif defined(SDL_PLATFORM_MACOS)
    pd.nwh = SDL_GetPointerProperty(
      SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr
    );
    pd.ndt = nullptr;
#elif defined(SDL_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
      pd.ndt = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr
      );
      pd.nwh = (Window)SDL_GetNumberProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0
      );
    }
    else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
      pd.ndt = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr
      );
      pd.nwh = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr
      );
    }
#elif defined(SDL_PLATFORM_EMSCRIPTEN)
    pd.nwh = (void*)"#canvas";
    pd.ndt = nullptr;
#endif

    pd.context      = nullptr;
    pd.backBuffer   = nullptr;
    pd.backBufferDS = nullptr;

    bgfx::Init init{};
    for (auto disabled : ge.settings.bgfxDisabledCapabilities)
      init.capabilities &= ~disabled;

    static BGFXCallbackHandler bgfxCallbacks{};
    init.callback = &bgfxCallbacks;

    init.type              = bgfx::RendererType::OpenGL;
    init.vendorId          = BGFX_PCI_ID_NONE;
    init.platformData      = pd;
    init.resolution.width  = ge.meta.screenSize.x;
    init.resolution.height = ge.meta.screenSize.y;
    init.resolution.reset  = BGFX_RESET_VSYNC;

    {
      ZoneScopedN("bgfx. bgfx::init()");
      if (!bgfx::init(init)) {
        LOGE("bgfx::init(init) failed!");
        exit(EXIT_FAILURE);
      }
    }

    bgfx::setDebug(BGFX_DEBUG_TEXT);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR, 0x000000ff, 1.0f, 0);
  }

#if defined(SDL_PLATFORM_EMSCRIPTEN)
  js_TriggerOnResize();
#endif
  InitEngine();

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* /* appstate */) {  ///
  SDL_AppResult result{};

#ifdef BF_PLATFORM_WebYandex
  static bool pr1 = false;
  static bool pr2 = false;
  if (!pr1) {
    pr1 = true;
    LOGI("Waiting for yandex sdk to initialize...");
  }
  if (!ge.meta.ysdkLoaded)
    return SDL_APP_CONTINUE;
  if (!pr2) {
    pr2 = true;
    LOGI("Waiting for yandex sdk to initialize... Done!");
  }
#endif

  {
    ZoneScopedN("SDL. SDL_AppIterate()");

    bgfx::setViewRect(0, 0, 0, (u16)ge.meta.screenSize.x, (u16)ge.meta.screenSize.y);
    bgfx::touch(0);
    bgfx::setViewMode(0, bgfx::ViewMode::Sequential);

    EngineOnFrameStart();

    bgfx::dbgTextClear(0, false);

    {
      Vector2 size = (Vector2)LOGICAL_RESOLUTION;
      Vector2 pos{};
      if (ge.meta.screenToLogicalRatio > 1) {
        auto d = size.x * (ge.meta.screenToLogicalRatio - 1);
        size.x += d;
        pos.x -= d / 2;
      }
      else if (ge.meta.screenToLogicalRatio < 1) {
        auto d = size.y * (1.0f / ge.meta.screenToLogicalRatio - 1);
        size.y += d;
        pos.y -= d / 2;
      }

      // Background.
      RenderGroup_Begin(RenderZ_SCREEN_BACKGROUND);
      RenderGroup_SetSortY(0);
      {
        RenderGroup_CommandRect({
          .pos  = pos,
          .size = size,
          .anchor{},
          .color = ge.settings.backgroundColor,
        });
      }
      RenderGroup_End();

      // Fade.
      if (ge.settings.screenFade > 0) {
        RenderGroup_Begin(RenderZ_SCREEN_FADE);
        RenderGroup_SetSortY(0);
        {
          RenderGroup_CommandRect({
            .pos  = pos,
            .size = size,
            .anchor{},
            .color = Fade(ge.settings.screenFadeColor, ge.settings.screenFade),
          });
        }
        RenderGroup_End();
      }
    }

    result = EngineUpdate();
    if (result == SDL_APP_CONTINUE) {
      ZoneScopedN("bgfx. bgfx::frame()");
      bgfx::frame(false);
    }
  }

  FrameMark;
  return result;
}

SDL_AppResult SDL_AppEvent(void* /* appstate */, SDL_Event* event) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
  case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    return SDL_APP_SUCCESS;

  case SDL_EVENT_WINDOW_RESIZED: {  ///
    auto window = SDL_GetWindowFromID(event->window.windowID);

    auto width  = event->window.data1;
    auto height = event->window.data2;

    bgfx::reset(width, height, BGFX_RESET_VSYNC);
    ge.meta.screenSize = {width, height};
  } break;

  case SDL_EVENT_KEY_DOWN: {  ///
    switch (event->key.key) {
#if defined(SDL_PLATFORM_DESKTOP)
    case SDLK_F11: {
      auto window     = SDL_GetWindowFromID(event->key.windowID);
      auto mode       = SDL_GetWindowFullscreenMode(window);
      auto fullscreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
      SDL_SetWindowFullscreen(window, !fullscreen);
    } break;
#endif
    }
  } break;

  case SDL_EVENT_FINGER_DOWN: {  ///
    auto& t = event->tfinger;
    _OnTouchDown({
      ._id{._touchID = t.touchID, ._fingerID = t.fingerID},
      ._screenPos = Vector2{t.x, 1 - t.y} * (Vector2)ge.meta.screenSize,
    });
  } break;

  case SDL_EVENT_FINGER_UP: {  ///
    auto& t = event->tfinger;
    _OnTouchUp({
      ._id{._touchID = t.touchID, ._fingerID = t.fingerID},
      ._screenPos = Vector2{t.x, 1 - t.y} * (Vector2)ge.meta.screenSize,
    });
  } break;

  case SDL_EVENT_FINGER_MOTION: {  ///
    auto& t = event->tfinger;
    _OnTouchMoved({
      ._id{._touchID = t.touchID, ._fingerID = t.fingerID},
      ._screenPos  = Vector2{t.x, 1 - t.y} * (Vector2)ge.meta.screenSize,
      ._screenDPos = Vector2{t.dx, -t.dy} * (Vector2)ge.meta.screenSize,
    });
  } break;

  case SDL_EVENT_WINDOW_FOCUS_LOST: {
    // Required by yandex.
    // TODO: check if it works.
    ma_engine_set_volume(&ge.meta._soundManager.engine, 0);
  } break;

  case SDL_EVENT_WINDOW_FOCUS_GAINED: {
    // Required by yandex.
    // TODO: check if it works.
    ma_engine_set_volume(&ge.meta._soundManager.engine, 1);
  } break;
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* /* appstate */, SDL_AppResult result) {  ///
  DeinitEngine();

  bgfx::shutdown();

  SDL_DestroyWindow(g_appstate.window);
  SDL_Quit();
}

///
