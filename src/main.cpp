// #include <windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#if defined(SDL_PLATFORM_EMSCRIPTEN)
#  include <emscripten.h>
#endif

bool g_shouldExit = false;

void Update() {
  static int counter = 0;

  bgfx::setViewRect(0, 0, 0, uint16_t(1280), uint16_t(720));
  bgfx::touch(0);
  bgfx::dbgTextClear();
  bgfx::dbgTextPrintf(0, 1, 0x4f, "Counter: %d", counter++);
  bgfx::frame();

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

int SDL_main(int argc, char* argv[]) {
  SDL_Init(0);

  auto window = SDL_CreateWindow("Bgfx SDL3", 1280, 720, 0);

  {
    bgfx::PlatformData pd{};

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
      // struct wl_display *display*
      pd.ndt = SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL
      );
      // struct wl_surface *surface*
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
    bgfx::setPlatformData(pd);

    bgfx::Init init{};
    init.type              = bgfx::RendererType::OpenGL;
    init.vendorId          = BGFX_PCI_ID_NONE;
    init.platformData.nwh  = pd.nwh;
    init.platformData.ndt  = pd.ndt;
    init.resolution.width  = 1280;
    init.resolution.height = 720;
    // init.resolution.reset = BGFX_RESET_VSYNC;
    if (!bgfx::init(init)) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "bgfx::init(init) failed!");
      return 1;
    }

    // bgfx::reset( 1280, 720, BGFX_RESET_VSYNC );
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030FF, 1.0f, 0);
  }

#if defined(SDL_PLATFORM_EMSCRIPTEN)
  emscripten_set_main_loop(Update, 0, 1);
#else
  while (!shouldExit)
    Update();

  bgfx::shutdown();

  SDL_DestroyWindow(window);
  SDL_Quit();
#endif

  return 0;
}
