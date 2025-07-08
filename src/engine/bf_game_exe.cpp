#pragma once

#include <memory>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/c99/bgfx.h>

#include "bf_lib.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#if defined(SDL_PLATFORM_EMSCRIPTEN)
#  include <emscripten.h>
#endif

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

void bgfx_fatal(
  bgfx_callback_interface_t* _this,
  const char*                filePath,
  uint16_t                   line,
  bgfx_fatal_t               code,
  const char*                str
) {
  LOGE("bgfx fatal [%s:%d]: %s\n", filePath, line, str);
  INVALID_PATH;
  exit(EXIT_FAILURE);
}

void trace_vargs_impl(
  bgfx_callback_interface_t* _this,
  const char*                _filePath,
  uint16_t                   _line,
  const char*                _format,
  va_list                    _argList
) {}

void profiler_begin_impl(
  bgfx_callback_interface_t* _this,
  const char*                _name,
  uint32_t                   _abgr,
  const char*                _filePath,
  uint16_t                   _line
) {}

void profiler_begin_literal_impl(
  bgfx_callback_interface_t* _this,
  const char*                _name,
  uint32_t                   _abgr,
  const char*                _filePath,
  uint16_t                   _line
) {}

void profiler_end_impl(bgfx_callback_interface_t* _this) {}

uint32_t cache_read_size_impl(bgfx_callback_interface_t* _this, uint64_t _id) {
  return 0;
}

bool cache_read_impl(
  bgfx_callback_interface_t* _this,
  uint64_t                   _id,
  void*                      _data,
  uint32_t                   _size
) {
  return false;
}

void cache_write_impl(
  bgfx_callback_interface_t* _this,
  uint64_t                   _id,
  const void*                _data,
  uint32_t                   _size
) {}

void screen_shot_impl(
  bgfx_callback_interface_t* _this,
  const char*                _filePath,
  uint32_t                   _width,
  uint32_t                   _height,
  uint32_t                   _pitch,
  const void*                _data,
  uint32_t                   _size,
  bool                       _yflip
) {}

void capture_begin_impl(
  bgfx_callback_interface_t* _this,
  uint32_t                   _width,
  uint32_t                   _height,
  uint32_t                   _pitch,
  bgfx_texture_format_t      _format,
  bool                       _yflip
) {}

void capture_end_impl(bgfx_callback_interface_t* _this) {}

void capture_frame_impl(
  bgfx_callback_interface_t* _this,
  const void*                _data,
  uint32_t                   _size
) {}

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
    bgfx_platform_data_t pd{};

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
    bgfx_set_platform_data(&pd);

    auto r = SDL_GetRenderer(window);
    auto n = SDL_GetRendererName(r);

    // for (int i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
    //   SDL_RendererInfo info{};
    //   if (SDL_GetRenderDriverInfo(i, &info) == 0) {
    //     printf("  %d: %s\n", i, info.name);
    //   }
    // }

    bgfx_init_t init{};

    // bgfx_callback_vtbl_t table{
    //   .fatal                  = bgfx_fatal,
    //   .trace_vargs            = trace_vargs_impl,
    //   .profiler_begin         = profiler_begin_impl,
    //   .profiler_begin_literal = profiler_begin_literal_impl,
    //   .profiler_end           = profiler_end_impl,
    //   .cache_read_size        = cache_read_size_impl,
    //   .cache_read             = cache_read_impl,
    //   .cache_write            = cache_write_impl,
    //   .screen_shot            = screen_shot_impl,
    //   .capture_begin          = capture_begin_impl,
    //   .capture_end            = capture_end_impl,
    //   .capture_frame          = capture_frame_impl,
    // };
    // bgfx_callback_interface_t tableInterface{.vtbl = &table};
    // init.callback = &tableInterface;
    init.type              = BGFX_RENDERER_TYPE_OPENGL;
    init.vendorId          = BGFX_PCI_ID_NONE;
    init.platformData.nwh  = pd.nwh;
    init.platformData.ndt  = pd.ndt;
    init.resolution.width  = 1280;
    init.resolution.height = 720;
    // init.resolution.reset  = BGFX_RESET_VSYNC;
    if (!bgfx_init(&init)) {
      LOGE("bgfx_init(init) failed!");
      exit(EXIT_FAILURE);
    }

    auto r1 = SDL_GetRenderer(window);
    auto n2 = SDL_GetRendererName(r1);

    // bgfx_reset( 1280, 720, BGFX_RESET_VSYNC );
    bgfx_set_debug(BGFX_DEBUG_TEXT);
    // bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030FF, 1.0f, 0);
    bgfx_set_view_clear(0, BGFX_CLEAR_COLOR, 0x303030FF, 1.0f, 0);
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
  static int counter = 0;

  bgfx_set_view_rect(0, 0, 0, uint16_t(1280), uint16_t(720));
  bgfx_touch(0);
  GameUpdate();
  bgfx_dbg_text_clear(0, false);
  bgfx_dbg_text_printf(0, 1, 0x4f, "Counter: %d", counter++);
  bgfx_frame(false);

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
  bgfx_shutdown();

  if (appstate)
    delete appstate;

  auto appstate_ = (EngineAppState*)appstate;

  SDL_DestroyWindow(appstate_->window);
  SDL_Quit();
}

///
