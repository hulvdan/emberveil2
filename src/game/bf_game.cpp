// John Carmack on Inlined Code.
// http://number-none.com/blow/blog/programming/2014/09/26/carmack-on-inlined-code.html
//
// «The real enemy addressed by inlining is unexpected dependency and mutation of state.»
//
// If you are going to make a lot of state changes, having them all happen inline does
// have advantages; you should be made constantly aware of the full horror of what you are
// doing. When it gets to be too much to take, figure out how to factor blocks out into
// pure functions.
//
// To sum up:
//
// * If a function is only called from a single place, consider inlining it.
//
// * If a function is called from multiple places, see if it is possible to arrange
//   for the work to be done in a single place, perhaps with flags, and inline that.
//
// * If there are multiple versions of a function,
//   consider making a single function with more, possibly defaulted, parameters.
//
// * If the work is close to purely functional, with few references to global state,
//   try to make it completely functional.
//
// * Try to use const on both parameters and functions when the function
//   really must be used in multiple places.
//
// * Minimize control flow complexity and "area under ifs", favoring consistent
//   execution paths and times over "optimally" avoiding unnecessary work.

#pragma once

#include "box2d/box2d.h"

#include "bf_constants.cpp"

b2Vec2 ToB2Vec2(Vector2 value) {  ///
  return {value.x, value.y};
}

Vector2 ToVector2(b2Vec2 value) {  ///
  return {value.x, value.y};
}

const char* GetWindowTitle() {  ///
  return "The Game"
#if BF_DEBUG
         " [DEBUG]"
#endif
#if BF_PROFILING && defined(DOCTEST_CONFIG_DISABLE)
         " [PROFILING]"
#endif
#if BF_ENABLE_ASSERTS
         " [ASSERTS]"
#endif
    ;
}

// g_gameVersion { ///
static const char* const g_gameVersion = BF_VERSION
#if BF_DEBUG
  " [DEBUG]"
#endif
#if BF_PROFILING && defined(DOCTEST_CONFIG_DISABLE)
  " [PROFILING]"
#endif
#if BF_ENABLE_ASSERTS
  " [ASSERTS]"
#endif
  ;
// }

enum ControlsContext {  ///
  ControlsContext_INVALID,
  ControlsContext_COUNT,
};

enum ShapeCategory : u32 {  ///
  ShapeCategory_STATIC   = 1 << 0,
  ShapeCategory_PLAYER   = 1 << 1,
  ShapeCategory_CREATURE = 1 << 2,
  // ShapeCategory_FOOT     = 1 << 3,
  // ShapeCategory_PROJECTILE = 1 << 3,
};

enum BodyType : u32 {  ///
  BodyType_INVALID,
  BodyType_STATIC,
  BodyType_CREATURE,
};

enum ShapeUserDataType : u32 {  ///
  ShapeUserDataType_INVALID,
  ShapeUserDataType_STATIC,
  ShapeUserDataType_CREATURE,
};

struct ShapeUserData {  ///
  ShapeUserDataType type   = {};
  int               _value = {};

  static ShapeUserData Static() {
    return {.type = ShapeUserDataType_STATIC};
  }

  static ShapeUserData Creature(int value) {
    ASSERT(value >= 0);
    return {
      .type   = ShapeUserDataType_CREATURE,
      ._value = value,
    };
  }

  int GetCreatureID() const {
    ASSERT(type == ShapeUserDataType_CREATURE);
    return _value;
  }

  static ShapeUserData FromPointer(void* ptr) {
    static_assert((sizeof(ShapeUserData) == 4) || (sizeof(ShapeUserData) == 8));

    if (sizeof(void*) == 8) {
      // 64-bit.
      return *(ShapeUserData*)&ptr;
    }
    else {
      // 32-bit. Little endian.
      auto p = (u8*)&ptr;
      u8   type[4]{};
      u8   value[4]{};

      type[0]  = p[0];
      value[0] = p[1];
      value[1] = p[2];
      value[2] = p[3];

      return {
        .type   = *(ShapeUserDataType*)type,
        ._value = *(int*)value,
      };
    }
  }

  void* ToPointer() {
    if (sizeof(void*) == 8) {
      // 64-bit.
      return *(void**)this;
    }
    else {
      // 32-bit. Little endian.
      u8 value[4]{};
      value[0] = ((u8*)&type)[0];
      value[1] = ((u8*)&_value)[0];
      value[2] = ((u8*)&_value)[1];
      value[3] = ((u8*)&_value)[2];

      return *(void**)value;
    }
  }
};

struct Body {  ///
  int      createdID = {};
  b2BodyId id        = {};
};

enum BodyShapeType {  ///
  BodyShapeType_INVALID,
  BodyShapeType_CIRCLE,
  BodyShapeType_RECT,
};

struct BodyShape {  ///
  bool          active = true;
  Body          body   = {};
  BodyShapeType type   = {};
  Color         color  = WHITE;

  union {
    struct {
      f32 radius = {};
    } _circle;

    struct {
      Vector2 size = {};
    } _rect;
  } _u;

  auto& DataCircle() {
    ASSERT(type == BodyShapeType_CIRCLE);
    return _u._circle;
  }

  const auto& DataCircle() const {
    ASSERT(type == BodyShapeType_CIRCLE);
    return _u._circle;
  }

  auto& DataRect() {
    ASSERT(type == BodyShapeType_RECT);
    return _u._rect;
  }

  const auto& DataRect() const {
    ASSERT(type == BodyShapeType_RECT);
    return _u._rect;
  }
};

struct MakeBodyData {  ///
  BodyType      type     = {};
  bool          isSensor = false;
  f32           density  = 1.0f;
  ShapeUserData userData = {};
  bool          isPlayer = {};
};

struct Passenger {  ///
  int     color     = 0;
  Vector2 offVisual = {};

  operator bool() const {
    return color > 0;
  }
};

using PassengerRow = Array<Passenger, 3>;

struct Zone {  ///
  Vector2Int                     posi = {};
  PushableArray<PassengerRow, 5> rows = {};

  Vector2 pos() const {
    return Vector2(posi) + Vector2Half();
  }

  Rect Rect() const {
    return {
      .pos  = pos(),
      .size = ToVector2(glib->zone_size()),
    };
  }
};

const Color ZONE_COLORS_[]{
  ColorFromRGBA(0xfb6b1dff), ColorFromRGBA(0xe83b3bff), ColorFromRGBA(0x831c5dff),
  ColorFromRGBA(0xc32454ff), ColorFromRGBA(0xf04f78ff), ColorFromRGBA(0xf68181ff),
  ColorFromRGBA(0xfca790ff), ColorFromRGBA(0xe3c896ff), ColorFromRGBA(0xab947aff),
  ColorFromRGBA(0x966c6cff), ColorFromRGBA(0x625565ff), ColorFromRGBA(0x3e3546ff),
  ColorFromRGBA(0x0b5e65ff), ColorFromRGBA(0x0b8a8fff), ColorFromRGBA(0x1ebc73ff),
  ColorFromRGBA(0x91db69ff), ColorFromRGBA(0xfbff86ff), ColorFromRGBA(0xfbb954ff),
  ColorFromRGBA(0xcd683dff), ColorFromRGBA(0x9e4539ff), ColorFromRGBA(0x7a3045ff),
  ColorFromRGBA(0x6b3e75ff), ColorFromRGBA(0x905ea9ff), ColorFromRGBA(0xa884f3ff),
  ColorFromRGBA(0xeaadedff), ColorFromRGBA(0x8fd3ffff), ColorFromRGBA(0x4d9be6ff),
  ColorFromRGBA(0x4d65b4ff), ColorFromRGBA(0x484a77ff), ColorFromRGBA(0x30e1b9ff),
  ColorFromRGBA(0x8ff8e2ff),
};
VIEW_FROM_ARRAY_DANGER(ZONE_COLORS);

enum PlayerAction {  ///
  PlayerAction_NONE,
  PlayerAction_PICKUP,
  PlayerAction_EXCHANGE,
  PlayerAction_PUT,
};

struct GameData {
  struct Meta {
    Font            fontUI      = {};
    LoadFontsResult loadedFonts = {};

    Vector2 screenSizeUI       = {};
    Vector2 screenSizeUIMargin = {};

    bool playerUsesKeyboardOrController = false;

    struct {
      bool    controlling   = false;
      Vector2 startPos      = {};
      Vector2 targetPos     = {};
      Vector2 calculatedDir = {};
    } stickControl;

    bool reload = {};
  } meta;

  struct Save {
    i32 level = 0;
  } save;

  struct Run {
    u32 randomSeedForLevelHotReload = 0;

    Vector2Int worldSize  = {};
    Vector2    worldSizef = {};
    b2WorldId  world      = {};

    Camera camera{};

    uint        remainingRows = 0;
    FrameVisual won           = {};

    FrameVisual advanceScheduled = {};  // TODO: Transition.
    FrameVisual advancedAt       = {};

    struct Player {
      int          zone                 = -1;
      int          actionPassengerIndex = -1;
      PlayerAction action               = {};
      FrameGame    actionStartedAt      = {};
      Passenger    passenger            = {};
    } player;

    // Using "X-macros". ref: https://www.geeksforgeeks.org/c/x-macros-in-c/
    // These containers preserve allocated memory upon resetting state of the run.

    PushableArray<Zone, 15> zones = {};

#define VECTORS_TABLE X(BodyShape, bodyShapes)

#define X(type_, name_) Vector<type_> name_ = {};
    VECTORS_TABLE;
#undef X
  } run;
} g = {};

void DestroyBody(Body* body) {  ///
  b2DestroyBody(body->id);
  for (auto& shape : g.run.bodyShapes) {
    if (shape.body.createdID == body->createdID)
      shape.active = false;
  }
}

void AddBodyShape(BodyShape v) {  ///
  ASSERT(v.active);
  ASSERT(v.type);

  for (auto& shape : g.run.bodyShapes) {
    if (!shape.active) {
      shape = v;
      return;
    }
  }

  *g.run.bodyShapes.Add() = v;
}

struct MakeBodyResult {  ///
  Body       body     = {};
  b2ShapeDef shapeDef = {};
};

MakeBodyResult MakeBody(Vector2 pos, MakeBodyData data) {  ///
  ASSERT(data.type != BodyType_INVALID);
  ASSERT(data.userData.type != ShapeUserDataType_INVALID);

  b2BodyDef bodyDef = b2DefaultBodyDef();
  if (data.type == BodyType_CREATURE)
    bodyDef.type = b2_dynamicBody;
  bodyDef.position      = ToB2Vec2(pos);
  bodyDef.linearDamping = glib->nohotreload_player_linear_damping();

  b2BodyId body = b2CreateBody(g.run.world, &bodyDef);

  b2ShapeDef shapeDef = b2DefaultShapeDef();
  shapeDef.userData   = data.userData.ToPointer();
  shapeDef.isSensor   = data.isSensor;

  auto& categoryBits = shapeDef.filter.categoryBits;
  auto& maskBits     = shapeDef.filter.maskBits;
  maskBits           = ShapeCategory_STATIC | ShapeCategory_PLAYER;

  switch (data.type) {
  case BodyType_STATIC: {
    categoryBits = ShapeCategory_STATIC;
    maskBits |= ShapeCategory_CREATURE;
  } break;

  case BodyType_CREATURE: {
    categoryBits = ShapeCategory_CREATURE;
    if (data.isPlayer)
      categoryBits |= ShapeCategory_PLAYER;
  } break;

  default:
    INVALID_PATH;
  }

  shapeDef.density = data.density;

  static int     lastCreatedID = 0;
  MakeBodyResult result{
    .body{.createdID = ++lastCreatedID, .id = body},
    .shapeDef = shapeDef,
  };

  return result;
}

struct MakeRectBodyData {  ///
  Vector2      pos      = {};
  Vector2      size     = {};
  Vector2      anchor   = Vector2Half();
  f32          radius   = {};
  MakeBodyData bodyData = {};
};

Body MakeRectBody(MakeRectBodyData data) {  ///
  ASSERT(data.size.x > 0);
  ASSERT(data.size.y > 0);

  auto makeBodyResult
    = MakeBody(data.pos - (data.anchor - Vector2Half()) * data.size, data.bodyData);

  auto box = b2MakeRoundedBox(
    data.size.x / 2 - data.radius, data.size.y / 2 - data.radius, data.radius
  );
  b2CreatePolygonShape(makeBodyResult.body.id, &makeBodyResult.shapeDef, &box);

  AddBodyShape({
    .body  = makeBodyResult.body,
    .type  = BodyShapeType_RECT,
    .color = YELLOW,
    ._u{._rect{.size = data.size}},
  });

  return makeBodyResult.body;
}

struct MakeCircleBodyData {  ///
  Vector2      pos           = {};
  f32          radius        = {};
  f32          hurtboxRadius = {};
  MakeBodyData bodyData      = {};
};

Body MakeCircleBody(MakeCircleBodyData data) {  ///
  ASSERT(data.radius > 0);

  auto makeBodyResult = MakeBody(data.pos, data.bodyData);

  b2Circle circle{.radius = data.radius};
  b2CreateCircleShape(makeBodyResult.body.id, &makeBodyResult.shapeDef, &circle);

  if (data.bodyData.type == BodyType_CREATURE) {
    makeBodyResult.shapeDef.isSensor = true;
    circle.radius                    = data.hurtboxRadius;
    b2CreateCircleShape(makeBodyResult.body.id, &makeBodyResult.shapeDef, &circle);

    AddBodyShape({
      .body  = makeBodyResult.body,
      .type  = BodyShapeType_CIRCLE,
      .color = RED,
      ._u{._circle{.radius = data.hurtboxRadius}},
    });
  }

  AddBodyShape({
    .body  = makeBodyResult.body,
    .type  = BodyShapeType_CIRCLE,
    .color = YELLOW,
    ._u{._circle{.radius = data.radius}},
  });

  return makeBodyResult.body;
}

void GameLoad(const BFSave::Save* save) {  ///
  auto& s = g.save;
  s.level = save->level();
}

void GameDumpStateForSaving(BFSave::SaveT& save) {  ///
  save.level = g.save.level;
  if (g.run.won.IsSet())
    save.level += 1;
}

struct Line {  ///
  Vector2 v1 = {};
  Vector2 v2 = {};
};

struct MakeWallsData {  ///
  const View<Line> lines     = {};
  View<Body>       outBodies = {};
};

void MakeWalls(MakeWallsData data) {  ///
  ASSERT(data.lines.count > 0);
  if (data.outBodies.count)
    ASSERT(data.lines.count == data.outBodies.count);
  ASSERT(data.lines.base);

  FOR_RANGE (int, i, data.lines.count) {
    const auto& line = data.lines[i];

    auto v1 = line.v1;
    auto v2 = line.v2;

    if ((v1.x != v2.x) && (v1.y != v2.y))
      INVALID_PATH;
    if (v1 == v2)
      INVALID_PATH;

    if (v1.x > v2.x) {
      auto t = v1;
      v1     = v2;
      v2     = t;
    }
    if (v1.y > v2.y) {
      auto t = v1;
      v1     = v2;
      v2     = t;
    }

    auto body = MakeRectBody({
      .pos    = v1,
      .size   = (v2 - v1) + Vector2One(),
      .anchor = Vector2Zero(),
      .bodyData{
        .type     = BodyType_STATIC,
        .userData = ShapeUserData::Static(),
      },
    });

    if (data.outBodies.count)
      data.outBodies[i] = body;
  }
}

const auto GetFBLevel(int index, int* actualIndex = nullptr) {  ///
  const auto fb_levels = glib->levels();
  if (index >= fb_levels->size()) {
    index -= fb_levels->size();
    index = glib->cycleable_levels_indices()->Get(
      index % glib->cycleable_levels_indices()->size()
    );
  }
  if (actualIndex)
    *actualIndex = index;
  return fb_levels->Get(index);
}

void RunInit() {
  ZoneScoped;

  const auto fb_level               = GetFBLevel(g.save.level);
  g.run.randomSeedForLevelHotReload = fb_level->random_seed();

  // Consistent GRAND state.
  {  ///
    u32 val = Hash32((u8*)&g.save.level, sizeof(g.save.level)) + fb_level->random_seed();
    ge.meta.logicRand._state = Hash32((u8*)&val, sizeof(val));
  }

  // Making box2d world.
  {  ///
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity    = {0, 0};
    g.run.world         = b2CreateWorld(&worldDef);
  }

  // Making zones.
  if (fb_level->zones()) {  ///
    ASSERT_FALSE(g.run.zones.count);

    int zone = -1;
    for (auto fb_zone : *fb_level->zones()) {
      zone++;
      auto& z = *g.run.zones.Add();
      z       = {.posi = ToVector2(fb_zone->pos())};
    }

    // Filling zones with passengers.
    {
      ZoneScopedN("Filling zones with passengers.");

      int timesContinued = -1;

    zonesContinue:
      timesContinued++;
      if (timesContinued >= 10)
        INVALID_PATH;

      for (auto& z : g.run.zones) {
        z.rows.Reset();
        *z.rows.Add() = {};
        ASSERT(z.rows.count == 1);
      }

      g.run.remainingRows = (int)fb_level->zones()->size();
      if (fb_level->override_passenger_rows())
        g.run.remainingRows = fb_level->override_passenger_rows();

      // TEMPORARY!
      ASSERT(g.run.remainingRows <= (int)fb_level->zones()->size());

      FOR_RANGE (int, row, g.run.remainingRows) {
        auto& z = g.run.zones[row];
        FOR_RANGE (int, passengerIndexInRow, 3)
          z.rows[0][passengerIndexInRow] = {.color = row + 1};
      }

      // Permutations.
      FOR_RANGE (int, _, g.run.remainingRows * glib->permutations()) {
        auto& r1 = g.run.zones[GRAND.Rand() % g.run.zones.count].rows[0];
        auto& r2 = g.run.zones[GRAND.Rand() % g.run.zones.count].rows[0];
        std::swap(r1[GRAND.Rand() % 3], r2[GRAND.Rand() % 3]);
      }

      // Checking that no rows contain 3 passengers of the same color.
      for (const auto& z : g.run.zones) {
        for (const auto& r : z.rows) {
          const auto& p1 = r[0];
          const auto& p2 = r[1];
          const auto& p3 = r[2];
          if (p1 && p2 && p3 && (p1.color == p2.color) && (p2.color == p3.color))
            goto zonesContinue;
        }
      }

      LOGD("Filling zones with passengers: timesContinued %d", timesContinued);
    }
  }
}

void RunReset() {  ///
  ZoneScoped;

  const bool advanced = g.run.advanceScheduled.IsSet();
  const auto world    = g.run.world;
  b2DestroyWorld(g.run.world);

  // Resetting `g.run` to a default value,
  // while preserving allocated memory of it's Vectors.
  struct {
#define X(type_, name_) Vector<type_> name_ = g.run.name_;
    VECTORS_TABLE;
#undef X
  } temp{};

#define X(type_, name_) temp.name_.Reset();
  VECTORS_TABLE;
#undef X

  g.run = {
    .world = world,
#define X(type_, name_) .name_ = temp.name_,
    VECTORS_TABLE
#undef X
  };

  if (advanced)
    g.run.advancedAt.SetNow();
}

void GamePreInit(GamePreInitOpts opts) {  ///
  ZoneScoped;

  *opts.baseFont = &g.meta.fontUI;
}

void ReloadFontsIfNeeded() {  ///
  static bool       loaded             = false;
  static Vector2Int previousScreenSize = {};

  if ((ge.meta.screenSize.x <= 0) || (ge.meta.screenSize.y <= 0))
    return;

  // Debounce.
  {
    static int debounce = 0;
    if (previousScreenSize == ge.meta.screenSize) {
      debounce = 0;
      return;
    }
    if (loaded) {
      Clay_ResetMeasureTextCache();
      static Vector2Int lastFrameScreenSize = {};
      if (lastFrameScreenSize != ge.meta.screenSize) {
        lastFrameScreenSize = ge.meta.screenSize;
        debounce            = 0;
        return;
      }
      debounce++;
      if (debounce < FIXED_FPS / 5)
        return;
    }
    previousScreenSize = ge.meta.screenSize;
  }

  if (loaded) {
    ReloadFonts(&g.meta.loadedFonts);
    return;
  }

  static auto fontpath = "res/arialnb.ttf";
  // static int  numberCodepoints[]{
  //   ' ', '+', '-', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'x'
  // };

  static LoadFontData loadFontData_[]{
    // fontUI.
    {
      .filepath        = fontpath,
      .size            = 18,
      .FIXME_sizeScale = 45.0f / 30.0f,
      .codepoints      = g_codepoints,
      .codepointsCount = ARRAY_COUNT(g_codepoints),
    },
    // // fontUIOutlined.
    // {
    //   .filepath        = fontpath,
    //   .size            = 18,
    //   .FIXME_sizeScale = 45.0f / 30.0f,
    //   .codepoints      = g_codepoints,
    //   .codepointsCount = ARRAY_COUNT(g_codepoints),
    //   .outlineWidth    = 3,
    //   .outlineAdvance  = 1,
    // },
    // // fontUIBig.
    // {
    //   .filepath        = fontpath,
    //   .size            = 22,
    //   .FIXME_sizeScale = 45.0f / 30.0f,
    //   .codepoints      = g_codepoints,
    //   .codepointsCount = ARRAY_COUNT(g_codepoints),
    // },
    // // fontUIBigOutlined.
    // {
    //   .filepath        = fontpath,
    //   .size            = 26,
    //   .FIXME_sizeScale = 45.0f / 30.0f,
    //   .codepoints      = g_codepoints,
    //   .codepointsCount = ARRAY_COUNT(g_codepoints),
    //   .outlineWidth    = 3,
    //   .outlineAdvance  = 1,
    // },
    // // fontStats.
    // {
    //   .filepath        = fontpath,
    //   .size            = 15,
    //   .FIXME_sizeScale = 45.0f / 30.0f,
    //   .codepoints      = g_codepoints,
    //   .codepointsCount = ARRAY_COUNT(g_codepoints),
    // },
    // // // fontPricesOutlined.
    // // {
    // //   .filepath        = fontpath,
    // //   .size            = 20,
    // //   .FIXME_sizeScale = 45.0f / 30.0f,
    // //   .codepoints      = numberCodepoints,
    // //   .codepointsCount = ARRAY_COUNT(numberCodepoints),
    // //   .outlineWidth    = 3,
    // //   .outlineAdvance  = 1,
    // // },
    // // fontItemCountsOutlined.
    // {
    //   .filepath        = fontpath,
    //   .size            = 20,
    //   .FIXME_sizeScale = 45.0f / 30.0f,
    //   .codepoints      = numberCodepoints,
    //   .codepointsCount = ARRAY_COUNT(numberCodepoints),
    //   .outlineWidth    = 3,
    //   .outlineAdvance  = 1,
    // },
    // // fontUIGiganticOutlined.
    // {
    //   .filepath        = fontpath,
    //   .size            = 40 * 5 / 4,
    //   .FIXME_sizeScale = 45.0f / 30.0f,
    //   .codepoints      = g_codepoints,
    //   .codepointsCount = ARRAY_COUNT(g_codepoints),
    //   .outlineWidth    = 4,
    //   .outlineAdvance  = 0,
    // },
    // // fontUINextWave.
    // {
    //   .filepath        = fontpath,
    //   .size            = 40,
    //   .FIXME_sizeScale = 45.0f / 30.0f,
    //   .codepoints      = numberCodepoints,
    //   .codepointsCount = ARRAY_COUNT(numberCodepoints),
    //   .outlineWidth    = 3,
    //   .outlineAdvance  = 0,
    // },
  };
  VIEW_FROM_ARRAY_DANGER(loadFontData);

  loaded = true;
  g.meta.loadedFonts
    = LoadFonts({.count = loadFontData.count, .base = &g.meta.fontUI}, loadFontData);
}

void CheckGamelib() {
#if !BF_ENABLE_ASSERTS
  return;
#endif

#ifdef TESTS
  glibFile = LoadFile(GAMELIB_PATH, nullptr);
  glib     = BFGame::GetGameLibrary(glibFile);
#endif

  // Checking levels cycling and mirroring.
  if (0) {  ///
    const int expectedLevelIndices[]{
      0,   //
      1,   //
      2,   //
      3,   //
      4,   //
      5,   //
      6,   //
      7,   //
      8,   //
      9,   //
      10,  //
      11,  //
      12,  //
      13,  //
      14,  //
      15,  //
      16,  //
      17,  //
      18,  //
      19,  //
      2,   //
      3,   //
      4,   //
      6,   //
      7,   //
      8,   //
      9,   //
      10,  //
      11,  //
      12,  //
      13,  //
      14,  //
      15,  //
      16,  //
      17,  //
      18,  //
      19,  //
      2,   //
      3,   //
      4,   //
      6,   //
      7,   //
      8,   //
      9,   //
      10,  //
      11,  //
      12,  //
      13,  //
      14,  //
      15,  //
      16,  //
      17,  //
      18,  //
      19,  //
      2,   //
      3,   //
      4,   //
      6,   //
      7,   //
      8,   //
      9,   //
      10,  //
      11,  //
      12,  //
      13,  //
      14,  //
      15,  //
      16,  //
      17,  //
      18,  //
      19,  //
      2,   //
      3,   //
      4,   //
      6,   //
      7,   //
      8,   //
      9,   //
      10,  //
      11,  //
      12,  //
      13,  //
      14,  //
      15,  //
      16,  //
      17,  //
      18,  //
      19,  //
      2,   //
      3,   //
      4,   //
      6,   //
      7,   //
      8,   //
      9,   //
      10,  //
      11,  //
      12,  //
      13,  //
      14,  //
      15,  //
      16,  //
      17,  //
      18,  //
      19,  //
    };
    int i = -1;
    for (auto expected : expectedLevelIndices) {
      i++;
      int actualIndex = 0;
      GetFBLevel(i, &actualIndex);
      ASSERT(expected == actualIndex);
    }
  }

#ifdef TESTS
  glib = nullptr;
  UnloadFile(glibFile);
  glibFile = nullptr;
#endif
}

TEST_CASE ("CheckGamelib") {  ///
  CheckGamelib();
}

void GameInit() {  ///
  ZoneScoped;

  SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");

  ReloadFontsIfNeeded();
  CheckGamelib();
}

void GameInitAfterLoadingSavedata() {  ///
  ZoneScoped;

  RunInit();

  LOGI("GameInitAfterLoadingSavedata...");
  DEFER {
    LOGI("GameInitAfterLoadingSavedata... Finished!");
  };
}

// NOTE: Logic must be executed only when `ge.meta._drawing` (`draw`) is false!
// e.g. updating mouse position, processing `clicked()`,
// logically reacting to `Clay_Hovered()`, changing game's state, etc.
void DoUI() {
#define BF_UI_PRE
#include "engine/bf_clay_ui.cpp"

  auto& pl   = g.run.player;
  blockInput = g.run.advanceScheduled.IsSet();

#define GAP_SMALL (8)
#define GAP_BIG (20)

  // HUD.
  CLAY(  ///
    {
      .layout{
        BF_CLAY_SIZING_GROW_XY,
        BF_CLAY_PADDING_HORIZONTAL_VERTICAL(
          UI_PADDING_OUTER_HORIZONTAL, UI_PADDING_OUTER_VERTICAL
        ),
      },
      .floating{
        .zIndex             = zIndex,
        .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
        .attachTo           = CLAY_ATTACH_TO_PARENT,
      },
    }
  )
  CLAY({.layout{BF_CLAY_SIZING_GROW_XY}}) {
    // Current level.
    if (g.save.level) {
      CLAY({}) {
        BF_CLAY_TEXT_LOCALIZED(Loc_UI_LEVEL_TOP_LEVEL__CAPS);
        BF_CLAY_TEXT(TextFormat(" %d", g.save.level + 1));
      }
    }
  }

  // Level won screen.
  if (g.run.won.IsSet()) {
    CLAY(  ///
      {
        .layout{
          BF_CLAY_SIZING_GROW_XY,
          BF_CLAY_PADDING_HORIZONTAL_VERTICAL(
            UI_PADDING_OUTER_HORIZONTAL, UI_PADDING_OUTER_VERTICAL
          ),
        },
        .floating{
          .zIndex             = zIndex,
          .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
          .attachTo           = CLAY_ATTACH_TO_PARENT,
        },
      }
    ) {
      // Next level button.
      CLAY(  ///
        {
          .layout{
            BF_CLAY_PADDING_HORIZONTAL_VERTICAL(GAP_BIG, GAP_SMALL),
          },
          .floating{
            .zIndex = zIndex,
            .attachPoints{
              .element = CLAY_ATTACH_POINT_CENTER_CENTER,
              .parent  = CLAY_ATTACH_POINT_CENTER_CENTER,
            },
            .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
            .attachTo           = CLAY_ATTACH_TO_PARENT,
          },
        }
      ) {
        BF_CLAY_TEXT_LOCALIZED(Loc_UI_NEXT_LEVEL__CAPS);

        if (!g.run.advanceScheduled.IsSet() && clickedOrTouchedThisComponent())
          g.run.advanceScheduled.SetNow();
      }
    }
  }

#undef GAP_SMALL
#undef GAP_BIG

#define BF_UI_POST
#include "engine/bf_clay_ui.cpp"
}

void UpdateCamera() {  ///
  g.run.camera.pos = g.run.worldSizef / 2.0f;

  const auto v               = LOGICAL_RESOLUTIONf / g.run.worldSizef;
  g.run.camera.zoom          = MIN(v.x, v.y);
  g.run.camera.texturesScale = 4 * ASSETS_TO_LOGICAL_RATIO / 45.0f;
}

Vector2 GetPassengerBottomPos(int zone, int passengerIndex) {  ///
  f32 off = glib->zone_size()->x() / 2 - glib->passenger_margin()->x()
            - glib->passenger_size()->x() / 2;
  if (passengerIndex == 0)
    off *= -1;
  else if (passengerIndex == 1)
    off = 0;
  return g.run.zones[zone].pos()
         + Vector2(off, glib->passenger_margin()->y() - glib->zone_size()->y() / 2);
}

Rect GetPassengerRect(int zone, int passengerIndex) {  ///
  return {
    .pos = GetPassengerBottomPos(zone, passengerIndex)
           - Vector2(glib->passenger_size()->x() / 2, 0),
    .size = ToVector2(glib->passenger_size()),
  };
}

void GameFixedUpdate() {
  ZoneScoped;

  g.run.worldSize  = ToVector2Int(glib->world_size());
  g.run.worldSizef = (Vector2)g.run.worldSize;

  // Setup. {  ///
  auto& pl = g.run.player;
  ReloadFontsIfNeeded();
  // }

  if (g.run.randomSeedForLevelHotReload != GetFBLevel(g.save.level)->random_seed())
    g.meta.reload = true;

  // Reloading.
  if (g.meta.reload) {  ///
    g.meta.reload = false;
    RunReset();
    RunInit();
  }

  if (!ShouldGameplayStop()) {
    MarkGameplay();

    if (!pl.action && IsTouchPressed(ge.meta._latestActiveTouchID)) {
      const auto td = GetTouchData(ge.meta._latestActiveTouchID);
      const auto wp = LogicalPosToWorld(ScreenPosToLogical(td.screenPos), &g.run.camera);

      int zone = -1;
      for (auto& z : g.run.zones) {
        zone++;

        FOR_RANGE (int, passengerIndex, 3) {
          auto& p = z.rows[0][passengerIndex];

          auto r = GetPassengerRect(zone, passengerIndex);
          if (r.ContainsInside(wp)) {
            if (p && pl.passenger)
              pl.action = PlayerAction_EXCHANGE;
            else if (p)
              pl.action = PlayerAction_PICKUP;
            else if (pl.passenger)
              pl.action = PlayerAction_PUT;

            if (pl.action) {
              pl.actionStartedAt.SetNow();
              pl.actionPassengerIndex = passengerIndex;
              pl.zone                 = zone;
              goto actionWasSet;
            }
          }
        }
      }
    }

  actionWasSet:

    const auto actionDur = lframe::FromSeconds(glib->player_action_duration_seconds());

    if (pl.action && (pl.actionStartedAt.Elapsed() >= actionDur)) {
      auto& zonePassenger = g.run.zones[pl.zone].rows[0][pl.actionPassengerIndex];

      switch (pl.action) {
      case PlayerAction_PICKUP: {
        pl.passenger  = zonePassenger;
        zonePassenger = {};
      } break;

      case PlayerAction_EXCHANGE: {
        std::swap(pl.passenger, zonePassenger);
      } break;

      case PlayerAction_PUT: {
        zonePassenger = pl.passenger;
        pl.passenger  = {};
      } break;
      }

      pl.action          = {};
      pl.actionStartedAt = {};
    }

    // Checking if player's won.
    if (!g.run.remainingRows && !g.run.won.IsSet()) {  ///
      g.run.won.SetNow();
      Save();
    }

    ge.meta.frameGame++;
  }

  // Advancing level.
  if (g.run.advanceScheduled.IsSet()) {  ///
    if ((g.run.advanceScheduled.Elapsed()
         >= lframe::FromSeconds(glib->level_advance_transition_seconds()))
        && !g.meta.reload)
    {
      g.save.level++;
      g.meta.reload = true;
      ShowInterAd();
    }
  }

  UpdateCamera();

  DoUI();

  ge.meta.frameVisual++;
}

void GameDraw() {
  ZoneScoped;

  const auto& pl = g.run.player;

  BeginMode2D(&g.run.camera);

  // Drawing tiled background.
  if (gdebug.drawTiledBackground) {  ///
    FOR_RANGE (int, y, g.run.worldSize.y) {
      FOR_RANGE (int, x, g.run.worldSize.x) {
        if ((x + y) % 2)
          continue;
        DrawGroup_OneShotRect(
          {
            .pos{(f32)x, (f32)y},
            .size{1, 1},
            .anchor{},
            .color = Fade(WHITE, 0.1f),
          },
          DrawZ_DEBUG_TILED_BACKGROUND
        );
      }
    }
  }

  LAMBDA (
    void, drawPassenger, (Vector2 pos, const Passenger& p, f32 rotation, bool hoverable)
  )
  {  ///
    DrawGroup_CommandRect({
      .pos  = pos + p.offVisual,
      .size = ToVector2(glib->passenger_size()),
      .anchor{0.5f, 0},
      .rotation = rotation,
      .color    = Fade(ZONE_COLORS[p.color - 1], 0.5f),
    });
  };

  int        actualLevelIndex = -1;
  const auto fb_level         = GetFBLevel(g.save.level, &actualLevelIndex);

  // Drawing zones.
  if (gdebug.drawZones) {  ///
    DrawGroup_Begin(DrawZ_DEBUG_TILED_BACKGROUND);
    DrawGroup_SetSortY(0);

    int zone = -1;
    for (const auto& z : g.run.zones) {
      zone++;

      const auto r = z.Rect();
      // const bool zonePassengersAreHoverable = ((zone == pl.zone) && pl.parked);
      const bool zonePassengersAreHoverable = true;

      DrawGroup_CommandRect({
        .pos   = r.pos,
        .size  = r.size,
        .color = Fade(CYAN, 0.5f),
      });

      FOR_RANGE (int, passengerIndex, 3) {
        auto& p   = z.rows[0][passengerIndex];
        auto  pos = GetPassengerBottomPos(zone, passengerIndex);
        if (p)
          drawPassenger(pos, p, 0, zonePassengersAreHoverable);

        if (zonePassengersAreHoverable) {
          const auto r = GetPassengerRect(zone, passengerIndex);
          DrawGroup_CommandRectLines({
            .pos    = r.pos,
            .size   = r.size,
            .anchor = {},
            .color  = YELLOW,
          });
        }
      }
    }

    DrawGroup_End();
  }

  // Drawing player.
  {  ///
    auto color = WHITE;

    DrawGroup_Begin(DrawZ_DEFAULT);
    DrawGroup_SetSortY(0);

    f32 rotation = 0;

    auto pos = ToVector2(fb_level->player()) + Vector2(-0.5f, 0.5f);
    if (pl.zone) {
    }

    DrawGroup_CommandRect({
      .pos      = pos,
      .size     = PLAYER_SIZE,
      .rotation = rotation,
      .color    = color,
    });
    if (pl.passenger) {
      drawPassenger(
        pos - Vector2Rotate(Vector2(0, PLAYER_SIZE.y / 2.0f), rotation),
        pl.passenger,
        rotation,
        true
      );
    }

    DrawGroup_End();
  }

  // Gizmos. Colliders.
  if (gdebug.gizmos) {  ///
    DrawGroup_Begin(DrawZ_GIZMOS);
    DrawGroup_SetSortY(0);

    for (const auto& shape : g.run.bodyShapes) {
      if (!shape.active)
        continue;

      auto pos = ToVector2(b2Body_GetPosition(shape.body.id));

      auto color = shape.color;
      if (!b2Body_IsEnabled(shape.body.id))
        color = GRAY;

      switch (shape.type) {
      case BodyShapeType_CIRCLE: {
        DrawGroup_CommandCircleLines({
          .pos    = pos,
          .radius = shape.DataCircle().radius,
          .color  = color,
        });
      } break;

      case BodyShapeType_RECT: {
        const auto rot = b2Body_GetRotation(shape.body.id);
        DrawGroup_CommandRectLines({
          .pos      = pos,
          .size     = shape.DataRect().size,
          .rotation = atan2f(rot.s, rot.c),
          .color    = color,
        });
      } break;

      default:
        INVALID_PATH;
      }
    }

    DrawGroup_End();
  }

  EndMode2D();

  // // Drawing touch controls.
  // if (g.meta.stickControl.controlling  //
  //     && !gdebug.hideUIForVideo)
  // {  ///
  //   DrawGroup_Begin(DrawZ_TOUCH_CONTROLS);
  //   DrawGroup_SetSortY(0);
  //
  //   const auto& c = g.meta.stickControl;
  //
  //   const auto startPos  = c.startPos;
  //   auto       targetPos = c.targetPos;
  //   if (startPos != targetPos)
  //     targetPos = startPos + c.calculatedDir * g.ui.touchControlMaxLogicalOffset;
  //
  //   const struct {
  //     Vector2 pos   = {};
  //     int     texID = {};
  //   } data[]{
  //     {.pos = startPos, .texID = glib->ui_controls_touch_base_texture_id()},
  //     {.pos = targetPos, .texID = glib->ui_controls_touch_handle_texture_id()},
  //   };
  //   for (auto& d : data) {
  //     DrawGroup_CommandTexture({.texID = d.texID, .pos = d.pos});
  //   }
  //
  //   DrawGroup_End();
  // }

  DoUI();

  // Level advancing transition.
  {  ///
    const auto transitionDur
      = lframe::FromSeconds(glib->level_advance_transition_seconds());

    // f32 transitionP = 0;
    if (g.run.advancedAt.IsSet()) {
      ge.settings.screenFade
        = MAX(0, 1 - g.run.advancedAt.Elapsed().Progress(transitionDur));
      // transitionP = ge.settings.screenFade;
    }
    if (g.run.advanceScheduled.IsSet()) {
      ge.settings.screenFade
        = MIN(1, g.run.advanceScheduled.Elapsed().Progress(transitionDur));
      // transitionP = ge.settings.screenFade;
    }
  }

  FlushDrawCommands();

  // Debug info.
  if (ge.meta.debugEnabled) {  ///
    if (0)
      IM::ShowDemoWindow();

    auto localizationEn = glib->localizations()->Get(1)->strings();

    if (IM::Begin("Debug") && IM::BeginTabBar("tabs")) {
      if (IM::BeginTabItem("info")) {
        IM::Text("Close debug menu: hold F1 -> press F2");

        IM::Text("");

        if (IM::Button("Reset Debug"))
          gdebug = {};

        IM::Checkbox("Gizmos", &gdebug.gizmos);
        IM::Checkbox("Emulating Mobile", &gdebug.emulatingMobile);
        IM::Checkbox("Hide UI For Video", &gdebug.hideUIForVideo);
        ge.meta.device
          = (gdebug.emulatingMobile ? DeviceType_MOBILE : DeviceType_DESKTOP);

        IM::Text("MarkGameplay: %d", ge.meta.markGameplay);

        IM::Text("F3 change localization");
        IM::Text("F4 change device");

        LAMBDA (void, debugTextArena, (const char* name, const Arena& arena)) {
          IM::Text(
            "%s: %d %d (%.1f%%) (max: %d, %.1f%%)",
            name,
            (int)arena.used,
            (int)arena.size,
            100.0f * (f32)arena.used / (f32)arena.size,
            (int)arena.maxUsed,
            100.0f * (f32)arena.maxUsed / (f32)arena.size
          );
        };

        debugTextArena("ge.meta._arena", ge.meta._arena);
        debugTextArena("ge.meta.trashArena", ge.meta.trashArena);
        debugTextArena("ge.meta._transientDataArena", ge.meta._transientDataArena);

        IM::Checkbox("Draw Tiled Background", &gdebug.drawTiledBackground);
        IM::Checkbox("Draw Zones", &gdebug.drawZones);

        if (IM::Button("Win") && !g.run.won.IsSet())
          g.run.won.SetNow();

        if (IM::Button("Reset Level"))
          g.meta.reload = true;
        if (IM::Button("Set Level To 0")) {
          g.save.level  = 0;
          g.meta.reload = true;
        }
        if (IM::Button("Level++")) {
          g.save.level++;
          g.meta.reload = true;
        }
        if (IM::Button("Level--")) {
          g.save.level--;
          if (g.save.level < 0)
            g.save.level = 0;
          g.meta.reload = true;
        }

        IM::Text("Actual level index %d", actualLevelIndex);

        // if (IM::Button(
        //       TextFormat("Level Seed (%d) ++", (int)g.run.randomSeedForLevelHotReload)
        //     ))
        //   g.run.randomSeedForLevelHotReload++;

        IM::EndTabItem();
      }

      IM::EndTabBar();
    }

    IM::End();
  }
}

///
