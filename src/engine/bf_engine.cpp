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

const BFGame::GameLibrary* glib = nullptr;

struct EngineData {
  struct Meta {
    Texture2D atlas = {};
    // gbFileContents gamelib = {};
    void* gamelib = {};
  } meta;
} ge = {};

///
Texture2D LoadTexture(const char* filepath) {
  Texture2D result{};

  int channels = 0;
  result.data  = stbi_load(filepath, &result.w, &result.h, &channels, 4);
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
  bgfx_destroy_texture(*(bgfx_texture_handle_t*)&texture->tex);
  stbi_image_free(texture->data);
  *texture = {};
}

void* LoadFileData(const char* filepath, int* out_size = nullptr) {
  auto fp = fopen(filepath, "rb");
  ASSERT(fp);
  if (!fp) {
    perror("Failed to open file");
    LOGE("Failed to open file %s", filepath);
    INVALID_PATH;
    return nullptr;
  }

  fseek(fp, 0, SEEK_END);
  auto size = ftell(fp);
  rewind(fp);

  auto buffer = malloc(size + 1);
  if (!buffer) {
    LOGE("Failed to allocate buffer while opening file");
    fclose(fp);
    INVALID_PATH;
    return nullptr;
  }

  auto read_size = fread(buffer, 1, size, fp);
  if (read_size != size) {
    LOGE("Failed to read entire file");
    free(buffer);
    fclose(fp);
    return nullptr;
  }

  ((u8*)buffer)[size] = '\0';
  fclose(fp);

  if (out_size)
    *out_size = size;

  return buffer;
}

void UnloadFileData(void* ptr) {
  free(ptr);
}

void InitializeEngine() {
  ge.meta.atlas   = LoadTexture("resources/atlas.png");
  ge.meta.gamelib = LoadFileData("resources/gamelib.bin");
  LOGI("Initialized engine!");
  glib = BFGame::GetGameLibrary(ge.meta.gamelib);
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
