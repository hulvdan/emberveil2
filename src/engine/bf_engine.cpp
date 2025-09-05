#pragma once

#if BF_DEBUG
#  define STB_IMAGE_WRITE_IMPLEMENTATION
#  include "stb_image_write.h"
#endif

constexpr auto SQRT_2        = 1.41421356237f;
constexpr auto SQRT_2_OVER_2 = 0.70710678f;

f32 Vector2Length(Vector2 v) {  ///
  return glm::length(v);
}

f32 Vector2LengthSqr(Vector2 v) {  ///
  return v.x * v.x + v.y * v.y;
}

f32 Vector2Dot(Vector2 v1, Vector2 v2) {  ///
  return glm::dot(v1, v2);
}

f32 Vector2Distance(Vector2 v1, Vector2 v2) {  ///
  return Vector2Length(v2 - v1);
}

f32 Vector2DistanceSqr(Vector2 v1, Vector2 v2) {  ///
  return Vector2LengthSqr(v2 - v1);
}

f32 Vector2Angle(Vector2 v1, Vector2 v2) {  ///
  auto a1 = atan2f(v1.y, v1.x);
  auto a2 = atan2f(v2.y, v2.x);
  return a2 - a1;
}

Vector2 Vector2Normalize(Vector2 v) {  ///
  return glm::normalize(v);
}

Vector2 Vector2Lerp(Vector2 v1, Vector2 v2, f32 amount) {  ///
  return v1 + (v2 - v1) * amount;
}

Vector2 Vector2Reflect(Vector2 v, Vector2 normal) {  ///
  return glm::reflect(v, normal);
}

Vector2 Vector2Rotate(Vector2 v, f32 angle) {  ///
  auto c = cosf(angle);
  auto s = sinf(angle);
  return {v.x * c - v.y * s, v.x * s + v.y * c};
}

bool Vector2Equals(Vector2 v1, Vector2 v2) {  ///
  return FloatEquals(v1.x, v2.x) && FloatEquals(v1.y, v2.y);
}

bool Vector3Equals(Vector3 v1, Vector3 v2) {  ///
  return FloatEquals(v1.x, v2.x) && FloatEquals(v1.y, v2.y) && FloatEquals(v1.z, v2.z);
}

Vector2 Vector2MoveTowards(Vector2 v, Vector2 target, f32 maxDistance) {  ///
  if (Vector2Equals(v, target))
    return target;

  auto d = Vector2Normalize(target - v);
  v.x    = MoveTowards(v.x, target.x, d.x * maxDistance);
  v.y    = MoveTowards(v.y, target.y, d.y * maxDistance);
  return v;
}

Vector2 Vector2Invert(Vector2 v) {  ///
  return {1.0f / v.x, 1.0f / v.y};
}

Vector2 Vector2Clamp(Vector2 v, Vector2 min, Vector2 max) {  ///
  return glm::clamp(v, min, max);
}

Vector2 Vector2ClampValue(Vector2 v, f32 min, f32 max) {  ///
  return glm::clamp(v, min, max);
}

constexpr Vector2 Vector2Zero() {  ///
  return Vector2{0, 0};
}

constexpr Vector2 Vector2Half() {  ///
  return Vector2{0.5f, 0.5f};
}

constexpr Vector2 Vector2One() {  ///
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

  Color operator*(const Color& v) {  ///
    Color result{
      (u8)(v.r * r / 255),
      (u8)(v.g * g / 255),
      (u8)(v.b * b / 255),
      (u8)(v.a * a / 255),
    };
    return result;
  }
};

Color ColorFromRGB(u32 color) {  ///
  Color color_{
    .r = (u8)((color & (0xff << 16)) >> 16),
    .g = (u8)((color & (0xff << 8)) >> 8),
    .b = (u8)((color & (0xff << 0)) >> 0),
    .a = 255,
  };
  return color_;
}

u32 ColorToRGBA(Color color) {  ///
  u32 value = 0;
  value += (u32)color.r << 24;
  value += (u32)color.g << 16;
  value += (u32)color.b << 8;
  value += (u32)color.a << 0;
  return value;
}

constexpr Color WHITE   = Color{};
constexpr Color BLACK   = Color{0, 0, 0, u8_max};
constexpr Color GRAY    = Color{u8_max / 2, u8_max / 2, u8_max / 2, u8_max};
constexpr Color RED     = Color{u8_max, 0, 0, u8_max};
constexpr Color GREEN   = Color{0, u8_max, 0, u8_max};
constexpr Color BLUE    = Color{0, 0, u8_max, u8_max};
constexpr Color YELLOW  = Color{u8_max, u8_max, 0, u8_max};
constexpr Color CYAN    = Color{0, u8_max, u8_max, u8_max};
constexpr Color MAGENTA = Color{u8_max, 0, u8_max, u8_max};

Color Darken(Color value, f32 p) {  ///
  ASSERT(p >= 0);
  ASSERT(p <= 1);
  return {
    .r = (u8)((f32)value.g * p),
    .g = (u8)((f32)value.g * p),
    .b = (u8)((f32)value.b * p),
    .a = value.a,
  };
}

Color Fade(Color value, f32 p) {  ///
  ASSERT(p >= 0);
  ASSERT(p <= 1);
  value.a = (u8)(255 * Clamp01(p));
  return value;
}

constexpr Vector2Int ASSETS_REFERENCE_RESOLUTION = {1920, 1080};
constexpr Vector2Int LOGICAL_RESOLUTION          = {1280, 720};
constexpr f32        ASSETS_TO_LOGICAL_RATIO     = 1280.0f / 1920.0f;

struct Texture2D {
  Vector2Int          size   = {};
  bgfx::TextureHandle handle = {};
};

const BFGame::GameLibrary* glib = nullptr;

struct _PosColorTexVertex {  ///
  f32 x, y, z;
  u32 abgr;
  f32 u, v;

  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout _PosColorTexVertex::layout;

struct _PosColorVertex {  ///
  f32 x, y, z;
  u32 abgr;

  static bgfx::VertexLayout layout;
};

bgfx::VertexLayout _PosColorVertex::layout;

struct Shader {
  bgfx::ProgramHandle program              = {};
  bgfx::VertexLayout  vertexLayout         = {};
  u64                 additionalStateFlags = {};
};

enum DeviceType : int {
  DeviceType_DESKTOP,
  DeviceType_MOBILE,
  DeviceType_COUNT,
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

struct Margins {
  f32 left, right, top, bottom = {};
};

struct DrawTextureData {
  int     texId         = -1;
  f32     rotation      = {};
  Vector2 pos           = {};
  Vector2 anchor        = Vector2Half();
  Vector2 scale         = {1, 1};
  Margins sourceMargins = {0, 0};
  Color   color         = WHITE;
  // bgfx::ProgramHandle program = {};
  // int materialsBufferStart = -1;
};

struct DrawCircleData {
  // TODO: not only lines.
  Vector2 pos    = {};
  f32     radius = {};
  Color   color  = WHITE;
};

struct DrawRectData {
  Vector2 pos    = {};
  Vector2 size   = {};
  Vector2 anchor = Vector2Half();
  Color   color  = {};
};

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
  f32  outlineAdvance  = {};
};

struct DrawTextData {
  Vector2 pos = {};
  // TODO: Vector2 scale = {1, 1};
  Vector2     anchor     = Vector2Half();
  const Font* font       = {};
  const char* text       = {};
  int         bytesCount = {};
  Color       color      = WHITE;
  // TODO: Color outlineColor = BLACK + shader.
};

enum RenderCommandType {
  RenderCommandType_INVALID,
  RenderCommandType_TEXTURE,
  RenderCommandType_CIRCLE,
  RenderCommandType_RECT,
  RenderCommandType_STRIPS,
  RenderCommandType_CIRCLE_LINES,
  RenderCommandType_RECT_LINES,
  RenderCommandType_TEXT,
};

struct RenderGroup {
  RenderZ z                  = RenderZ_DEFAULT;
  f32     sortY              = f32_inf;
  int     commandsCount      = 0;
  int     commandsStartIndex = -1;
};

struct RenderCommand {
  RenderCommandType type = RenderCommandType_INVALID;

  union {
    DrawTextureData texture;
    DrawCircleData  circle;
    DrawRectData    rect;
    DrawTextData    text;
  } _u;

  auto& DataTexture() const {  ///
    ASSERT(type == RenderCommandType_TEXTURE);
    return _u.texture;
  }

  auto& DataCircle() const {  ///
    auto allowed
      = (type == RenderCommandType_CIRCLE) || (type == RenderCommandType_CIRCLE_LINES);
    ASSERT(allowed);
    return _u.circle;
  }

  auto& DataRect() const {  ///
    auto allowed
      = (type == RenderCommandType_RECT) || (type == RenderCommandType_RECT_LINES);
    ASSERT(allowed);
    return _u.rect;
  }

  auto& DataText() const {  ///
    ASSERT(type == RenderCommandType_TEXT);
    return _u.text;
  }
};

struct EngineData {
  struct Meta {
    i64 frame = 0;

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

    Arena _arena = {};

    DeviceType deviceType = DeviceType_DESKTOP;

    Vector2 _screenToLogicalScale = {};
    Vector2 _screenToLogicalAdd   = {};

    int localization = 1;  // en.

    struct SoundManager {
      ma_sound* sounds[ARRAY_COUNT(g_sounds)];
      int*      soundPlayedIndicesPerVariation[ARRAY_COUNT(g_sounds)];
      ma_engine engine = {};
    } _soundManager = {};

    bool ysdkLoaded = false;
    bool paused     = false;

    bool debugEnabled = false;
  } meta;

  struct Settings {
    Color     backgroundColor = BLACK;
    Color     screenFadeColor = BLACK;
    f32       screenFade      = 0;
    View<u64> bgfxDisabledCapabilities{};

    struct {
      bool                     setup      = {};
      const char*              build      = {};
      const char*              gameKey    = {};
      const char*              gameSecret = {};
      std::vector<std::string> currencies = {};
      std::vector<std::string> resources  = {};
    } gameanalytics;

    size_t additionalArenaSize = 0;
  } settings;

  struct Render {
    Vector<RenderCommand> commands          = {};
    Vector<RenderGroup>   groups            = {};
    int                   currentGroupIndex = -1;
    bool                  flushedThisFrame  = false;
  } render;
} ge = {};

// clang-format off
#ifdef BF_PLATFORM_WebYandex
EM_JS(void, js_YandexReady, (), {
  window.ysdk.features.LoadingAPI.ready();
});
#endif
// clang-format on

void GameReady() {  ///
#ifdef BF_PLATFORM_WebYandex
  js_YandexReady();
#endif
}

ma_sound* PlaySound(Sound sound) {  ///
  auto& params = g_sounds[sound];

  auto  variation = Rand() % params.variations;
  auto& varIndex = ge.meta._soundManager.soundPlayedIndicesPerVariation[sound][variation];
  auto  index    = variation * params.pool + varIndex;
  ASSERT(index < params.variations * params.pool);
  auto& s = ge.meta._soundManager.sounds[sound][index];

  if (ma_sound_is_playing(&s))
    return nullptr;

  ma_sound_seek_to_pcm_frame(&s, 0);

  if (params.volume != 1.0f)
    ma_sound_set_volume(&s, params.volume);

  if (params.pitchMin != 1.0f) {
    auto t = FRand();
    t      = EaseInOutQuad(t);
    ma_sound_set_pitch(&s, Lerp(params.pitchMin, params.pitchMax, t));
  }

  ma_sound_start(&s);
  IncrementSetZeroOn(&varIndex, params.pool);

  return &ge.meta._soundManager.sounds[sound][index];
}

void StopSound(ma_sound* sound) {  ///
  if (sound)
    ma_sound_stop(sound);
}

BF_FORCE_INLINE void RenderGroup_Begin(RenderZ z) {  ///
  ASSERT(ge.render.currentGroupIndex == -1);
  ge.render.currentGroupIndex = ge.render.groups.count;
  *ge.render.groups.Add()     = {.z = z, .commandsStartIndex = ge.render.commands.count};
}

BF_FORCE_INLINE void RenderGroup_SetSortY(f32 value) {  ///
  auto& group = ge.render.groups[ge.render.currentGroupIndex];

  if (group.sortY == f32_inf)
    group.sortY = value;
  else
    INVALID_PATH;
}

enum RenderCommandSetSortY {
  RenderCommandSetSortY_DO_NOTHING,
  RenderCommandSetSortY_SET_BASELINE,
};

BF_FORCE_INLINE void RenderGroup_CommandTexture(
  DrawTextureData       data,
  RenderCommandSetSortY setSortY = RenderCommandSetSortY_DO_NOTHING
) {  ///
  ASSERT(data.sourceMargins.left >= 0);
  ASSERT(data.sourceMargins.right >= 0);
  ASSERT(data.sourceMargins.top >= 0);
  ASSERT(data.sourceMargins.bottom >= 0);
  ASSERT(data.sourceMargins.left <= 1);
  ASSERT(data.sourceMargins.right <= 1);
  ASSERT(data.sourceMargins.top <= 1);
  ASSERT(data.sourceMargins.bottom <= 1);

  auto mx = data.sourceMargins.left + data.sourceMargins.right;
  auto my = data.sourceMargins.bottom + data.sourceMargins.top;
  ASSERT(mx >= 0);
  ASSERT(my >= 0);

  if ((mx >= 1) || (my >= 1))
    return;

  if (setSortY == RenderCommandSetSortY_SET_BASELINE) {
    auto tex    = glib->atlas_textures()->Get(data.texId);
    auto height = (f32)tex->size_y();
    auto off    = ASSETS_TO_LOGICAL_RATIO * data.scale.y
               * ((f32)tex->baseline() - (f32)tex->size_y() * data.anchor.y);
    RenderGroup_SetSortY(data.pos.y + off);
  }

  ge.render.groups[ge.render.currentGroupIndex].commandsCount++;
  *ge.render.commands.Add() = {
    .type = RenderCommandType_TEXTURE,
    ._u{.texture = data},
  };
}

BF_FORCE_INLINE void RenderGroup_CommandCircle(  ///
  DrawCircleData        data,
  RenderCommandSetSortY setSortY = RenderCommandSetSortY_DO_NOTHING
) {
  if (setSortY == RenderCommandSetSortY_SET_BASELINE)
    RenderGroup_SetSortY(data.pos.y - data.radius);

  ge.render.groups[ge.render.currentGroupIndex].commandsCount++;
  *ge.render.commands.Add() = {
    .type = RenderCommandType_CIRCLE,
    ._u{.circle = data},
  };
}

BF_FORCE_INLINE void RenderGroup_CommandRect(  ///
  DrawRectData          data,
  RenderCommandSetSortY setSortY = RenderCommandSetSortY_DO_NOTHING
) {
  if (setSortY == RenderCommandSetSortY_SET_BASELINE)
    RenderGroup_SetSortY(data.pos.y - data.size.y * (data.anchor.y - 0.5f));

  ge.render.groups[ge.render.currentGroupIndex].commandsCount++;
  *ge.render.commands.Add() = {
    .type = RenderCommandType_RECT,
    ._u{.rect = data},
  };
}

BF_FORCE_INLINE void RenderGroup_CommandCircleLines(DrawCircleData data) {  ///
  ge.render.groups[ge.render.currentGroupIndex].commandsCount++;
  *ge.render.commands.Add() = {
    .type = RenderCommandType_CIRCLE_LINES,
    ._u{.circle = data},
  };
}

BF_FORCE_INLINE void RenderGroup_CommandRectLines(DrawRectData data) {  ///
  ge.render.groups[ge.render.currentGroupIndex].commandsCount++;
  *ge.render.commands.Add() = {
    .type = RenderCommandType_RECT_LINES,
    ._u{.rect = data},
  };
}

BF_FORCE_INLINE void RenderGroup_CommandText(DrawTextData data) {  ///
  ge.render.groups[ge.render.currentGroupIndex].commandsCount++;
  *ge.render.commands.Add() = {
    .type = RenderCommandType_TEXT,
    ._u{.text = data},
  };
}

BF_FORCE_INLINE void RenderGroup_End() {  ///
  const auto& group           = ge.render.groups[ge.render.currentGroupIndex];
  ge.render.currentGroupIndex = -1;

  if (!group.commandsCount) {
    ge.render.groups.count--;
    return;
  }

  ASSERT(group.sortY != f32_inf);
}

BF_FORCE_INLINE void RenderGroup_OneShotTexture(DrawTextureData data, RenderZ z) {  ///
  RenderGroup_Begin(z);
  RenderGroup_CommandTexture(data, RenderCommandSetSortY_SET_BASELINE);
  RenderGroup_End();
}

BF_FORCE_INLINE void RenderGroup_OneShotCircle(DrawCircleData data, RenderZ z) {  ///
  RenderGroup_Begin(z);
  RenderGroup_CommandCircle(data, RenderCommandSetSortY_SET_BASELINE);
  RenderGroup_End();
}

BF_FORCE_INLINE void RenderGroup_OneShotCircleLines(DrawCircleData data) {  ///
  RenderGroup_Begin(RenderZ_GIZMOS);
  RenderGroup_SetSortY(0);
  RenderGroup_CommandCircleLines(data);
  RenderGroup_End();
}

BF_FORCE_INLINE void RenderGroup_OneShotRect(DrawRectData data, RenderZ z) {  ///
  RenderGroup_Begin(z);
  RenderGroup_CommandRect(data, RenderCommandSetSortY_SET_BASELINE);
  RenderGroup_End();
}

BF_FORCE_INLINE void RenderGroup_OneShotRectLines(DrawRectData data) {  ///
  RenderGroup_Begin(RenderZ_GIZMOS);
  RenderGroup_SetSortY(0);
  RenderGroup_CommandRectLines(data);
  RenderGroup_End();
}

// NOTE: Ignores Y!
BF_FORCE_INLINE void RenderGroup_OneShotText(DrawTextData data, RenderZ z) {  ///
  RenderGroup_Begin(z);
  RenderGroup_SetSortY(0);
  RenderGroup_CommandText(data);
  RenderGroup_End();
}

BF_FORCE_INLINE int _RenderGroupCmp(const RenderGroup* v1, const RenderGroup* v2) {  ///
  ASSERT(v1->z > RenderZ_INVALID);
  ASSERT(v2->z > RenderZ_INVALID);
  ASSERT(v1->z < RenderZ_COUNT);
  ASSERT(v2->z < RenderZ_COUNT);

  const auto y1 = v1->sortY;
  const auto y2 = v2->sortY;

  auto cmp = 0;
  if (v1->z > v2->z)
    cmp = 1;
  else if (v1->z < v2->z)
    cmp = -1;
  else if (y1 > y2)
    cmp = -1;
  else if (y1 < y2)
    cmp = 1;

  if (cmp == 0) {
    const auto& cmd1 = ge.render.commands[v1->commandsStartIndex];
    const auto& cmd2 = ge.render.commands[v2->commandsStartIndex];
    if ((cmd1.type == RenderCommandType_TEXTURE)  //
        && (cmd2.type == RenderCommandType_TEXTURE))
      cmp = (cmd1.DataTexture().pos.x > cmd2.DataTexture().pos.x ? 1 : -1);
    else if (cmd1.type == RenderCommandType_TEXTURE)
      cmp = 1;
    else if (cmd2.type == RenderCommandType_TEXTURE)
      cmp = -1;
  }

  return cmp;
}

// clang-format off
static const u8 _utf8d[364]{  ///
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

u32 _UTF8Decode(u32* _state, u8 _ch, u32* _codep) {  ///
  u32 byte = _ch;
  u32 type = _utf8d[byte];
  *_codep  = (*_state != 0) ? (byte & 0x3fu) | (*_codep << 6) : (0xff >> type) & (byte);
  *_state  = _utf8d[256 + *_state + type];
  return *_state;
}

BF_FORCE_INLINE void IterateOverCodepoints(
  const char*                      text,
  int                              bytesCount,
  auto&& /* void(u32 codepoint) */ lambda
) noexcept {  ///
  ASSERT(bytesCount > 0);
  ASSERT(text);

  u32  codepoint{};
  u32  state{};
  auto remaining = bytesCount + 1;

  auto p = text;
  for (; *p; ++p) {
    remaining--;
    if (remaining <= 0)
      break;
    if (_UTF8Decode(&state, *(u8*)p, &codepoint))
      continue;

    lambda(codepoint);
  }
  lambda(0);
  ASSERT_FALSE(state);  // The string is not well-formed.
}

//
void FlushRenderCommands() {
  ZoneScoped;

  ASSERT_FALSE(ge.render.flushedThisFrame);
  ge.render.flushedThisFrame = true;

  // Setup. {  ///
  ASSERT(ge.render.currentGroupIndex == -1);

  qsort(
    (void*)ge.render.groups.base,
    ge.render.groups.count,
    sizeof(RenderGroup),
    (int (*)(const void*, const void*))_RenderGroupCmp
  );

  Shader shaders_[]{
    {
      .program              = ge.meta.programDefaultTexture,
      .vertexLayout         = _PosColorTexVertex::layout,
      .additionalStateFlags = 0,
    },
    {
      .program              = ge.meta.programDefaultQuad,
      .vertexLayout         = _PosColorVertex::layout,
      .additionalStateFlags = 0,
    },
    {
      .program              = ge.meta.programDefaultQuad,
      .vertexLayout         = _PosColorVertex::layout,
      .additionalStateFlags = BGFX_STATE_PT_LINES,
    },
    {
      .program              = ge.meta.programDefaultTexture,
      .vertexLayout         = _PosColorTexVertex::layout,
      .additionalStateFlags = 0,
    },
  };
  VIEW_FROM_ARRAY_DANGER(shaders);

  u16 drawCallIndicesCounts_[512];
  int drawCallVerticesCounts_[512];
  VIEW_FROM_ARRAY_DANGER(drawCallIndicesCounts);
  VIEW_FROM_ARRAY_DANGER(drawCallVerticesCounts);

  bgfx::TransientVertexBuffer tvb{};
  bgfx::TransientIndexBuffer  tib{};
  // }

  FOR_RANGE (int, mode, 2) {
    // 0 - calculating size of index / vertex buffers.
    // 1 - allocating and filling them.

    int  drawCallIndicesCount  = 0;
    int  drawCallVerticesCount = 0;
    bool addedSomethingToDraw  = false;
    int  drawCallIndex         = 0;

    const Font*   font   = nullptr;
    const Shader* shader = nullptr;

    LAMBDA (void, endShader, (int mode)) {  ///
      if (!mode) {
        drawCallIndicesCounts[drawCallIndex]  = drawCallIndicesCount;
        drawCallVerticesCounts[drawCallIndex] = drawCallVerticesCount;
      }
      else {
        ASSERT(drawCallIndicesCounts[drawCallIndex] == drawCallIndicesCount);
        ASSERT(drawCallVerticesCounts[drawCallIndex] == drawCallVerticesCount);

        if (font)
          bgfx::setTexture(0, ge.meta.uniformTexture, font->atlasTexture.handle);
        else {
          // TODO: don't bind for shaders that don't need it.
          bgfx::setTexture(0, ge.meta.uniformTexture, ge.meta.atlas.handle);
        }

        bgfx::setVertexBuffer(0, &tvb);
        bgfx::setIndexBuffer(&tib);
        bgfx::setState(
          BGFX_STATE_WRITE_RGB | BGFX_STATE_BLEND_ALPHA | shader->additionalStateFlags
        );
        bgfx::submit(0, shader->program);

        tvb = {};
        tib = {};
      }
    };

    for (auto& group : ge.render.groups) {
      ASSERT(group.z < RenderZ_COUNT);
      ASSERT(group.commandsStartIndex >= 0);
      ASSERT(group.commandsCount > 0);

      FOR_RANGE (int, commandIndex, group.commandsCount) {
        // Setup. {  ///
        const auto& command = ge.render.commands[group.commandsStartIndex + commandIndex];

        const Font*   newFont   = nullptr;
        const Shader* newShader = nullptr;

        if (command.type == RenderCommandType_TEXTURE)
          newShader = &shaders[0];
        else if (command.type == RenderCommandType_RECT)
          newShader = &shaders[1];
        else if (command.type == RenderCommandType_STRIPS) {
          if (ge.meta.screenToLogicalRatio == 1)
            continue;
          newShader = &shaders[1];
        }
        else if (command.type == RenderCommandType_CIRCLE_LINES)
          newShader = &shaders[2];
        else if (command.type == RenderCommandType_RECT_LINES)
          newShader = &shaders[2];
        else if (command.type == RenderCommandType_TEXT) {
          newShader = &shaders[3];
          newFont   = command.DataText().font;
        }
        else
          INVALID_PATH;

        if ((shader != newShader) || (font != newFont)) {
          if (addedSomethingToDraw) {
            endShader(mode);
            drawCallIndicesCount  = 0;
            drawCallVerticesCount = 0;
            drawCallIndex++;
          }

          shader = newShader;
          font   = newFont;

          if (mode == 1) {
            bgfx::allocTransientVertexBuffer(
              &tvb, drawCallVerticesCounts[drawCallIndex], shader->vertexLayout
            );
            bgfx::allocTransientIndexBuffer(&tib, drawCallIndicesCounts[drawCallIndex]);
          }
        }
        // }

        switch (command.type) {
        case RenderCommandType_TEXTURE: {  ///
          if (!mode) {
            drawCallIndicesCount += 6;
            drawCallVerticesCount += 4;
          }
          else {
            const auto& data = command.DataTexture();
            ASSERT(data.texId >= 0);

            const auto tex = glib->atlas_textures()->Get(data.texId);

            const Rect sourceRec{
              .pos{
                (f32)tex->atlas_x() + (f32)tex->size_x() * data.sourceMargins.left,
                (f32)tex->atlas_y() + (f32)tex->size_y() * data.sourceMargins.bottom,
              },
              .size{
                (f32)tex->size_x()
                  * (1 - data.sourceMargins.right - data.sourceMargins.left),
                (f32)tex->size_y()
                  * (1 - data.sourceMargins.top - data.sourceMargins.bottom),
              },
            };
            Rect destRec{
              .pos{
                data.pos.x
                  + (f32)tex->size_x() * data.sourceMargins.left * abs(data.scale.x)
                      * ge.meta.screenScale * data.anchor.x,
                data.pos.y
                  + (f32)tex->size_y() * data.sourceMargins.bottom * abs(data.scale.y)
                      * ge.meta.screenScale * data.anchor.y,
              },
              .size{
                (f32)tex->size_x()
                  // * (1 - data.sourceMargins.left - data.sourceMargins.right)
                  * abs(data.scale.x) * ge.meta.screenScale,
                (f32)tex->size_y()
                  // * (1 - data.sourceMargins.bottom - data.sourceMargins.top)
                  * abs(data.scale.y) * ge.meta.screenScale,
              },
            };
            destRec.pos -= LOGICAL_RESOLUTION / 2;

            // TODO: add / subtract epsilon?
            auto sx0 = sourceRec.pos.x;
            auto sx1 = sx0 + sourceRec.size.x;
            auto sy0 = sourceRec.pos.y;
            auto sy1 = sy0 + sourceRec.size.y;
            sx0 += 0.7f;
            sx1 -= 0.7f;
            sy0 += 0.7f;
            sy1 -= 0.7f;
            sx0 /= (f32)ge.meta.atlas.size.x;
            sx1 /= (f32)ge.meta.atlas.size.x;
            sy0 /= (f32)ge.meta.atlas.size.y;
            sy1 /= (f32)ge.meta.atlas.size.y;
            if (data.scale.x < 0) {
              auto t = sx0;
              sx0    = sx1;
              sx1    = t;
            }
            if (data.scale.y < 0) {
              auto t = sy0;
              sy0    = sy1;
              sy1    = t;
            }

            Vector2 topLeft{};
            Vector2 topRight{};
            Vector2 bottomLeft{};
            Vector2 bottomRight{};

            auto dsx
              = destRec.size.x * (1 - data.sourceMargins.left - data.sourceMargins.right);
            auto dsy
              = destRec.size.y * (1 - data.sourceMargins.bottom - data.sourceMargins.top);

            if (data.rotation == 0.0f) {
              auto dx0 = destRec.pos.x;
              auto dx1 = destRec.pos.x + dsx;
              auto dy0 = destRec.pos.y;
              auto dy1 = destRec.pos.y + dsy;

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

              topRight.x = destRec.pos.x + (dx + dsx) * cosRotation - dy * sinRotation;
              topRight.y = destRec.pos.y + (dx + dsx) * sinRotation + dy * cosRotation;

              bottomLeft.x = destRec.pos.x + dx * cosRotation - (dy + dsy) * sinRotation;
              bottomLeft.y = destRec.pos.y + dx * sinRotation + (dy + dsy) * cosRotation;

              bottomRight.x
                = destRec.pos.x + (dx + dsx) * cosRotation - (dy + dsy) * sinRotation;
              bottomRight.y
                = destRec.pos.y + (dx + dsx) * sinRotation + (dy + dsy) * cosRotation;
            }

            topLeft.x /= (f32)LOGICAL_RESOLUTION.x / 2.0f;
            topRight.x /= (f32)LOGICAL_RESOLUTION.x / 2.0f;
            bottomLeft.x /= (f32)LOGICAL_RESOLUTION.x / 2.0f;
            bottomRight.x /= (f32)LOGICAL_RESOLUTION.x / 2.0f;
            topLeft.y /= (f32)LOGICAL_RESOLUTION.y / 2.0f;
            topRight.y /= (f32)LOGICAL_RESOLUTION.y / 2.0f;
            bottomLeft.y /= (f32)LOGICAL_RESOLUTION.y / 2.0f;
            bottomRight.y /= (f32)LOGICAL_RESOLUTION.y / 2.0f;

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

            const int quadIndices[]{0, 1, 2, 1, 3, 2};
            for (const auto index : quadIndices) {
              ((u16*)tib.data)[drawCallIndicesCount++]
                = (u16)(index + drawCallVerticesCount);
            }

            const auto               color = *(u32*)&data.color;
            const _PosColorTexVertex quadVertices[]{
              {topLeft.x, topLeft.y, 0.0f, color, sx0, sy1},
              {topRight.x, topRight.y, 0.0f, color, sx1, sy1},
              {bottomLeft.x, bottomLeft.y, 0.0f, color, sx0, sy0},
              {bottomRight.x, bottomRight.y, 0.0f, color, sx1, sy0},
            };
            for (const auto& vertex : quadVertices)
              ((_PosColorTexVertex*)tvb.data)[drawCallVerticesCount++] = vertex;
          }

          ASSERT_FALSE(drawCallIndicesCount % 6);
          ASSERT_FALSE(drawCallVerticesCount % 4);
        } break;

        case RenderCommandType_CIRCLE: {  ///
          NOT_IMPLEMENTED;
        } break;

        case RenderCommandType_RECT: {  ///
          if (!mode) {
            drawCallIndicesCount += 6;
            drawCallVerticesCount += 4;
          }
          else {
            const auto& data = command.DataRect();

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

            const auto color = *(u32*)&data.color;

            const int quadIndices[]{0, 1, 2, 1, 3, 2};
            for (const auto index : quadIndices) {
              ((u16*)tib.data)[drawCallIndicesCount++]
                = (u16)(index + drawCallVerticesCount);
            }

            const _PosColorVertex quadVertices[]{
              {points[0].x, points[0].y, 0.0f, color},
              {points[1].x, points[1].y, 0.0f, color},
              {points[2].x, points[2].y, 0.0f, color},
              {points[3].x, points[3].y, 0.0f, color},
            };
            for (const auto& vertex : quadVertices)
              ((_PosColorVertex*)tvb.data)[drawCallVerticesCount++] = vertex;
          }
        } break;

        case RenderCommandType_CIRCLE_LINES: {  ///
          if (!mode) {
            drawCallIndicesCount += 16;
            drawCallVerticesCount += 8;
          }
          else {
            const auto& data = command.DataCircle();

            const auto& centerX = data.pos.x;
            const auto& centerY = data.pos.y;
            const auto& radius  = data.radius;

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

            const int quadIndices[]{0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 0};
            for (const auto index : quadIndices) {
              ((u16*)tib.data)[drawCallIndicesCount++]
                = (u16)(index + drawCallVerticesCount);
            }

            const auto            color = *(u32*)&data.color;
            const _PosColorVertex quadVertices[]{
              {points[0].x, points[0].y, 0.0f, color},
              {points[1].x, points[1].y, 0.0f, color},
              {points[2].x, points[2].y, 0.0f, color},
              {points[3].x, points[3].y, 0.0f, color},
              {points[4].x, points[4].y, 0.0f, color},
              {points[5].x, points[5].y, 0.0f, color},
              {points[6].x, points[6].y, 0.0f, color},
              {points[7].x, points[7].y, 0.0f, color},
            };
            for (const auto& vertex : quadVertices)
              ((_PosColorVertex*)tvb.data)[drawCallVerticesCount++] = vertex;
          }
        } break;

        case RenderCommandType_RECT_LINES: {  ///
          if (!mode) {
            drawCallIndicesCount += 8;
            drawCallVerticesCount += 4;
          }
          else {
            const auto& data = command.DataRect();

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

            const int quadIndices[]{0, 1, 1, 2, 2, 3, 3, 0};
            for (const auto index : quadIndices) {
              ((u16*)tib.data)[drawCallIndicesCount++]
                = (u16)(index + drawCallVerticesCount);
            }

            auto                  color = *(u32*)&data.color;
            const _PosColorVertex quadVertices[]{
              {points[0].x, points[0].y, 0.0f, color},
              {points[1].x, points[1].y, 0.0f, color},
              {points[2].x, points[2].y, 0.0f, color},
              {points[3].x, points[3].y, 0.0f, color},
            };
            for (const auto& vertex : quadVertices)
              ((_PosColorVertex*)tvb.data)[drawCallVerticesCount++] = vertex;
          }
        } break;

        case RenderCommandType_TEXT: {  ///
          const auto& data = command.DataText();

          ASSERT(data.bytesCount > 0);
          ASSERT(font);
          if (!font->loaded) {
            LOGE("RenderCommandType_TEXT: font is not loaded!");
            INVALID_PATH;
            break;
          }

          if (!mode) {
            IterateOverCodepoints(
              data.text,
              data.bytesCount,
              [&font, &drawCallIndicesCount, &drawCallVerticesCount](u32 codepoint)
                BF_FORCE_INLINE_LAMBDA {
                  if (!codepoint)
                    return;
                  if (codepoint == (u32)'\n')
                    return;

                  drawCallIndicesCount += 6;
                  drawCallVerticesCount += 4;
                }
            );
          }
          else {
            auto& font = data.font;
            auto  pos  = data.pos;

            // Processing `anchor`.
            {
              f32 width    = 0;
              f32 maxWidth = 0;
              int height   = 1;
              IterateOverCodepoints(
                data.text,
                data.bytesCount,
                [&maxWidth, &width, &height, &font](u32 codepoint)
                  BF_FORCE_INLINE_LAMBDA {
                    if (!codepoint) {
                      maxWidth = MAX(maxWidth, width);
                      return;
                    }
                    if (codepoint == (u32)'\n') {
                      maxWidth = MAX(maxWidth, width);
                      width    = 0;
                      height++;
                      return;
                    }

                    auto glyphIndex = ArrayBinaryFind(
                      font->codepoints, font->codepointsCount, (int)codepoint
                    );
                    ASSERT(glyphIndex >= 0);

                    f32                y_{};
                    stbtt_aligned_quad q{};
                    stbtt_GetPackedQuad(
                      font->chars,
                      font->atlasTexture.size.x,
                      font->atlasTexture.size.y,
                      glyphIndex,
                      &width,
                      // &data.pos.y,
                      &y_,
                      &q,
                      1  // 1=opengl & d3d10+,0=d3d9
                    );
                  }
              );
              pos.x -= maxWidth * data.anchor.x;
              pos.y
                += (f32)height
                   * ((f32)font->size * font->FIXME_sizeScale + 2 * (f32)font->outlineWidth)
                   * data.anchor.y;
            }

            auto y = LOGICAL_RESOLUTION.y - pos.y;

            const auto startPosX = pos.x;

            IterateOverCodepoints(
              data.text,
              data.bytesCount,
              [&](u32 codepoint) BF_FORCE_INLINE_LAMBDA {
                if (!codepoint)
                  return;
                if (codepoint == (u32)'\n') {
                  pos.x = startPosX;
                  y += (f32)font->size * font->FIXME_sizeScale
                       + 2 * (f32)font->outlineWidth;
                  return;
                }

                auto glyphIndex = ArrayBinaryFind(
                  font->codepoints, font->codepointsCount, (int)codepoint
                );
                ASSERT(glyphIndex >= 0);

                stbtt_aligned_quad q{};
                stbtt_GetPackedQuad(
                  font->chars,
                  font->atlasTexture.size.x,
                  font->atlasTexture.size.y,
                  glyphIndex,
                  &pos.x,
                  // &pos.y,
                  &y,
                  &q,
                  1  // 1=opengl & d3d10+,0=d3d9
                );

                q.x0 -= font->outlineWidth;
                q.x1 += font->outlineWidth;
                q.y0 -= font->outlineWidth;
                q.y1 += font->outlineWidth;

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

                const int quadIndices[]{0, 1, 2, 1, 3, 2};
                for (const auto index : quadIndices) {
                  ((u16*)tib.data)[drawCallIndicesCount++]
                    = (u16)(index + drawCallVerticesCount);
                }

                const auto               color = *(u32*)&data.color;
                const _PosColorTexVertex quadVertices[]{
                  {topLeft.x, topLeft.y, 0.0f, color, sx0, sy1},
                  {topRight.x, topRight.y, 0.0f, color, sx1, sy1},
                  {bottomLeft.x, bottomLeft.y, 0.0f, color, sx0, sy0},
                  {bottomRight.x, bottomRight.y, 0.0f, color, sx1, sy0},
                };
                for (const auto& vertex : quadVertices)
                  ((_PosColorTexVertex*)tvb.data)[drawCallVerticesCount++] = vertex;
              }
            );
          }
        } break;

        case RenderCommandType_STRIPS: {  ///
          const auto r = ge.meta.screenToLogicalRatio;

          if (!mode) {
            if (r > 1 || r < 1) {
              drawCallIndicesCount += 12;
              drawCallVerticesCount += 8;
            }
          }
          else {
            auto color = *(u32*)&ge.settings.backgroundColor;
            if (BF_DEBUG_VIGNETTE_AND_STRIPS)
              color = *(u32*)&GREEN;

            _PosColorVertex quadVertices[8]{};

            auto shouldDrawStrips = true;

            const auto    tex = glib->atlas_textures()->Get(glib->vignette_texture_id());
            constexpr f32 VIGNETTE_SIZE = 28.0f;

            if (r > 1) {
              // Window is too wide. Draw left and right strips.
              const auto vignetteSize = VIGNETTE_SIZE / (f32)tex->size_x() / r;
              const auto stripSize    = 1 - 1 / r + 0.002f - vignetteSize;

              // 1. Top-left.
              quadVertices[0] = {-1.001f, -1.001f, 0, color};
              // 1. Top-right.
              quadVertices[1] = {-1.001f + stripSize, -1.001f, 0, color};
              // 1. Bottom-left.
              quadVertices[2] = {-1.001f, 1.001f, 0, color};
              // 1. Bottom-right.
              quadVertices[3] = {-1.001f + stripSize, 1.001f, 0, color};

              // 2. Top-left.
              quadVertices[4] = {1.001f - stripSize, -1.001f, 0, color};
              // 2. Top-right.
              quadVertices[5] = {1.001f, -1.001f, 0, color};
              // 2. Bottom-left.
              quadVertices[6] = {1.001f - stripSize, 1.001f, 0, color};
              // 2. Bottom-right.
              quadVertices[7] = {1.001f, 1.001f, 0, color};
            }
            else if (r < 1) {
              // Window is too high. Draw bottom and top strips.
              const auto vignetteSize = VIGNETTE_SIZE / (f32)tex->size_y() * r;
              const auto stripSize    = 1 - r + 0.002f - vignetteSize;

              // 1. Top-left.
              quadVertices[0] = {-1.001f, -1.001f, 0, color};
              // 1. Top-right.
              quadVertices[1] = {1.001f, -1.001f, 0, color};
              // 1. Bottom-left.
              quadVertices[2] = {-1.001f, -1.001f + stripSize, 0, color};
              // 1. Bottom-right.
              quadVertices[3] = {1.001f, -1.001f + stripSize, 0, color};

              // 2. Top-left.
              quadVertices[4] = {-1.001f, 1.001f, 0, color};
              // 2. Top-right.
              quadVertices[5] = {1.001f, 1.001f, 0, color};
              // 2. Bottom-left.
              quadVertices[6] = {-1.001f, 1.001f - stripSize, 0, color};
              // 2. Bottom-right.
              quadVertices[7] = {1.001f, 1.001f - stripSize, 0, color};
            }
            else
              shouldDrawStrips = false;

            if (shouldDrawStrips) {
              const int quadIndices[]{0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6};
              for (const auto index : quadIndices) {
                ((u16*)tib.data)[drawCallIndicesCount++]
                  = (u16)(index + drawCallVerticesCount);
              }

              for (const auto& vertex : quadVertices)
                ((_PosColorVertex*)tvb.data)[drawCallVerticesCount++] = vertex;
            }
          }
        } break;
        }

        addedSomethingToDraw = true;
      }
    }

    if (addedSomethingToDraw)
      endShader(mode);
  }

  ge.render.commands.Reset();
  ge.render.groups.Reset();
}

void _OnTouchDown(Touch touch) {  ///
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

void _OnTouchUp(Touch touch) {  ///
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

void _OnTouchMoved(Touch touch) {  ///
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

void ResetPressedReleasedStates() {  ///
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

bool IsTouchPressed(TouchID id) {  ///
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id) {
      auto& t = ge.meta._touches[i];
      return t._state & _TouchDataState_PRESSED;
    }
  }
  INVALID_PATH;  // Not found.
  return false;
}

bool IsTouchReleased(TouchID id) {  ///
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id) {
      auto& t = ge.meta._touches[i];
      return t._state & _TouchDataState_RELEASED;
    }
  }
  INVALID_PATH;  // Not found.
  return false;
}

bool IsTouchDown(TouchID id) {  ///
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id) {
      auto& t = ge.meta._touches[i];
      return t._state & _TouchDataState_DOWN;
    }
  }
  return false;
}

TEST_CASE ("Touch controls") {  ///
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

void* LoadFileData(const char* filepath, int* out_size = nullptr) {  ///
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

void UnloadFileData(void* ptr) {  ///
  free(ptr);
}

bgfx::ShaderHandle _LoadShader(const u8* data, u32 size) {  ///
  return bgfx::createShader(bgfx::makeRef(data, size));
}

View<TouchID> GetTouchIDs() {  ///
  return {.count = ge.meta._touchIDs.count, .base = ge.meta._touchIDs.base};
}

bgfx::ProgramHandle
LoadProgram(const u8* vsh, u32 sizeVsh, const u8* fsh, u32 sizeFsh) {  ///
  return bgfx::createProgram(
    _LoadShader(vsh, sizeVsh),
    _LoadShader(fsh, sizeFsh),
    /* destroyShaders */ true
  );
}

Texture2D _LoadTexture(const char* filepath, Vector2Int size) {  ///
  ZoneScopedN("_LoadTexture()");
  LOGI("Loading texture '%s'...", filepath);

  Texture2D result{.size = size};

  int channels = 0;
  int dataSize = 0;

  void* data = nullptr;
  {
    ZoneScopedN("LoadFileData()");
    data = LoadFileData(filepath, &dataSize);
  }
  ASSERT(data);

  struct {
    basist::transcoder_texture_format from                = {};
    bgfx::TextureFormat::Enum         to                  = {};
    int                               bytesPerPixel       = 1;
    int                               divideBytesPerPixel = 1;
  } formatPairs[]{
    {
      .from = basist::transcoder_texture_format::cTFBC7_RGBA,
      .to   = bgfx::TextureFormat::BC7,
    },
    {
      .from = basist::transcoder_texture_format::cTFBC3_RGBA,
      .to   = bgfx::TextureFormat::BC3,
    },
    {
      .from                = basist::transcoder_texture_format::cTFPVRTC1_4_RGBA,
      .to                  = bgfx::TextureFormat::PTC14A,
      .divideBytesPerPixel = 2,
    },
    {
      .from                = basist::transcoder_texture_format::cTFPVRTC1_4_RGBA,
      .to                  = bgfx::TextureFormat::PTC14A,
      .divideBytesPerPixel = 2,
    },
    {
      .from = basist::transcoder_texture_format::cTFETC2_RGBA,
      .to   = bgfx::TextureFormat::ETC2A,
    },
    {
      .from          = basist::transcoder_texture_format::cTFRGBA32,
      .to            = bgfx::TextureFormat::RGBA8,
      .bytesPerPixel = 4,
    },
  };

  auto success = false;
  {
    ZoneScopedN("Transcoding texture");

    for (const auto pair : formatPairs) {
      LOGI(
        "Trying to transcode '%s' to '%s' format...",
        filepath,
        basist::basis_get_format_name(pair.from)
      );

      {
        ZoneScopedN("bgfx::isTextureValid()");

        if (!bgfx::isTextureValid(1, false, 1, pair.to, 0)) {
          LOGI(
            "'%s' is not supported on this platform",
            basist::basis_get_format_name(pair.from)
          );
          continue;
        }
      }

      basist::ktx2_transcoder transcoder{};
      transcoder.init(data, dataSize);

      // if (!transcoder.validate_file_checksums(data, dataSize, false))
      //   INVALID_PATH;
      // if (!transcoder.validate_header(data, dataSize))
      //   INVALID_PATH;
      if (!transcoder.start_transcoding())
        INVALID_PATH;

      ASSERT_FALSE(transcoder.is_video());

      basist::ktx2_image_level_info levelInfo{};
      if (!transcoder.get_image_level_info(levelInfo, 0, 0, 0))
        INVALID_PATH;

      auto outDataSize = (size_t)levelInfo.m_total_blocks
                         * (size_t)levelInfo.m_block_width
                         * (size_t)levelInfo.m_block_height * (size_t)pair.bytesPerPixel
                         / (size_t)pair.divideBytesPerPixel;
      auto outData = (u8*)malloc(outDataSize);

      bool ok = false;
      {
        ZoneScopedN("basist. transcoder.transcode_image_level()");
        ok = transcoder.transcode_image_level(
          0,  // level_index
          0,  // layer_index
          0,  // face_index
          (void*)outData,
          (u32)outDataSize,
          pair.from
        );
      }

      if (ok) {
        ZoneScopedN("bgfx. bgfx::createTexture2D()");

        result.handle = bgfx::createTexture2D(
          size.x,
          size.y,
          false /* _hasMips*/,
          1,
          pair.to,
          BGFX_SAMPLER_MIN_ANISOTROPIC      //
            | BGFX_SAMPLER_MAG_ANISOTROPIC  //
            | BGFX_SAMPLER_MIP_POINT        //
            | BGFX_SAMPLER_U_CLAMP          //
            | BGFX_SAMPLER_V_CLAMP,
          bgfx::copy(outData, outDataSize)
        );

        success = true;
        break;
      }
      else
        INVALID_PATH;

      free(outData);
    }
  }

  ASSERT(success);

  UnloadFileData(data);

  LOGI("Loaded texture '%s'!", filepath);

  return result;
}

void _UnloadTexture(Texture2D* texture) {  ///
  bgfx::destroy(texture->handle);
  *texture = {};
}

constexpr f32 ScaleToFit(Vector2 inner, Vector2 container) {  ///
  f32 scaleX = container.x / inner.x;
  f32 scaleY = container.y / inner.y;
  f32 scale  = (scaleX < scaleY) ? scaleX : scaleY;
  return scale;
}

TEST_CASE ("ScaleToFit") {  ///
  ASSERT(FloatEquals(ScaleToFit({1, 1}, {2, 2}), 2));
  ASSERT(FloatEquals(ScaleToFit({1, 1}, {3, 2}), 2));
  ASSERT(FloatEquals(ScaleToFit({1, 1}, {2, 3}), 2));
  ASSERT(FloatEquals(ScaleToFit({3, 3}, {2, 3}), 2.0f / 3.0f));
}

constexpr f32 ScaleToCover(Vector2 inner, Vector2 container) {  ///
  f32 scaleX = container.x / inner.x;
  f32 scaleY = container.y / inner.y;
  f32 scale  = (scaleX > scaleY) ? scaleX : scaleY;
  return scale;
}

TEST_CASE ("ScaleToCover") {  ///
  ASSERT(FloatEquals(ScaleToCover({1, 1}, {2, 2}), 2));
  ASSERT(FloatEquals(ScaleToCover({1, 1}, {3, 2}), 3));
  ASSERT(FloatEquals(ScaleToCover({1, 1}, {2, 3}), 3));
  ASSERT(FloatEquals(ScaleToCover({3, 3}, {2, 3}), 1));
}

void LoadGamelib() {  ///
  glib = BFGame::GetGameLibrary(LoadFileData("resources/gamelib.bin"));
}

void InitEngine() {  ///
  ZoneScopedN("InitEngine");

  ge.meta._keyboardState = SDL_GetKeyboardState(&ge.meta._keyboardStateCount);

  size_t arenaSize = 3 * sizeof(bool) * ge.meta._keyboardStateCount;
  for (auto& params : g_sounds) {
    arenaSize += sizeof(ma_sound) * params.pool * params.variations;
    arenaSize += sizeof(int) * params.variations;
  }
  ge.meta._arena = MakeArena(arenaSize + ge.settings.additionalArenaSize);

  if (Sound_COUNT) {  // Not initializing audio if there is no sounds in project.
    auto config = ma_engine_config_init();
    if (ma_engine_init(&config, &ge.meta._soundManager.engine) != MA_SUCCESS)
      LOGW("Failed to init miniaudio engine");
    else {
      LOGI("miniaudio engine initialized");

      FOR_RANGE (int, i, ARRAY_COUNT(g_sounds)) {
        ge.meta._soundManager.sounds[i] = ALLOCATE_ZEROS_ARRAY(
          &ge.meta._arena, ma_sound, g_sounds[i].pool * g_sounds[i].variations
        );
      }
      FOR_RANGE (int, i, ARRAY_COUNT(g_sounds)) {
        ge.meta._soundManager.soundPlayedIndicesPerVariation[i]
          = ALLOCATE_ZEROS_ARRAY(&ge.meta._arena, int, g_sounds[i].variations);
      }

      int soundTypeIndex = 0;
      for (const auto params : g_sounds) {
        auto flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC;
        if (params.pitchMin == params.pitchMax)
          flags |= MA_SOUND_FLAG_NO_PITCH;

        FOR_RANGE (int, variation, params.variations) {
          FOR_RANGE (int, poolInnerIndex, params.pool) {
            if (!poolInnerIndex) {
              // Loading original.
              auto soundPath = params.pathVariations[variation];
              auto error     = ma_sound_init_from_file(
                &ge.meta._soundManager.engine,
                soundPath,
                flags,
                nullptr,
                nullptr,
                &ge.meta._soundManager
                   .sounds[soundTypeIndex][variation * params.pool + poolInnerIndex]
              );
              if (error != MA_SUCCESS) {
                LOGE("Failed to load sound %s: error code %d", soundPath, error);
                break;
              }
            }
            else {
              // Loading copies.
              ma_sound_init_copy(
                &ge.meta._soundManager.engine,
                &ge.meta._soundManager.sounds[soundTypeIndex][variation * params.pool],
                flags,
                nullptr,
                &ge.meta._soundManager
                   .sounds[soundTypeIndex][variation * params.pool + poolInnerIndex]
              );
            }
          }
        }

        soundTypeIndex++;
      }
    }
  }

#if !defined(SDL_PLATFORM_EMSCRIPTEN)
  if (ge.settings.gameanalytics.setup) {
    const auto& a = ge.settings.gameanalytics;
    gameanalytics::GameAnalytics::setEnabledInfoLog(true);
    gameanalytics::GameAnalytics::setEnabledVerboseLog(BF_DEBUG);
    gameanalytics::GameAnalytics::configureBuild(a.build);
    gameanalytics::GameAnalytics::configureAvailableResourceCurrencies(a.currencies);
    gameanalytics::GameAnalytics::configureAvailableResourceItemTypes(a.resources);
    gameanalytics::GameAnalytics::initialize(a.gameKey, a.gameSecret);
  }
#endif

  {
    ZoneScopedN("basist. basist::basisu_transcoder_init()");
    basist::basisu_transcoder_init();
  }

  ge.meta._touches.Reserve(8);
  ge.meta._touchIDs.Reserve(8);
  ge.meta._keyboardStatePrev
    = ALLOCATE_ZEROS_ARRAY(&ge.meta._arena, bool, ge.meta._keyboardStateCount);
  ge.meta._keyboardStatePressed
    = ALLOCATE_ZEROS_ARRAY(&ge.meta._arena, bool, ge.meta._keyboardStateCount);
  ge.meta._keyboardStateReleased
    = ALLOCATE_ZEROS_ARRAY(&ge.meta._arena, bool, ge.meta._keyboardStateCount);

  LoadGamelib();
  ge.meta.atlas = _LoadTexture(
    "resources/atlas_d2.basis", {glib->atlas_size_x(), glib->atlas_size_y()}
  );
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

void DeinitEngine() {  ///
  FOR_RANGE (int, i, ARRAY_COUNT(g_sounds)) {
    FOR_RANGE (int, k, g_sounds[i].pool) {
      ma_sound_uninit(&ge.meta._soundManager.sounds[i][k]);
    }
  }
  ma_engine_uninit(&ge.meta._soundManager.engine);
}

void UnloadFont(Font* font) {  ///
  ZoneScoped;

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

  int outlineWidth   = 0;  // Optional.
  f32 outlineAdvance = 0;  // Optional.
};

Font LoadFont(LoadFontData data) {  ///
  ZoneScoped;

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
    .outlineAdvance  = data.outlineAdvance,
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

  {
    ZoneScopedN("stbtt_PackFontRanges()");

    if (!stbtt_PackFontRanges(&context, font.fileData, 0, &range, 1)) {
      LOGE("stbtt_PackFontRanges failed");
      INVALID_PATH;
      return {};
    }
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
    ZoneScopedN("Outlining");

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
      c.xadvance += data.outlineAdvance;
    }

    free(dist_);
  }

#if BF_DEBUG & !defined(SDL_PLATFORM_EMSCRIPTEN)
  {
    ZoneScopedN("stbi_write_png");
    stbi_write_png("debugFontAtlas.png", atlasSize.x, atlasSize.y, 4, atlasData, 0);
  }
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

i64 GetTicks() {  ///
  return ge.meta.ticks;
}

f64 GetTime() {  ///
  return (f64)ge.meta.ticks / 1000.0;
}

f32 FrameTime() {  ///
  return (f32)(ge.meta.frameTime - ge.meta.prevFrameTime);
}

void EngineOnFrameStart() {
  ge.render.flushedThisFrame = false;
  ge.meta.ticks              = (i64)SDL_GetTicks();
  ge.meta.prevFrameTime      = ge.meta.frameTime;
  ge.meta.frameTime          = GetTime();

  constexpr auto ratioLogical  = (f32)LOGICAL_RESOLUTION.x / (f32)LOGICAL_RESOLUTION.y;
  auto           ratioActual   = (f32)ge.meta.screenSize.x / (f32)ge.meta.screenSize.y;
  ge.meta.screenToLogicalRatio = ratioActual / ratioLogical;

  // Controls. Keyboard.
  {  ///
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

  // Controls. Mouse.
  {  ///
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

  // Caching ScreenToLogical data.
  {  ///
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

bool IsKeyDown(SDL_Scancode key) {  ///
  return ge.meta._keyboardState[key];
}

bool IsKeyPressed(SDL_Scancode key) {  ///
  return ge.meta._keyboardStatePressed[key];
}

bool IsKeyReleased(SDL_Scancode key) {  ///
  return ge.meta._keyboardStateReleased[key];
}

// Values: L, M, R, X1, X2.
#define IsMouseDown(button_) (ge.meta._mouseState & (SDL_BUTTON_##button_##MASK))
#define IsMousePressed(button_) \
  (ge.meta._mouseStatePressed & (SDL_BUTTON_##button_##MASK))
#define IsMouseReleased(button_) \
  (ge.meta._mouseStateReleased & (SDL_BUTTON_##button_##MASK))

Vector2 GetTouchScreenPos(TouchID id) {  ///
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id) {
      return ge.meta._touches[i]._screenPos;
    }
  }
  INVALID_PATH;  // Not found.
  return {};
}

Vector2 GetTouchScreenDPos(TouchID id) {  ///
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id)
      return ge.meta._touches[i]._screenDPos;
  }
  INVALID_PATH;  // Not found.
  return {};
}

void SetTouchUserData(TouchID id, u64 userData) {  ///
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id) {
      ge.meta._touches[i]._userData = userData;
      return;
    }
  }
  INVALID_PATH;  // Not found.
}

u64 GetTouchUserData(TouchID id) {  ///
  FOR_RANGE (int, i, ge.meta._touches.count) {
    if (ge.meta._touchIDs[i] == id)
      return ge.meta._touches[i]._userData;
  }
  INVALID_PATH;  // Not found.
  return 0;
}

Vector2 GetMouseScreenPos() {  ///
  return ge.meta._mousePos;
}

void EngineApplyVignette() {  ///
  RenderGroup_OneShotTexture(
    {
      .texId = glib->vignette_texture_id(),
      .pos   = LOGICAL_RESOLUTION / 2,
      .scale{4.01f, 4.01f},
      .color = (BF_DEBUG_VIGNETTE_AND_STRIPS ? RED : ge.settings.backgroundColor),
    },
    RenderZ_VIGNETTE
  );
}

void EngineApplyStrips() {  ///
  RenderGroup_Begin(RenderZ_VIGNETTE);
  RenderGroup_SetSortY(0);
  ge.render.groups[ge.render.currentGroupIndex].commandsCount++;
  *ge.render.commands.Add() = {.type = RenderCommandType_STRIPS};
  RenderGroup_End();
}

const char* TextFormat(const char* text, ...) {  ///
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

void GameInit();
void GameReady();
void GameFixedUpdate();
void GameDraw();

SDL_AppResult EngineUpdate() {  ///
  ZoneScoped;

  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    GameInit();
    GameReady();
  }

  static f32 frameTime = 0.0f;
  frameTime += FrameTime();

  if (IsKeyDown(SDL_SCANCODE_F1) && IsKeyPressed(SDL_SCANCODE_F2))
    ge.meta.debugEnabled = !ge.meta.debugEnabled;

  if (ge.meta.debugEnabled) {
    if (IsKeyPressed(SDL_SCANCODE_F3))
      IncrementSetZeroOn(&ge.meta.localization, 2);

    if (IsKeyPressed(SDL_SCANCODE_F4))
      IncrementSetZeroOn((int*)&ge.meta.deviceType, (int)DeviceType_COUNT);
  }

  int simulated = 0;

  while (frameTime >= FIXED_DT) {
    frameTime -= FIXED_DT;

    GameFixedUpdate();

    ResetPressedReleasedStates();
    ge.meta.frame++;

    // Skipping frames if there are too many of them.
    if (simulated++ >= FIXED_FPS / 2) {
      frameTime = 0;
      break;
    }
  }

  GameDraw();

  return SDL_APP_CONTINUE;
}

///
