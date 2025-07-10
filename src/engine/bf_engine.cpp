#pragma once

#define LOGI(...) SDL_Log(__VA_ARGS__)
#define LOGW(...) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#define LOGE(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

struct Texture2D {
  int   w    = {};
  int   h    = {};
  void* data = {};

  bgfx_texture_handle_t tex = {};
};

struct EngineData {
  Texture2D      atlas   = {};
  gbFileContents gamelib = {};
} e = {};

///
Texture2D LoadTexture(const char* path) {
  Texture2D result{};

  int channels = 0;
  result.data  = stbi_load(path, &result.w, &result.h, &channels, 4);
  ASSERT(result.data);

  auto memory = bgfx_copy(result.data, result.w * result.h * 4);

  result.tex = bgfx_create_texture_2d(
    result.w,
    result.h,
    false /* _hasMips*/,
    1,
    BGFX_TEXTURE_FORMAT_RGBA8,
    BGFX_SAMPLER_MIN_POINT      //
      | BGFX_SAMPLER_MAG_POINT  //
      | BGFX_SAMPLER_MIP_POINT  //
      | BGFX_SAMPLER_U_CLAMP    //
      | BGFX_SAMPLER_V_CLAMP,
    memory
  );

  return result;
}

///
void UnloadTexture(Texture2D* texture) {
  ASSERT(texture->data);

  static_assert(sizeof(texture->tex) == sizeof(bgfx_texture_handle_t));
  bgfx_destroy_texture(*(bgfx_texture_handle_t*)&texture->tex);

  texture->tex = {};
  stbi_image_free(texture->data);
  texture->data = nullptr;
}

void InitializeEngine() {
  e.atlas   = LoadTexture("resources/atlas.png");
  e.gamelib = gb_file_read_contents(gb_heap_allocator(), 0, "resources/gamelib.bin");
  LOGI("Initialized engine!");
}

///
u64 GetTime() {
  return SDL_GetTicks();
}

enum UpdateFunctionResult {
  UpdateFunctionResult_NO_ERROR_CONTINUE_RUNNING = -1,
  UpdateFunctionResult_FINISHED_SUCCESSFULLY     = 0,
  UpdateFunctionResult_ERR                       = 1,
};

///
