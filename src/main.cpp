// #include <windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/c99/bgfx.h>

#if defined(SDL_PLATFORM_EMSCRIPTEN)
#  include <emscripten.h>
#endif

#include "bf_lib.cpp"
#include "bf_game.cpp"

#define LOGI(...) SDL_Log(__VA_ARGS__)
#define LOGW(...) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#define LOGE(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

bool g_shouldExit = false;

void Update() {
  static int counter = 0;

  bgfx_set_view_rect(0, 0, 0, uint16_t(1280), uint16_t(720));
  bgfx_touch(0);
  Draw();
  bgfx_dbg_text_clear(0, false);
  bgfx_dbg_text_printf(0, 1, 0x4f, "Counter: %d", counter++);
  bgfx_frame(false);

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      g_shouldExit = true;
      break;
    }
  }
}

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

int SDL_main(int argc, char* argv[]) {
  SDL_Init(0);

  // #if defined(SDL_PLATFORM_WIN32)
  //   SDL_SetHint(SDL_HINT_RENDER_DRIVER, "direct3d11");
  // #endif

  auto window = SDL_CreateWindow(
    "The Game"
#if BF_DEBUG
    " [DEBUG]"
#endif
    ,
    1280,
    720,
    0
  );

  auto renderer = SDL_CreateRenderer(window, -1, 0);

  {
    bgfx_platform_data_t pd{};

#if defined(SDL_PLATFORM_WIN32)
    pd.nwh = SDL_GetPointerProperty(
      SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL
    );
    pd.ndt = NULL;
#elif defined(SDL_PLATFORM_MACOS)
    pd.nwh = SDL_GetPointerProperty(
      SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL
    );
    pd.ndt = NULL;
#elif defined(SDL_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
      pd.ndt = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL
      );
      pd.nwh = (Window)SDL_GetNumberProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0
      );
    }
    else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
      pd.ndt = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL
      );
      pd.nwh = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL
      );
    }
#elif defined(SDL_PLATFORM_EMSCRIPTEN)
    pd.nwh = (void*)"#canvas";
    pd.ndt = NULL;
#endif

    pd.context      = NULL;
    pd.backBuffer   = NULL;
    pd.backBufferDS = NULL;
    bgfx_set_platform_data(&pd);

    bgfx_init_t init{};
    // #if defined(SDL_PLATFORM_EMSCRIPTEN)
    init.type = BGFX_RENDERER_TYPE_OPENGL;
    // #else
    // init.type = BGFX_RENDERER_TYPE_DIRECT3D11;
    // #endif
    init.vendorId          = BGFX_PCI_ID_NONE;
    init.platformData.nwh  = pd.nwh;
    init.platformData.ndt  = pd.ndt;
    init.resolution.width  = 1280;
    init.resolution.height = 720;
    init.resolution.reset  = BGFX_RESET_VSYNC;
    if (!bgfx_init(&init)) {
      LOGE("bgfx_init(init) failed!");
      return 1;
    }

    // bgfx_reset( 1280, 720, BGFX_RESET_VSYNC );
    bgfx_set_debug(BGFX_DEBUG_TEXT);
    // bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030FF, 1.0f, 0);
    bgfx_set_view_clear(0, BGFX_CLEAR_COLOR, 0x303030FF, 1.0f, 0);
  }

#if defined(SDL_PLATFORM_EMSCRIPTEN)
  log_webgl_version();
  emscripten_set_main_loop(Update, 0, 1);
#else
  while (!g_shouldExit)
    Update();

  bgfx_shutdown();

  SDL_DestroyWindow(window);
  SDL_Quit();
#endif

  return 0;
}

///
