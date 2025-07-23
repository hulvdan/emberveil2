#pragma once

#if BF_DEBUG
#  define STB_IMAGE_WRITE_IMPLEMENTATION
#  include "stb_image_write.h"
#endif

#define LOGI(...) SDL_Log(__VA_ARGS__)
#define LOGW(...) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#define LOGE(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

using Vector2    = glm::vec2;
using Vector3    = glm::vec3;
using Vector4    = glm::vec4;
using Vector2Int = glm::ivec2;
using Vector3Int = glm::ivec3;
using Vector4Int = glm::ivec4;

constexpr auto SQRT_2        = 1.41421356237f;
constexpr auto SQRT_2_OVER_2 = 0.70710678f;

///
f32 Vector2Length(Vector2 v) {
  return glm::length(v);
}

///
f32 Vector2LengthSqr(Vector2 v) {
  return v.x * v.x + v.y * v.y;
}

///
f32 Vector2Dot(Vector2 v1, Vector2 v2) {
  return glm::dot(v1, v2);
}

///
f32 Vector2Distance(Vector2 v1, Vector2 v2) {
  return Vector2Length(v2 - v1);
}

///
f32 Vector2DistanceSqr(Vector2 v1, Vector2 v2) {
  return Vector2LengthSqr(v2 - v1);
}

///
f32 Vector2Angle(Vector2 v1, Vector2 v2) {
  auto a1 = atan2f(v1.y, v1.x);
  auto a2 = atan2f(v2.y, v2.x);
  return a2 - a1;
}

///
Vector2 Vector2Normalize(Vector2 v) {
  return glm::normalize(v);
}

///
Vector2 Vector2Lerp(Vector2 v1, Vector2 v2, f32 amount) {
  return v1 + (v2 - v1) * amount;
}

///
Vector2 Vector2Reflect(Vector2 v, Vector2 normal) {
  return glm::reflect(v, normal);
}

///
Vector2 Vector2Rotate(Vector2 v, f32 angle) {
  auto c = cosf(angle);
  auto s = sinf(angle);
  return {v.x * c - v.y * s, v.x * s + v.y * c};
}

///
bool Vector2Equals(Vector2 v1, Vector2 v2) {
  return FloatEquals(v1.x, v2.x) && FloatEquals(v1.y, v2.y);
}

///
bool Vector3Equals(Vector3 v1, Vector3 v2) {
  return FloatEquals(v1.x, v2.x) && FloatEquals(v1.y, v2.y) && FloatEquals(v1.z, v2.z);
}

///
Vector2 Vector2MoveTowards(Vector2 v, Vector2 target, f32 maxDistance) {
  if (Vector2Equals(v, target))
    return target;

  auto d = Vector2Normalize(target - v);
  v.x    = MoveTowards(v.x, target.x, d.x * maxDistance);
  v.y    = MoveTowards(v.y, target.y, d.y * maxDistance);
  return v;
}

///
Vector2 Vector2Invert(Vector2 v) {
  return {1.0f / v.x, 1.0f / v.y};
}

///
Vector2 Vector2Clamp(Vector2 v, Vector2 min, Vector2 max) {
  return glm::clamp(v, min, max);
}

///
Vector2 Vector2ClampValue(Vector2 v, f32 min, f32 max) {
  return glm::clamp(v, min, max);
}

///
constexpr Vector2 Vector2Zero() {
  return Vector2{0, 0};
}

///
constexpr Vector2 Vector2Half() {
  return Vector2{0.5f, 0.5f};
}

///
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
constexpr Color RED     = Color{u8_max, 0, 0, u8_max};
constexpr Color GREEN   = Color{0, u8_max, 0, u8_max};
constexpr Color BLUE    = Color{0, 0, u8_max, u8_max};
constexpr Color YELLOW  = Color{u8_max, u8_max, 0, u8_max};
constexpr Color CYAN    = Color{0, u8_max, u8_max, u8_max};
constexpr Color MAGENTA = Color{u8_max, 0, u8_max, u8_max};

constexpr Vector2Int ASSETS_REFERENCE_RESOLUTION = {1920, 1080};
constexpr Vector2Int LOGICAL_RESOLUTION          = {1280, 720};

struct Texture2D {
  Vector2Int size = {};
  void*      data = {};

  bgfx::TextureHandle handle = {};
};

const BFGame::GameLibrary* glib = nullptr;

///
struct _PosColorTexVertex {
  f32 x, y, z;
  u32 abgr;
  f32 u, v;

  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout _PosColorTexVertex::layout;

///
struct _PosColorVertex {
  f32 x, y, z;
  u32 abgr;

  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout _PosColorVertex::layout;

enum DeviceType {
  DeviceType_DESKTOP,
  DeviceType_MOBILE,
};

struct TouchID {
  SDL_TouchID  _touchID  = {};
  SDL_FingerID _fingerID = {};

  bool operator==(const TouchID& other) const {
    return (_touchID == other._touchID) && (_fingerID == other._fingerID);
  }
};

constexpr TouchID InvalidTouchID = {};

struct Touch {
  TouchID _id         = {};
  Vector2 _screenPos  = {};
  Vector2 _screenDPos = {};
};

enum _TouchDataState : u32 {
  _TouchDataState_DOWN     = 0x1,
  _TouchDataState_PREV     = 0x2,
  _TouchDataState_PRESSED  = 0x4,
  _TouchDataState_RELEASED = 0x8,
};

struct _TouchData {
  Vector2 _screenPos  = {};
  Vector2 _screenDPos = {};
  u64     _userData   = {};
  u32     _state      = {};
};

struct EngineData {
  struct Meta {
    Texture2D           atlas                 = {};
    Vector2Int          atlasSize             = {};
    bgfx::ProgramHandle programDefaultTexture = {};
    bgfx::ProgramHandle programDefaultQuad    = {};
    bgfx::UniformHandle uniformTexture        = {};

    Vector2Int screenSize = {};

    f32 screenToLogicalRatio = {};
    f32 screenScale          = {};

    int         _keyboardStateCount    = {};
    const bool* _keyboardState         = {};
    bool*       _keyboardStatePrev     = {};
    bool*       _keyboardStatePressed  = {};
    bool*       _keyboardStateReleased = {};

    u32     _mouseState         = {};
    u32     _mouseStatePrev     = {};
    u32     _mouseStatePressed  = {};
    u32     _mouseStateReleased = {};
    Vector2 _mousePos           = {};

    Vector<TouchID>    _touchIDs = {};
    Vector<_TouchData> _touches  = {};

    i64 ticks         = {};
    f64 prevFrameTime = {};
    f64 frameTime     = {};

    Arena _trashArena = {};

    DeviceType deviceType = DeviceType_DESKTOP;

    Vector2 _screenToLogicalScale = {};
    Vector2 _screenToLogicalAdd   = {};
  } meta;

  struct Settings {
    Color vignetteAndStripsColor = BLACK;
  } settings;
} ge = {};

///
void _OnTouchDown(Touch touch) {
  for (auto id : ge.meta._touchIDs)
    ASSERT(id != touch._id);

  _TouchData d{
    ._screenPos  = touch._screenPos,
    ._screenDPos = touch._screenDPos,
    ._state      = _TouchDataState_PRESSED | _TouchDataState_DOWN,
  };
  *ge.meta._touchIDs.Add() = touch._id;
  *ge.meta._touches.Add()  = d;
}

///
void _OnTouchUp(Touch touch) {
  auto found = false;
  // Marking as released. It will be removed on calling `ResetPressedReleasedStates`.
  FOR_RANGE (int, i, ge.meta._touches.count) {
    auto id = ge.meta._touchIDs[i];
    if (id == touch._id) {
      found        = true;
      auto& t      = ge.meta._touches[i];
      t._screenPos = touch._screenPos;
      t._state |= _TouchDataState_RELEASED;
      t._state &= ~_TouchDataState_DOWN;
      break;
    }
  }
  ASSERT(found);
}

///
void _OnTouchMoved(Touch touch) {
  auto found = false;
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == touch._id) {
      found         = true;
      auto& t       = ge.meta._touches[i];
      t._screenPos  = touch._screenPos;
      t._screenDPos = touch._screenDPos;
      break;
    }
  }
  ASSERT(found);
}

///
void ResetPressedReleasedStates() {
  ge.meta._mouseStatePressed  = 0;
  ge.meta._mouseStateReleased = 0;
  FOR_RANGE (int, i, ge.meta._keyboardStateCount) {
    ge.meta._keyboardStatePressed[i]  = false;
    ge.meta._keyboardStateReleased[i] = false;
  }

  // Removing previously released touches.
  {
    auto total = ge.meta._touches.count;
    auto off   = 0;
    FOR_RANGE (int, i, total) {
      if (ge.meta._touches[i - off]._state & _TouchDataState_RELEASED) {
        auto id = ge.meta._touchIDs[i - off];
        ge.meta._touchIDs.UnstableRemoveAt(i - off);
        ge.meta._touches.UnstableRemoveAt(i - off);
        off++;
      }
    }
  }

  for (auto& t : ge.meta._touches)
    t._state &= ~_TouchDataState_PRESSED;
}

///
bool IsTouchPressed(TouchID id) {
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id) {
      auto& t = ge.meta._touches[i];
      return t._state & _TouchDataState_PRESSED;
    }
  }
  INVALID_PATH;  // Not found.
  return false;
}

///
bool IsTouchReleased(TouchID id) {
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id) {
      auto& t = ge.meta._touches[i];
      return t._state & _TouchDataState_RELEASED;
    }
  }
  INVALID_PATH;  // Not found.
  return false;
}

///
bool IsTouchDown(TouchID id) {
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id) {
      auto& t = ge.meta._touches[i];
      return t._state & _TouchDataState_DOWN;
    }
  }
  return false;
}

///
TEST_CASE ("Touch controls") {
  LAMBDA (TouchID, id, (SDL_TouchID i)) {
    return {._touchID = i, ._fingerID = i};
  };
  LAMBDA (Touch, touch, (SDL_TouchID _id)) {
    return {._id = id(_id)};
  };

  {
    _OnTouchDown(touch(1));
    _OnTouchMoved(touch(1));

    ASSERT(IsTouchPressed(id(1)));
    ASSERT(IsTouchDown(id(1)));
    ASSERT(!IsTouchReleased(id(1)));
    ResetPressedReleasedStates();

    ASSERT(!IsTouchPressed(id(1)));
    ASSERT(IsTouchDown(id(1)));
    ASSERT(!IsTouchReleased(id(1)));
    ResetPressedReleasedStates();

    _OnTouchUp(touch(1));
    ASSERT(!IsTouchPressed(id(1)));
    ASSERT(!IsTouchDown(id(1)));
    ASSERT(IsTouchReleased(id(1)));
    ResetPressedReleasedStates();

    ASSERT(ge.meta._touches.count == 0);
  }

  {
    _OnTouchDown(touch(3));
    _OnTouchDown(touch(4));
    ASSERT(ge.meta._touches.count == 2);
    ASSERT(ge.meta._touchIDs.count == 2);

    ResetPressedReleasedStates();

    _OnTouchUp(touch(4));
    _OnTouchUp(touch(3));

    ResetPressedReleasedStates();

    ASSERT(ge.meta._touches.count == 0);
    ASSERT(ge.meta._touchIDs.count == 0);
  }

  {
    _OnTouchDown(touch(5));
    _OnTouchDown(touch(6));

    ResetPressedReleasedStates();

    _OnTouchUp(touch(5));
    _OnTouchUp(touch(6));

    ResetPressedReleasedStates();

    ASSERT(ge.meta._touches.count == 0);
    ASSERT(ge.meta._touchIDs.count == 0);
  }
}

bgfx::ShaderHandle _LoadShader(const u8* data, u32 size) {
  return bgfx::createShader(bgfx::makeRef(data, size));
}

View<TouchID> GetTouchIDs() {
  return {.count = ge.meta._touchIDs.count, .base = ge.meta._touchIDs.base};
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
    BGFX_SAMPLER_MIN_ANISOTROPIC      //
      | BGFX_SAMPLER_MAG_ANISOTROPIC  //
      | BGFX_SAMPLER_MIP_POINT        //
      | BGFX_SAMPLER_U_CLAMP          //
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
constexpr f32 ScaleToFit(Vector2 inner, Vector2 container) {
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
constexpr f32 ScaleToCover(Vector2 inner, Vector2 container) {
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

///
void InitializeEngine() {
  ge.meta._touches.Reserve(8);
  ge.meta._touchIDs.Reserve(8);
  ge.meta._keyboardState = SDL_GetKeyboardState(&ge.meta._keyboardStateCount);
  ge.meta._trashArena    = MakeArena(3 * sizeof(bool) * ge.meta._keyboardStateCount);
  ge.meta._keyboardStatePrev
    = ALLOCATE_ZEROS_ARRAY(&ge.meta._trashArena, bool, ge.meta._keyboardStateCount);
  ge.meta._keyboardStatePressed
    = ALLOCATE_ZEROS_ARRAY(&ge.meta._trashArena, bool, ge.meta._keyboardStateCount);
  ge.meta._keyboardStateReleased
    = ALLOCATE_ZEROS_ARRAY(&ge.meta._trashArena, bool, ge.meta._keyboardStateCount);

  ge.meta.atlas = LoadTexture("resources/atlas.png");
  glib          = BFGame::GetGameLibrary(LoadFileData("resources/gamelib.bin"));
  ge.meta.programDefaultTexture = LoadProgram(
    quad_tex_vs_100_es,
    ARRAY_COUNT(quad_tex_vs_100_es),
    quad_tex_fs_100_es,
    ARRAY_COUNT(quad_tex_fs_100_es)
  );
  ge.meta.programDefaultQuad = LoadProgram(
    quad_vs_100_es,
    ARRAY_COUNT(quad_vs_100_es),
    quad_fs_100_es,
    ARRAY_COUNT(quad_fs_100_es)
  );

  // Remove Z?
  _PosColorTexVertex::layout.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
    .end();

  _PosColorVertex::layout.begin()
    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
    .end();

  ge.meta.uniformTexture = bgfx::createUniform("u_texture", bgfx::UniformType::Sampler);

  ge.meta.screenScale = ScaleToFit(ASSETS_REFERENCE_RESOLUTION, LOGICAL_RESOLUTION);

  LOGI("Initialized engine");
}

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
            * ge.meta.screenScale * data.anchor.x,
      data.pos.y
        + (f32)tex->size_y() * (1 - data.sourceSize.y) * abs(data.scale.y)
            * ge.meta.screenScale * data.anchor.y,
    },
    .size{
      (f32)tex->size_x() * data.sourceSize.x * abs(data.scale.x) * ge.meta.screenScale,
      (f32)tex->size_y() * data.sourceSize.y * abs(data.scale.y) * ge.meta.screenScale,
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

  Vector2 topLeft{};
  Vector2 topRight{};
  Vector2 bottomLeft{};
  Vector2 bottomRight{};

  if (data.rotation == 0.0f) {
    auto dx0 = destRec.pos.x;
    auto dx1 = destRec.pos.x + destRec.size.x;
    auto dy0 = destRec.pos.y;
    auto dy1 = destRec.pos.y + destRec.size.y;

    dx0 -= data.anchor.x * destRec.size.x;
    dx1 -= data.anchor.x * destRec.size.x;
    dy0 -= data.anchor.y * destRec.size.y;
    dy1 -= data.anchor.y * destRec.size.y;

    topLeft     = {dx0, dy0};
    topRight    = {dx1, dy0};
    bottomLeft  = {dx0, dy1};
    bottomRight = {dx1, dy1};
  }
  else {
    auto sinRotation = sinf(data.rotation);
    auto cosRotation = cosf(data.rotation);

    auto dx = -data.anchor.x * destRec.size.x;
    auto dy = -data.anchor.y * destRec.size.y;

    topLeft.x = destRec.pos.x + dx * cosRotation - dy * sinRotation;
    topLeft.y = destRec.pos.y + dx * sinRotation + dy * cosRotation;

    topRight.x = destRec.pos.x + (dx + destRec.size.x) * cosRotation - dy * sinRotation;
    topRight.y = destRec.pos.y + (dx + destRec.size.x) * sinRotation + dy * cosRotation;

    bottomLeft.x = destRec.pos.x + dx * cosRotation - (dy + destRec.size.y) * sinRotation;
    bottomLeft.y = destRec.pos.y + dx * sinRotation + (dy + destRec.size.y) * cosRotation;

    bottomRight.x = destRec.pos.x + (dx + destRec.size.x) * cosRotation
                    - (dy + destRec.size.y) * sinRotation;
    bottomRight.y = destRec.pos.y + (dx + destRec.size.x) * sinRotation
                    + (dy + destRec.size.y) * cosRotation;
  }

  topLeft.x /= LOGICAL_RESOLUTION.x / 2;
  topRight.x /= LOGICAL_RESOLUTION.x / 2;
  bottomLeft.x /= LOGICAL_RESOLUTION.x / 2;
  bottomRight.x /= LOGICAL_RESOLUTION.x / 2;
  topLeft.y /= LOGICAL_RESOLUTION.y / 2;
  topRight.y /= LOGICAL_RESOLUTION.y / 2;
  bottomLeft.y /= LOGICAL_RESOLUTION.y / 2;
  bottomRight.y /= LOGICAL_RESOLUTION.y / 2;

  const auto r = ge.meta.screenToLogicalRatio;
  if (r >= 1) {  // Window is too wide.
    const auto c = 1 - 1 / r;
    topLeft.x -= topLeft.x * c;
    topRight.x -= topRight.x * c;
    bottomLeft.x -= bottomLeft.x * c;
    bottomRight.x -= bottomRight.x * c;
  }
  else {  // Window is too high.
    const auto c = 1 - r;
    topLeft.y -= topLeft.y * c;
    topRight.y -= topRight.y * c;
    bottomLeft.y -= bottomLeft.y * c;
    bottomRight.y -= bottomRight.y * c;
  }

  const auto color = *(u32*)&data.color;

  const _PosColorTexVertex quadVertices[] = {
    {topLeft.x, topLeft.y, 0.0f, color, sx0, sy1},
    {topRight.x, topRight.y, 0.0f, color, sx1, sy1},
    {bottomLeft.x, bottomLeft.y, 0.0f, color, sx0, sy0},
    {bottomRight.x, bottomRight.y, 0.0f, color, sx1, sy0},
  };

  const u16 quadIndices[] = {0, 1, 2, 1, 3, 2};

  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer  tib{};
  bgfx::allocTransientVertexBuffer(&tvb, 4, _PosColorTexVertex::layout);
  bgfx::allocTransientIndexBuffer(&tib, 6);
  {
    memcpy(tvb.data, quadVertices, sizeof(quadVertices));
    memcpy(tib.data, quadIndices, sizeof(quadIndices));

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setTexture(0, ge.meta.uniformTexture, ge.meta.atlas.handle);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA);

    bgfx::submit(0, ge.meta.programDefaultTexture);
  }
}

struct Font {
  bool loaded = false;

  int size = {};

  // Used for scaling font size.
  // Specifying `size` 30 doesn't guarantee
  // that letter `M` would be 30 px high.
  f32 FIXME_sizeScale = {};

  Texture2D atlasTexture = {};

  const u8*         fileData = {};
  stbtt_packedchar* chars    = {};
  stbtt_fontinfo    info     = {};

  int* codepoints      = {};
  int  codepointsCount = {};
  int  outlineWidth    = {};
};

///
void UnloadFont(Font* font) {
  ASSERT(font->loaded);
  font->loaded = false;

  UnloadFileData((void*)font->fileData);
  font->fileData = nullptr;

  free(font->chars);
  font->chars = nullptr;

  bgfx::destroy(font->atlasTexture.handle);
  font->atlasTexture.handle = {};
}

struct LoadFontData {
  const char* filepath = {};

  int size            = {};
  f32 FIXME_sizeScale = {};

  int* codepoints      = {};
  int  codepointsCount = {};

  int outlineWidth = 0;  // Optional.
};

///
Font LoadFont(LoadFontData data) {
  ASSERT(data.filepath);
  ASSERT(data.size >= 0);
  ASSERT(data.codepoints);
  ASSERT(data.codepointsCount > 0);
  ASSERT(data.outlineWidth >= 0);

  const Vector2Int atlasSize{1024, 1024};

  auto oneChannelAtlasData = (u8*)malloc(atlasSize.x * atlasSize.y * 1);

  Font font{
    .loaded          = true,
    .size            = data.size,
    .FIXME_sizeScale = data.FIXME_sizeScale,
    .atlasTexture{.size = atlasSize},
    .fileData = (u8*)LoadFileData(data.filepath),
    .chars = (stbtt_packedchar*)malloc(sizeof(stbtt_packedchar) * data.codepointsCount),
    .codepoints      = data.codepoints,
    .codepointsCount = data.codepointsCount,
    .outlineWidth    = data.outlineWidth,
  };

  stbtt_InitFont(&font.info, font.fileData, 0);

  stbtt_pack_context context{};

  if (!stbtt_PackBegin(
        &context,
        oneChannelAtlasData,
        atlasSize.x,
        atlasSize.y,
        0,
        1 + 2 * data.outlineWidth,
        nullptr
      ))
  {
    LOGE("stbtt_PackBegin failed");
    INVALID_PATH;
    return {};
  }

  stbtt_pack_range range{
    .font_size                   = (f32)data.size * data.FIXME_sizeScale,
    .array_of_unicode_codepoints = data.codepoints,
    .num_chars                   = data.codepointsCount,
    .chardata_for_range          = font.chars,
  };
  if (!stbtt_PackFontRanges(&context, font.fileData, 0, &range, 1)) {
    LOGE("stbtt_PackFontRanges failed");
    INVALID_PATH;
    return {};
  }

  stbtt_PackEnd(&context);

  auto atlasData = (u8*)malloc(atlasSize.x * atlasSize.y * 4);

  FOR_RANGE (int, i, atlasSize.x * atlasSize.y) {
    atlasData[i * 4 + 0] = 255;
    atlasData[i * 4 + 1] = 255;
    atlasData[i * 4 + 2] = 255;
    atlasData[i * 4 + 3] = oneChannelAtlasData[i];
  }

  if (data.outlineWidth) {
    auto      dist_ = (f32*)malloc(atlasSize.x * atlasSize.y * sizeof(f32));
    View<f32> dist{
      .count = atlasSize.x * atlasSize.y,
      .base  = dist_,
    };

    for (int i = 0; i < atlasSize.x * atlasSize.y; i++) {
      if (oneChannelAtlasData[i])
        dist[i] = 1 - (f32)oneChannelAtlasData[i] / 255.0f;
      else
        dist[i] = f32_inf;
    }

#define INDEX(y_, x_) ((y_) * atlasSize.x + (x_))

    // First pass: top-left to bottom-right
    for (int y = 0; y < atlasSize.y; y++) {
      for (int x = 0; x < atlasSize.x; x++) {
        int idx = INDEX(y, x);
        // Check neighbor above
        if (y > 0) {
          auto t    = INDEX(y - 1, x);
          f32  val  = dist[t] + 1;
          dist[idx] = MIN(dist[idx], val);
        }
        // Check neighbor to the left
        if (x > 0) {
          auto t    = INDEX(y, x - 1);
          f32  val  = dist[t] + 1;
          dist[idx] = MIN(dist[idx], val);
        }
        // Check neighbor diagonally above-left
        if ((y > 0) && (x > 0)) {
          auto t    = INDEX(y - 1, x - 1);
          f32  val  = dist[t] + SQRT_2;
          dist[idx] = MIN(dist[idx], val);
        }
        // Check neighbor diagonally above-right
        if ((y > 0) && (x < atlasSize.x - 1)) {
          auto t    = INDEX(y - 1, x + 1);
          f32  val  = dist[t] + SQRT_2;
          dist[idx] = MIN(dist[idx], val);
        }
      }
    }

    // Second pass: bottom-right to top-left
    for (int y = atlasSize.y - 1; y >= 0; y--) {
      for (int x = atlasSize.x - 1; x >= 0; x--) {
        int idx = INDEX(y, x);
        // Check neighbor below
        if (y + 1 < atlasSize.y) {
          auto t    = INDEX(y + 1, x);
          f32  val  = dist[t] + 1;
          dist[idx] = MIN(dist[idx], val);
        }
        // Check neighbor to the right
        if (x + 1 < atlasSize.x) {
          auto t    = INDEX(y, x + 1);
          f32  val  = dist[t] + 1;
          dist[idx] = MIN(dist[idx], val);
        }
        // Check neighbor diagonally below-right
        if ((y + 1 < atlasSize.y) && (x + 1 < atlasSize.x)) {
          auto t    = INDEX(y + 1, x + 1);
          f32  val  = dist[t] + SQRT_2;
          dist[idx] = MIN(dist[idx], val);
        }
        // Check neighbor diagonally below-left
        if ((y + 1 < atlasSize.y) && (x > 0)) {
          auto t    = INDEX(y + 1, x - 1);
          f32  val  = dist[t] + SQRT_2;
          dist[idx] = MIN(dist[idx], val);
        }
      }
    }

#undef INDEX

    FOR_RANGE (int, y, atlasSize.y) {
      FOR_RANGE (int, x, atlasSize.x) {
        auto t = y * atlasSize.x + x;

        auto v               = oneChannelAtlasData[t];
        atlasData[t * 4 + 0] = v;
        atlasData[t * 4 + 1] = v;
        atlasData[t * 4 + 2] = v;

        if (dist[t] > 0) {
          u8 alpha = 255;
          if (dist[t] > (f32)data.outlineWidth - 1)
            alpha = (u8)(255.0f * MAX((f32)data.outlineWidth - dist[t], 0));
          atlasData[t * 4 + 3] = alpha;
        }
      }
    }

    FOR_RANGE (int, i, data.codepointsCount) {
      auto& c = font.chars[i];
      c.x0 -= data.outlineWidth;
      c.x1 += data.outlineWidth;
      c.y0 -= data.outlineWidth;
      c.y1 += data.outlineWidth;
    }

    free(dist_);
  }

#if BF_DEBUG & !defined(SDL_PLATFORM_EMSCRIPTEN)
  stbi_write_png("debugFontAtlas.png", atlasSize.x, atlasSize.y, 4, atlasData, 0);
#endif
  // TODO: Rework as bgfx::TextureFormat::A8 + appropriate fragment shader.
  font.atlasTexture.handle = bgfx::createTexture2D(
    atlasSize.x,
    atlasSize.y,
    false /* _hasMips*/,
    1,
    bgfx::TextureFormat::RGBA8,
    BGFX_SAMPLER_MIN_ANISOTROPIC      //
      | BGFX_SAMPLER_MAG_ANISOTROPIC  //
      | BGFX_SAMPLER_MIP_POINT        //
      | BGFX_SAMPLER_U_CLAMP          //
      | BGFX_SAMPLER_V_CLAMP,
    bgfx::copy(atlasData, atlasSize.x * atlasSize.y * 4)
  );

  free(oneChannelAtlasData);
  free(atlasData);

  return font;
}

// clang-format off
static const u8 s_utf8d[364]{
	// The first part of the table maps bytes to character classes that
	// to reduce the size of the transition table and create bitmasks.
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
	 7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
	 8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

	// The second part is a transition table that maps a combination
	// of a state of the automaton and a character class to a state.
	 0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
	12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
	12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
	12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
	12,36,12,12,12,12,12,12,12,12,12,12
};
// clang-format on

///
u32 utf8_decode(u32* _state, u8 _ch, u32* _codep) {
  u32 byte = _ch;
  u32 type = s_utf8d[byte];
  *_codep  = (*_state != 0) ? (byte & 0x3fu) | (*_codep << 6) : (0xff >> type) & (byte);
  *_state  = s_utf8d[256 + *_state + type];
  return *_state;
}

struct DrawTextData {
  Vector2 pos = {};
  // TODO: Vector2 scale = {1, 1};
  // TODO: Vector2 anchor = {0.5f, 0.5f};
  const Font* font  = {};
  const char* text  = {};
  int         count = {};
  Color       color = WHITE;
  // TODO: Color outlineColor = BLACK + shader.
};

///
void DrawText(DrawTextData data) {
  ASSERT(data.count > 0);
  ASSERT(data.font);
  if (!data.font->loaded) {
    LOGE("DrawText: font is not loaded!");
    INVALID_PATH;
    return;
  }

  auto& font = data.font;
  auto  p    = data.text;

  u32  codepoint{};
  u32  state{};
  auto remaining = data.count;

  auto y = LOGICAL_RESOLUTION.y - data.pos.y;

  for (; *p; ++p) {
    if (utf8_decode(&state, *(u8*)p, &codepoint))
      continue;

    auto glyphIndex
      = ArrayBinaryFind(font->codepoints, font->codepointsCount, (int)codepoint);
    ASSERT(glyphIndex >= 0);

    stbtt_aligned_quad q{};
    stbtt_GetPackedQuad(
      font->chars,
      font->atlasTexture.size.x,
      font->atlasTexture.size.y,
      glyphIndex,
      &data.pos.x,
      // &data.pos.y,
      &y,
      &q,
      1  // 1=opengl & d3d10+,0=d3d9
    );

    q.x0 -= font->outlineWidth;
    q.x1 += font->outlineWidth;
    q.y0 -= font->outlineWidth;
    q.y1 += font->outlineWidth;

    {
      auto sx0 = q.s0;
      auto sx1 = q.s1;
      auto sy0 = q.t0;
      auto sy1 = q.t1;

      Vector2 topLeft{};
      Vector2 topRight{};
      Vector2 bottomLeft{};
      Vector2 bottomRight{};

      auto lrh    = (Vector2)LOGICAL_RESOLUTION / 2.0f;
      topLeft     = Vector2{q.x0, q.y1} / lrh - Vector2One();
      topRight    = Vector2{q.x1, q.y1} / lrh - Vector2One();
      bottomLeft  = Vector2{q.x0, q.y0} / lrh - Vector2One();
      bottomRight = Vector2{q.x1, q.y0} / lrh - Vector2One();

      auto dy       = -(f32)font->size * 2.0f / (f32)LOGICAL_RESOLUTION.y;
      topLeft.y     = dy - topLeft.y;
      topRight.y    = dy - topRight.y;
      bottomLeft.y  = dy - bottomLeft.y;
      bottomRight.y = dy - bottomRight.y;

      const auto r = ge.meta.screenToLogicalRatio;
      if (r >= 1) {  // Window is too wide.
        const auto c = 1 - 1 / r;
        topLeft.x -= topLeft.x * c;
        topRight.x -= topRight.x * c;
        bottomLeft.x -= bottomLeft.x * c;
        bottomRight.x -= bottomRight.x * c;
      }
      else {  // Window is too high.
        const auto c = 1 - r;
        topLeft.y -= topLeft.y * c;
        topRight.y -= topRight.y * c;
        bottomLeft.y -= bottomLeft.y * c;
        bottomRight.y -= bottomRight.y * c;
      }

      const auto color = *(u32*)&data.color;

      const _PosColorTexVertex quadVertices[] = {
        {topLeft.x, topLeft.y, 0.0f, color, sx0, sy1},
        {topRight.x, topRight.y, 0.0f, color, sx1, sy1},
        {bottomLeft.x, bottomLeft.y, 0.0f, color, sx0, sy0},
        {bottomRight.x, bottomRight.y, 0.0f, color, sx1, sy0},
      };

      const u16 quadIndices[] = {0, 1, 2, 1, 3, 2};

      bgfx::TransientVertexBuffer tvb{};
      bgfx::TransientIndexBuffer  tib{};
      bgfx::allocTransientVertexBuffer(&tvb, 4, _PosColorTexVertex::layout);
      bgfx::allocTransientIndexBuffer(&tib, 6);
      {
        memcpy(tvb.data, quadVertices, sizeof(quadVertices));
        memcpy(tib.data, quadIndices, sizeof(quadIndices));

        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setIndexBuffer(&tib);
        bgfx::setTexture(0, ge.meta.uniformTexture, font->atlasTexture.handle);
        bgfx::setState(
          BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA
        );

        bgfx::submit(0, ge.meta.programDefaultTexture);
      }
    }

    remaining--;
    if (!remaining)
      break;
  }

  ASSERT_FALSE(state);  // The string is not well-formed.
}

///
void DrawCircleLines(f32 centerX, f32 centerY, f32 radius, Color color_) {
  const auto s = SQRT_2_OVER_2;
  Vector2    points[8]{
    {centerX, centerY - radius},
    {centerX + radius * s, centerY - radius * s},
    {centerX + radius, centerY},
    {centerX + radius * s, centerY + radius * s},
    {centerX, centerY + radius},
    {centerX - radius * s, centerY + radius * s},
    {centerX - radius, centerY},
    {centerX - radius * s, centerY - radius * s},
  };

  for (auto& point : points) {
    point -= (Vector2)(LOGICAL_RESOLUTION) / 2.0f;
    point /= (Vector2)(LOGICAL_RESOLUTION) / 2.0f;
  }

  const auto r = ge.meta.screenToLogicalRatio;
  if (r >= 1) {  // Window is too wide.
    const auto c = 1 - 1 / r;
    for (auto& point : points)
      point.x -= point.x * c;
  }
  else {  // Window is too high.
    const auto c = 1 - r;
    for (auto& point : points)
      point.y -= point.y * c;
  }

  bgfx::TransientVertexBuffer tvb{};
  bgfx::allocTransientVertexBuffer(&tvb, 9, _PosColorVertex::layout);
  {
    auto                  color          = *(u32*)&color_;
    const _PosColorVertex quadVertices[] = {
      {points[0].x, points[0].y, 0.0f, color},
      {points[1].x, points[1].y, 0.0f, color},
      {points[2].x, points[2].y, 0.0f, color},
      {points[3].x, points[3].y, 0.0f, color},
      {points[4].x, points[4].y, 0.0f, color},
      {points[5].x, points[5].y, 0.0f, color},
      {points[6].x, points[6].y, 0.0f, color},
      {points[7].x, points[7].y, 0.0f, color},
      {points[0].x, points[0].y, 0.0f, color},
    };

    memcpy(tvb.data, quadVertices, sizeof(quadVertices));

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setState(
      BGFX_STATE_WRITE_RGB | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_PT_LINESTRIP
    );

    bgfx::submit(0, ge.meta.programDefaultQuad);
  }
}

void DrawCircleLines(Vector2 center, f32 radius, Color color) {
  DrawCircleLines(center.x, center.y, radius, color);
}

struct DrawRectData {
  Vector2 pos    = {};
  Vector2 size   = {};
  Vector2 anchor = {};
  Color   color  = {};
};

///
void DrawRect(DrawRectData data) {
  auto    off = data.anchor * data.size;
  Vector2 points[]{
    {data.pos.x - off.x, data.pos.y + data.size.y - off.y},
    {data.pos.x + data.size.x - off.x, data.pos.y + data.size.y - off.y},
    {data.pos.x - off.x, data.pos.y - off.y},
    {data.pos.x + data.size.x - off.x, data.pos.y - off.y},
  };

  for (auto& point : points) {
    point -= (Vector2)(LOGICAL_RESOLUTION) / 2.0f;
    point /= (Vector2)(LOGICAL_RESOLUTION) / 2.0f;
  }

  const auto r = ge.meta.screenToLogicalRatio;
  if (r >= 1) {  // Window is too wide.
    const auto c = 1 - 1 / r;
    for (auto& point : points)
      point.x -= point.x * c;
  }
  else {  // Window is too high.
    const auto c = 1 - r;
    for (auto& point : points)
      point.y -= point.y * c;
  }

  {
    auto                  color          = *(u32*)&data.color;
    const _PosColorVertex quadVertices[] = {
      {points[0].x, points[0].y, 0.0f, color},
      {points[1].x, points[1].y, 0.0f, color},
      {points[2].x, points[2].y, 0.0f, color},
      {points[3].x, points[3].y, 0.0f, color},
    };

    const u16 quadIndices[] = {0, 1, 2, 1, 3, 2};

    bgfx::TransientVertexBuffer tvb{};
    bgfx::TransientIndexBuffer  tib{};
    bgfx::allocTransientVertexBuffer(&tvb, 4, _PosColorVertex::layout);
    bgfx::allocTransientIndexBuffer(&tib, 6);

    memcpy(tvb.data, quadVertices, sizeof(quadVertices));
    memcpy(tib.data, quadIndices, sizeof(quadIndices));

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA);

    bgfx::submit(0, ge.meta.programDefaultQuad);
  }
}

///
void DrawRectLines(DrawRectData data) {
  auto    off = data.anchor * data.size;
  Vector2 points[]{
    {data.pos.x - off.x, data.pos.y - off.y},
    {data.pos.x + data.size.x - off.x, data.pos.y - off.y},
    {data.pos.x + data.size.x - off.x, data.pos.y + data.size.y - off.y},
    {data.pos.x - off.x, data.pos.y + data.size.y - off.y},
  };

  for (auto& point : points) {
    point -= (Vector2)(LOGICAL_RESOLUTION) / 2.0f;
    point /= (Vector2)(LOGICAL_RESOLUTION) / 2.0f;
  }

  const auto r = ge.meta.screenToLogicalRatio;
  if (r >= 1) {  // Window is too wide.
    const auto c = 1 - 1 / r;
    for (auto& point : points)
      point.x -= point.x * c;
  }
  else {  // Window is too high.
    const auto c = 1 - r;
    for (auto& point : points)
      point.y -= point.y * c;
  }

  bgfx::TransientVertexBuffer tvb{};
  bgfx::allocTransientVertexBuffer(&tvb, 5, _PosColorVertex::layout);
  {
    auto                  color          = *(u32*)&data.color;
    const _PosColorVertex quadVertices[] = {
      {points[0].x, points[0].y, 0.0f, color},
      {points[1].x, points[1].y, 0.0f, color},
      {points[2].x, points[2].y, 0.0f, color},
      {points[3].x, points[3].y, 0.0f, color},
      {points[0].x, points[0].y, 0.0f, color},
    };

    memcpy(tvb.data, quadVertices, sizeof(quadVertices));

    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setState(
      BGFX_STATE_WRITE_RGB | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_PT_LINESTRIP
    );

    bgfx::submit(0, ge.meta.programDefaultQuad);
  }
}

i64 GetTicks() {
  return ge.meta.ticks;
}

f64 GetTime() {
  return (f64)ge.meta.ticks / 1000.0;
}

f32 FrameTime() {
  return (f32)(ge.meta.frameTime - ge.meta.prevFrameTime);
}

void EngineOnFrameStart() {
  ge.meta.ticks         = (i64)SDL_GetTicks();
  ge.meta.prevFrameTime = ge.meta.frameTime;
  ge.meta.frameTime     = GetTime();

  auto ratioLogical            = (f32)LOGICAL_RESOLUTION.x / (f32)LOGICAL_RESOLUTION.y;
  auto ratioActual             = (f32)ge.meta.screenSize.x / (f32)ge.meta.screenSize.y;
  ge.meta.screenToLogicalRatio = ratioActual / ratioLogical;

  /// Controls. Keyboard.
  {
    FOR_RANGE (int, i, ge.meta._keyboardStateCount) {
      ge.meta._keyboardStatePressed[i]
        = !ge.meta._keyboardStatePrev[i] && ge.meta._keyboardState[i];
      ge.meta._keyboardStateReleased[i]
        = ge.meta._keyboardStatePrev[i] && !ge.meta._keyboardState[i];
    }

    memcpy(
      (void*)ge.meta._keyboardStatePrev,
      (void*)ge.meta._keyboardState,
      sizeof(bool) * ge.meta._keyboardStateCount
    );
  }

  /// Controls. Mouse.
  {
    ge.meta._mouseState = SDL_GetMouseState(&ge.meta._mousePos.x, &ge.meta._mousePos.y);
    ge.meta._mousePos.y = ge.meta.screenSize.y - ge.meta._mousePos.y;

    int mouseButtons[]{
      SDL_BUTTON_LMASK,
      SDL_BUTTON_MMASK,
      SDL_BUTTON_RMASK,
      SDL_BUTTON_X1MASK,
      SDL_BUTTON_X2MASK,
    };
    ge.meta._mouseStatePressed  = 0;
    ge.meta._mouseStateReleased = 0;
    for (auto v : mouseButtons) {
      auto isDown  = ge.meta._mouseState & v;
      auto wasDown = ge.meta._mouseStatePrev & v;
      ge.meta._mouseStatePressed |= (~wasDown & isDown);
      ge.meta._mouseStateReleased |= (wasDown & ~isDown);
    }

    ge.meta._mouseStatePrev = ge.meta._mouseState;
  }

  /// Caching ScreenToLogical data.
  {
    const auto r = ge.meta.screenToLogicalRatio;
    const auto s = (Vector2)ge.meta.screenSize;

    if (r >= 1) {
      auto m = (f32)LOGICAL_RESOLUTION.x * (r - 1) / 2.0f;
      ge.meta._screenToLogicalScale
        = {(f32)(LOGICAL_RESOLUTION.x + 2 * m) / s.x, (f32)LOGICAL_RESOLUTION.y / s.y};
      ge.meta._screenToLogicalAdd = {-m, 0};
    }
    else {
      auto m = (f32)LOGICAL_RESOLUTION.y * (1.0f / r - 1) / 2.0f;
      ge.meta._screenToLogicalScale
        = {(f32)LOGICAL_RESOLUTION.x / s.x, ((f32)LOGICAL_RESOLUTION.y + 2 * m) / s.y};
      ge.meta._screenToLogicalAdd = {0, -m};
    }
  }
}

#define IsKeyDown(key_) (ge.meta._keyboardState[(SDL_SCANCODE_##key_)])
#define IsKeyPressed(key_) (ge.meta._keyboardStatePressed[(SDL_SCANCODE_##key_)])
#define IsKeyReleased(key_) (ge.meta._keyboardStateReleased[(SDL_SCANCODE_##key_)])

// Values: L, M, R, X1, X2.
#define IsMouseDown(button_) (ge.meta._mouseState & (SDL_BUTTON_##button_##MASK))
#define IsMousePressed(button_) \
  (ge.meta._mouseStatePressed & (SDL_BUTTON_##button_##MASK))
#define IsMouseReleased(button_) \
  (ge.meta._mouseStateReleased & (SDL_BUTTON_##button_##MASK))

///
Vector2 GetTouchScreenPos(TouchID id) {
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id) {
      return ge.meta._touches[i]._screenPos;
    }
  }
  INVALID_PATH;  // Not found.
  return {};
}

///
Vector2 GetTouchScreenDPos(TouchID id) {
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id)
      return ge.meta._touches[i]._screenDPos;
  }
  INVALID_PATH;  // Not found.
  return {};
}

///
void SetTouchUserData(TouchID id, u64 userData) {
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id) {
      ge.meta._touches[i]._userData = userData;
      return;
    }
  }
  INVALID_PATH;  // Not found.
}

///
u64 GetTouchUserData(TouchID id) {
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id)
      return ge.meta._touches[i]._userData;
  }
  INVALID_PATH;  // Not found.
  return 0;
}

Vector2 GetMouseScreenPos() {
  return ge.meta._mousePos;
}

///
void EngineApplyVignette() {
  DrawTexture({
    .texId = glib->vignette_texture_id(),
    .pos   = LOGICAL_RESOLUTION / 2,
    .color = ge.settings.vignetteAndStripsColor,
  });
}

///
void EngineApplyStrips() {
  f32 stripWidth  = 2.002f;
  f32 stripHeight = 2.002f;

  const auto color = *(u32*)&ge.settings.vignetteAndStripsColor;

  _PosColorVertex quadVertices[8]{};

  auto shouldDrawStrips = true;

  const auto r = ge.meta.screenToLogicalRatio;
  if (r > 1) {
    // Window is too wide. Draw left and right strips.
    stripWidth = 1 - 1 / r + 0.002f;

    quadVertices[0] = {-1.001f, -1.001f, 0, color};               // Top-left.
    quadVertices[1] = {-1.001f + stripWidth, -1.001f, 0, color};  // Top-right.
    quadVertices[2] = {-1.001f, 1.001f, 0, color};                // Bottom-left.
    quadVertices[3] = {-1.001f + stripWidth, 1.001f, 0, color};   // Bottom-right.
    quadVertices[4] = {1.001f - stripWidth, -1.001f, 0, color};   // Top-left.
    quadVertices[5] = {1.001f, -1.001f, 0, color};                // Top-right.
    quadVertices[6] = {1.001f - stripWidth, 1.001f, 0, color};    // Bottom-left.
    quadVertices[7] = {1.001f, 1.001f, 0, color};                 // Bottom-right.
  }
  else if (r < 1) {
    // Window is too high. Draw bottom and top strips.
    stripHeight = 1 - r + 0.002f;

    quadVertices[0] = {-1.001f, -1.001f, 0, color};                // Top-left.
    quadVertices[1] = {1.001f, -1.001f, 0, color};                 // Top-right.
    quadVertices[2] = {-1.001f, -1.001f + stripHeight, 0, color};  // Bottom-left.
    quadVertices[3] = {1.001f, -1.001f + stripHeight, 0, color};   // Bottom-right.
    quadVertices[4] = {-1.001f, 1.001f, 0, color};                 // Top-left.
    quadVertices[5] = {1.001f, 1.001f, 0, color};                  // Top-right.
    quadVertices[6] = {-1.001f, 1.001f - stripHeight, 0, color};   // Bottom-left.
    quadVertices[7] = {1.001f, 1.001f - stripHeight, 0, color};    // Bottom-right.
  }
  else
    shouldDrawStrips = false;

  if (shouldDrawStrips) {
    const u16 quadIndices[] = {0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6};

    bgfx::TransientVertexBuffer tvb{};
    bgfx::TransientIndexBuffer  tib{};
    bgfx::allocTransientVertexBuffer(&tvb, 8, _PosColorVertex::layout);
    bgfx::allocTransientIndexBuffer(&tib, 12);
    {
      memcpy(tvb.data, quadVertices, sizeof(quadVertices));
      memcpy(tib.data, quadIndices, sizeof(quadIndices));

      bgfx::setVertexBuffer(0, &tvb);
      bgfx::setIndexBuffer(&tib);
      bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA);

      bgfx::submit(0, ge.meta.programDefaultQuad);
    }
  }
}

///
const char* TextFormat(const char* text, ...) {
  // Maximum number of static buffers for text formatting.
#ifndef MAX_TEXTFORMAT_BUFFERS
#  define MAX_TEXTFORMAT_BUFFERS 4
#endif

// Maximum size of static text buffer.
#ifndef MAX_TEXT_BUFFER_LENGTH
#  define MAX_TEXT_BUFFER_LENGTH 1024
#endif

  // We create an array of buffers so strings
  // don't expire until MAX_TEXTFORMAT_BUFFERS invocations.
  static char buffers[MAX_TEXTFORMAT_BUFFERS][MAX_TEXT_BUFFER_LENGTH] = {0};
  static int  index                                                   = 0;

  char* currentBuffer = buffers[index];
  memset(currentBuffer, 0, MAX_TEXT_BUFFER_LENGTH);  // Clear buffer before using.

  va_list args;
  va_start(args, text);
  int requiredByteCount = vsnprintf(currentBuffer, MAX_TEXT_BUFFER_LENGTH, text, args);
  va_end(args);

  // If requiredByteCount is larger than the MAX_TEXT_BUFFER_LENGTH, then overflow occured
  if (requiredByteCount >= MAX_TEXT_BUFFER_LENGTH) {
    // Inserting "..." at the end of the string to mark as truncated
    char* truncBuffer
      = buffers[index] + MAX_TEXT_BUFFER_LENGTH - 4;  // Adding 4 bytes = "...\0"
    sprintf(truncBuffer, "...");
  }

  index += 1;  // Move to next buffer for next function call
  if (index >= MAX_TEXTFORMAT_BUFFERS)
    index = 0;

  return currentBuffer;
}

///
