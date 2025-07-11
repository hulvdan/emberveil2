#pragma once

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>
#include "glm/glm.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if defined(SDL_PLATFORM_EMSCRIPTEN)
#  include <emscripten.h>
#endif

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/bf_gamelib_generated.h"

#include "bf_lib.cpp"

#include "shaders/quad_fs_100_es.bin"
#include "shaders/quad_vs_100_es.bin"

#include "engine/bf_engine.cpp"
#include "game/bf_game.cpp"

struct EngineAppState {
  SDL_Window* window = {};
};

#if defined(SDL_PLATFORM_EMSCRIPTEN)
EM_JS(void, log_webgl_version, (), {
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

class BGFXCallbackHandler : public bgfx::CallbackI {
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
  void     profilerEnd() override {}
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

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
#if defined(SDL_PLATFORM_EMSCRIPTEN)
  log_webgl_version();
#endif

  auto appstate_ = new EngineAppState();
  *appstate      = (void*)appstate_;

  if (!SDL_Init(0)) {
    LOGE("SDL_Init failed!");
    return SDL_APP_FAILURE;
  }

  auto window = SDL_CreateWindow(GetWindowTitle(), 1280, 720, 0);
  if (!window) {
    LOGE("SDL_CreateWindow failed!");
    return SDL_APP_FAILURE;
  }
  appstate_->window = window;

  {
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

    LOCAL_PERSIST BGFXCallbackHandler bgfxCallbacks{};
    init.callback = &bgfxCallbacks;

    init.type              = bgfx::RendererType::OpenGL;
    init.vendorId          = BGFX_PCI_ID_NONE;
    init.platformData      = pd;
    init.resolution.width  = 1280;
    init.resolution.height = 720;
    // init.resolution.reset  = BGFX_RESET_VSYNC;
    if (!bgfx::init(init)) {
      LOGE("bgfx::init(init) failed!");
      exit(EXIT_FAILURE);
    }

    auto r1 = SDL_GetRenderer(window);
    auto n2 = SDL_GetRendererName(r1);

    // bgfx_reset( 1280, 720, BGFX_RESET_VSYNC );
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    // bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030FF, 1.0f, 0);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR, 0x303030FF, 1.0f, 0);
  }

  InitializeEngine();

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  static int counter = 0;

  bgfx::setViewRect(0, 0, 0, (u16)1280, (u16)720);
  bgfx::touch(0);

  switch (GameUpdate()) {
  case UpdateFunctionResult_SUCCESS:
    return SDL_APP_SUCCESS;
  case UpdateFunctionResult_FAILURE:
    return SDL_APP_FAILURE;
  }

  bgfx::dbgTextClear(0, false);
  bgfx::dbgTextPrintf(0, 1, 0x4f, "Counter: %d", counter++);
  bgfx::frame(false);

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
  switch (event->type) {
  case SDL_EVENT_QUIT:
  case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    return SDL_APP_SUCCESS;
  }

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
  bgfx::shutdown();

  if (appstate)
    delete appstate;

  auto appstate_ = (EngineAppState*)appstate;

  SDL_DestroyWindow(appstate_->window);
  SDL_Quit();
}

///
