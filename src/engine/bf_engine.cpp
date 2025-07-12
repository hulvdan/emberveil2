#pragma once

#define LOGI(...) SDL_Log(__VA_ARGS__)
#define LOGW(...) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#define LOGE(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

using Vector2    = glm::vec2;
using Vector3    = glm::vec3;
using Vector4    = glm::vec4;
using Vector2Int = glm::ivec2;
using Vector3Int = glm::ivec3;
using Vector4Int = glm::ivec4;

bool Vector2Equals(Vector2 v1, Vector2 v2) {
  return FloatEquals(v1.x, v2.x) && FloatEquals(v1.y, v2.y);
}

bool Vector3Equals(Vector3 v1, Vector3 v2) {
  return FloatEquals(v1.x, v2.x) && FloatEquals(v1.y, v2.y) && FloatEquals(v1.z, v2.z);
}

constexpr Vector2 Vector2Zero() {
  return Vector2{0, 0};
}

constexpr Vector2 Vector2Half() {
  return Vector2{0.5f, 0.5f};
}

constexpr Vector2 Vector2One() {
  return Vector2{1, 1};
}

struct Rect {
  Vector2 pos  = {};
  Vector2 size = {};
};

struct Color {
  u8 r = u8_max;
  u8 g = u8_max;
  u8 b = u8_max;
  u8 a = u8_max;
};

constexpr Color WHITE   = Color{};
constexpr Color BLACK   = Color{0, 0, 0, u8_max};
constexpr Color GRAY    = Color{u8_max / 2, u8_max / 2, u8_max / 2, u8_max};
constexpr Color RED     = Color{u8_max / 2, 0, 0, 1};
constexpr Color GREEN   = Color{0, u8_max / 2, 0, u8_max};
constexpr Color BLUE    = Color{0, 0, u8_max / 2, u8_max};
constexpr Color YELLOW  = Color{u8_max / 2, u8_max / 2, 0, u8_max};
constexpr Color CYAN    = Color{0, u8_max / 2, u8_max / 2, u8_max};
constexpr Color MAGENTA = Color{u8_max / 2, 0, u8_max / 2, u8_max};

struct Texture2D {
  Vector2Int size = {};
  void*      data = {};

  bgfx::TextureHandle handle = {};
};

const BFGame::GameLibrary* glib = nullptr;

///
struct _PosColorTexVertex {
  float    x, y, z;
  uint32_t abgr;
  float    u, v;

  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout _PosColorTexVertex::layout;

struct Camera {
  f32     zoom = 1;
  Vector2 pos  = {};
};

struct EngineData {
  struct Meta {
    Texture2D           atlas        = {};
    Vector2Int          atlasSize    = {};
    void*               gamelibBytes = {};
    bgfx::ProgramHandle program      = {};

    Camera     camera     = {};
    Vector2Int screenSize = {};

    f32 screenToLogicalRatio = {};
  } meta;
} ge = {};

void BeginMode2D(Camera camera) {}

void EndMode2D() {}

bgfx::ShaderHandle _LoadShader(const u8* data, u32 size) {
  return bgfx::createShader(bgfx::makeRef(data, size));
}

///
bgfx::ProgramHandle LoadProgram(const u8* vsh, u32 sizeVsh, const u8* fsh, u32 sizeFsh) {
  return bgfx::createProgram(
    _LoadShader(vsh, sizeVsh),
    _LoadShader(fsh, sizeFsh),
    /* destroyShaders */ true
  );
}

///
Texture2D LoadTexture(const char* filepath) {
  LOGI("Loading texture '%s'...", filepath);

  Texture2D result{};

  int channels = 0;
  result.data  = stbi_load(filepath, &result.size.x, &result.size.y, &channels, 4);
  ASSERT(result.data);

  auto memory = bgfx::makeRef(result.data, result.size.x * result.size.y * 4);

  result.handle = bgfx::createTexture2D(
    result.size.x,
    result.size.y,
    false /* _hasMips*/,
    1,
    bgfx::TextureFormat::RGBA8,
    BGFX_SAMPLER_MIN_POINT      //
      | BGFX_SAMPLER_MAG_POINT  //
      | BGFX_SAMPLER_MIP_POINT  //
      | BGFX_SAMPLER_U_CLAMP    //
      | BGFX_SAMPLER_V_CLAMP,
    memory
  );

  LOGI("Loaded texture '%s'!", filepath);

  return result;
}

///
void UnloadTexture(Texture2D* texture) {
  ASSERT(texture->data);
  bgfx::destroy(texture->handle);
  stbi_image_free(texture->data);
  *texture = {};
}

///
void* LoadFileData(const char* filepath, int* out_size = nullptr) {
  LOGI("Loading file data '%s'...", filepath);

  auto fp = fopen(filepath, "rb");
  ASSERT(fp);
  if (!fp) {
    LOGE("Failed to open file '%s'", filepath);
    INVALID_PATH;
    return nullptr;
  }

  fseek(fp, 0, SEEK_END);
  auto size = ftell(fp);
  rewind(fp);

  auto buffer = malloc(size + 1);
  if (!buffer) {
    LOGE("Failed to allocate buffer while opening file '%s'", filepath);
    fclose(fp);
    INVALID_PATH;
    return nullptr;
  }

  auto read_size = fread(buffer, 1, size, fp);
  fclose(fp);
  if (read_size != size) {
    LOGE("Failed to read entire file '%s'", filepath);
    free(buffer);
    return nullptr;
  }

  ((u8*)buffer)[size] = '\0';

  if (out_size)
    *out_size = size;

  LOGI("Loaded file data '%s'", filepath);

  return buffer;
}

void UnloadFileData(void* ptr) {
  free(ptr);
}

///
void InitializeEngine() {
  ge.meta.atlas        = LoadTexture("resources/atlas.png");
  ge.meta.gamelibBytes = LoadFileData("resources/gamelib.bin");
  glib                 = BFGame::GetGameLibrary(ge.meta.gamelibBytes);
  ge.meta.program      = LoadProgram(
    quad_vs_100_es,
    ARRAY_COUNT(quad_vs_100_es),
    quad_fs_100_es,
    ARRAY_COUNT(quad_fs_100_es)
  );

  {
    _PosColorTexVertex::layout.begin()
      .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
      .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
      .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
      .end();
  }

  LOGI("Initialized engine");
}

///
f32 ScaleToFit(Vector2 inner, Vector2 container) {
  f32 scaleX = container.x / inner.x;
  f32 scaleY = container.y / inner.y;
  f32 scale  = (scaleX < scaleY) ? scaleX : scaleY;
  return scale;
}

///
TEST_CASE ("ScaleToFit") {
  ASSERT(FloatEquals(ScaleToFit({1, 1}, {2, 2}), 2));
  ASSERT(FloatEquals(ScaleToFit({1, 1}, {3, 2}), 2));
  ASSERT(FloatEquals(ScaleToFit({1, 1}, {2, 3}), 2));
  ASSERT(FloatEquals(ScaleToFit({3, 3}, {2, 3}), 2.0f / 3.0f));
}

///
f32 ScaleToCover(Vector2 inner, Vector2 container) {
  f32 scaleX = container.x / inner.x;
  f32 scaleY = container.y / inner.y;
  f32 scale  = (scaleX > scaleY) ? scaleX : scaleY;
  return scale;
}

///
TEST_CASE ("ScaleToCover") {
  ASSERT(FloatEquals(ScaleToCover({1, 1}, {2, 2}), 2));
  ASSERT(FloatEquals(ScaleToCover({1, 1}, {3, 2}), 3));
  ASSERT(FloatEquals(ScaleToCover({1, 1}, {2, 3}), 3));
  ASSERT(FloatEquals(ScaleToCover({3, 3}, {2, 3}), 1));
}

constexpr Vector2Int LOGICAL_RESOLUTION = {1280, 720};

struct DrawTextureData {
  int     texId      = -1;
  f32     rotation   = {};
  Vector2 pos        = {};
  Vector2 anchor     = Vector2Half();
  Vector2 scale      = {1, 1};
  Vector2 sourceSize = {1, 1};
  Color   color      = WHITE;
  // bgfx::ProgramHandle program = {};
  // int materialsBufferStart = -1;
};

///
void DrawTexture(DrawTextureData data) {
  ASSERT(data.texId >= 0);

  auto tex = glib->atlas_textures()->Get(data.texId);

  Rect sourceRec{
    .pos{
      (f32)tex->atlas_x() + (f32)tex->size_x() * (1 - data.sourceSize.x),
      (f32)tex->atlas_y() + (f32)tex->size_y() * (1 - data.sourceSize.y),
    },
    .size{
      (f32)tex->size_x() * data.sourceSize.x * SIGN(data.scale.x),
      (f32)tex->size_y() * data.sourceSize.y * SIGN(data.scale.y),
    },
  };
  Rect destRec{
    .pos{
      data.pos.x
        + (f32)tex->size_x() * (1 - data.sourceSize.x) * abs(data.scale.x)
            * data.anchor.x,
      data.pos.y
        + (f32)tex->size_y() * (1 - data.sourceSize.y) * abs(data.scale.y)
            * data.anchor.y,
    },
    .size{
      (f32)tex->size_x() * data.sourceSize.x * abs(data.scale.x),
      (f32)tex->size_y() * data.sourceSize.y * abs(data.scale.y),
    },
  };
  destRec.pos -= LOGICAL_RESOLUTION / 2;

  // TODO: add / subtract epsilon?
  auto sx0 = sourceRec.pos.x;
  auto sx1 = sx0 + sourceRec.size.x;
  auto sy0 = sourceRec.pos.y;
  auto sy1 = sy0 + sourceRec.size.y;
  sx0 /= (f32)ge.meta.atlas.size.x;
  sx1 /= (f32)ge.meta.atlas.size.x;
  sy0 /= (f32)ge.meta.atlas.size.y;
  sy1 /= (f32)ge.meta.atlas.size.y;

  auto dx0 = destRec.pos.x;
  auto dx1 = destRec.pos.x + destRec.size.x;
  auto dy0 = destRec.pos.y;
  auto dy1 = destRec.pos.y + destRec.size.y;

  auto diffX = (dx1 - dx0);
  auto diffY = (dy1 - dy0);
  dx0 -= data.anchor.x * diffX;
  dx1 -= data.anchor.x * diffX;
  dy0 -= data.anchor.y * diffY;
  dy1 -= data.anchor.y * diffY;

  dx0 /= LOGICAL_RESOLUTION.x / 2;
  dx1 /= LOGICAL_RESOLUTION.x / 2;
  dy0 /= LOGICAL_RESOLUTION.y / 2;
  dy1 /= LOGICAL_RESOLUTION.y / 2;

  auto r = ge.meta.screenToLogicalRatio;
  if (r >= 1) {
    auto d = (dx1 - dx0) / 2 * (1 - 1 / r);
    dx0 += d;
    dx1 -= d;
  }
  else {
    auto d = (dy1 - dy0) / 2 * (1 - r);
    dy0 += d;
    dy1 -= d;
  }

  auto color = *(u32*)&data.color;

  const _PosColorTexVertex quadVertices[] = {
    {dx0, dy0, 0.0f, color, sx0, sy0},  // Top-left
    {dx1, dy0, 0.0f, color, sx1, sy0},  // Top-right
    {dx0, dy1, 0.0f, color, sx0, sy1},  // Bottom-left
    {dx1, dy1, 0.0f, color, sx1, sy1},  // Bottom-right
  };

  const uint16_t quadIndices[] = {0, 1, 2, 1, 3, 2};

  bgfx::UniformHandle u_texture
    = bgfx::createUniform("u_texture", bgfx::UniformType::Sampler);

  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer  tib{};
  bgfx::allocTransientVertexBuffer(&tvb, 4, _PosColorTexVertex::layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);
  {
    memcpy(tvb.data, quadVertices, sizeof(quadVertices));
    memcpy(tib.data, quadIndices, sizeof(quadIndices));

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setTexture(0, u_texture, ge.meta.atlas.handle);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA);

    bgfx::submit(0, ge.meta.program);
  }

  bgfx::destroy(u_texture);
}

u64 GetTime() {
  return SDL_GetTicks();
}

///
void EngineOnFrameStart() {
  auto ratioLogical            = (f32)LOGICAL_RESOLUTION.x / (f32)LOGICAL_RESOLUTION.y;
  auto ratioActual             = (f32)ge.meta.screenSize.x / (f32)ge.meta.screenSize.y;
  ge.meta.screenToLogicalRatio = ratioActual / ratioLogical;
}

///
