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
  int needsZoneIndex = -1;
};

struct ZoneCommonData {  ///
  Vector2Int pos             = {};
  int        width           = {};
  bool       passengersRight = {};
};

struct Zone {  ///
  ZoneCommonData    c          = {};
  Vector<Passenger> passengers = {};

  Rect Rect() const {
    return {.pos = Vector2(c.pos), .size = Vector2(c.width, 1)};
  }
};

const Color ZONE_COLORS_[]{RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA};
VIEW_FROM_ARRAY_DANGER(ZONE_COLORS);

enum PlayerState {  ///
  PlayerState_DEFAULT,
  PlayerState_PICKING_UP,
  PlayerState_PUTTING_DOWN,
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
  } meta;

  struct Run {
    struct State {
      int level = 0;
    } state;

    Vector2Int worldSize  = {20, 16};
    Vector2    worldSizef = {20, 16};
    b2WorldId  world      = {};

    Camera camera{
      .zoom          = METER_LOGICAL_SIZE,
      .texturesScale = 1.0f / METER_LOGICAL_SIZE,
    };

    struct Player {
      Vector2   pos       = {};
      f32       rotation  = {};
      Body      body      = {};
      Passenger passenger = {};

      Vector2 movement         = {};
      int     groundContacts   = 0;
      bool    isGrounded       = false;
      bool    isReallyGrounded = false;

      struct {
        PlayerState v         = {};
        FrameGame   startedAt = {};
        int         zoneIndex = -1;
      } state;

      bool CanMove() {  ///
        return !state.v;
      }

      bool CanMoveHorizontally() {  ///
        return CanMove() && !isGrounded;
      }

    } player;

    // Using "X-macros". ref: https://www.geeksforgeeks.org/c/x-macros-in-c/
    // These containers preserve allocated memory upon resetting state of the run.
#define VECTORS_TABLE      \
  X(BodyShape, bodyShapes) \
  X(Zone, zones)           \
  X(Passenger, passengers)

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
  bodyDef.position       = ToB2Vec2(pos);
  bodyDef.linearDamping  = glib->nohotreload_player_linear_damping();
  bodyDef.angularDamping = glib->nohotreload_player_angular_damping();

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
  MakeBodyData bodyData = {};
};

Body MakeRectBody(MakeRectBodyData data) {  ///
  ASSERT(data.size.x > 0);
  ASSERT(data.size.y > 0);

  auto makeBodyResult
    = MakeBody(data.pos - (data.anchor - Vector2Half()) * data.size, data.bodyData);

  auto box = b2MakeBox(data.size.x / 2, data.size.y / 2);
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

struct MakePlatformData {  ///
  Vector2Int pos  = {};
  Vector2Int size = {};
};

void MakePlatform(MakePlatformData data) {               ///
  if ((data.pos.x < 0) || (data.pos.y < 0)               //
      || (data.size.x <= 0) || (data.size.y <= 0)        //
      || (data.pos.x + data.size.x > g.run.worldSize.x)  //
      || (data.pos.y + data.size.y > g.run.worldSize.y))
  {
    INVALID_PATH;
    return;
  }

  MakeRectBody({
    .pos  = Vector2(data.pos),
    .size = Vector2(data.size),
    .anchor{0, 0},
    .bodyData{
      .type     = BodyType_STATIC,
      .userData = ShapeUserData::Static(),
    },
  });
}

void GameLoad(const BFSave::Save* save) {  ///
  auto& s = g.run.state;
  s.level = save->level();
}

void GameDumpStateForSaving(BFSave::SaveT& save) {  ///
  save.level = g.run.state.level;
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

void RunInit() {
  // Creating box2d world.
  {  ///
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity    = {0, 0};
    g.run.world         = b2CreateWorld(&worldDef);
  }

  // Creating player.
  {  ///
    auto pos     = g.run.worldSizef / 2.0f;
    g.run.player = {
      .pos  = pos,
      .body = MakeRectBody({
        .pos = pos,
        .size{1.5f, 1.5f},
        .bodyData{
          .type     = BodyType_CREATURE,
          .userData = ShapeUserData::Creature(0),
          .isPlayer = true,
        },
      }),
    };
  }

  // Placing walls.
  {  ///
    Vector2Int p00{-1, -1};
    auto       p11 = g.run.worldSize;

    Line lines_[]{
      // Walls around.
      {{p11.x, p00.y}, {p11.x, p11.y}},  // right
      {{p00.x, p11.y}, {p11.x, p11.y}},  // up
      {{p00.x, p00.y}, {p00.x, p11.y}},  // left
      {{p00.x, p00.y}, {p11.x, p00.y}},  // down
    };
    VIEW_FROM_ARRAY_DANGER(lines);

    MakeWalls({.lines = lines});
  }

  // ZoneCommonData zones[]{
  //   {.pos{1, 9}, .width = 5},
  //   {.pos{14, 13}, .width = 5, .passengersRight = true},
  //   {.pos{5, 3}, .width = 10},
  // };
  // int zoneIndex = -1;
  // for (const auto& x : zones) {
  //   ASSERT(x.width > 0);
  //   zoneIndex++;
  //
  // }

  const auto fb_level      = glib->levels()->Get(g.run.state.level);
  const auto fb_tiles      = glib->tiles();
  const auto fb_levelTiles = fb_level->tiles();
  const int  sy            = fb_level->sy();
  const int  sx            = fb_level->sx();

  g.run.worldSize  = {sx, sy};
  g.run.worldSizef = (Vector2)g.run.worldSize;

  LAMBDA (void, platformify, (auto checkLambda, auto implLambda)) {  ///
    FOR_RANGE (int, y, sy) {
      int platformX = -1;
      int platformW = 0;

      FOR_RANGE (int, x, sx + 1) {
        if (x == sx) {
          if (platformW)
            implLambda({platformX, y}, {platformW, 1});
          break;
        }

        const int  t    = y * sx + x;
        const auto type = (TileType)fb_levelTiles->Get(t);

        if (checkLambda(type, fb_tiles->Get(type))) {
          if (platformX == -1)
            platformX = x;
          platformW++;
        }
        else if (platformW) {
          implLambda({platformX, y}, {platformW, 1});
          platformW = 0;
          platformX = -1;
        }
      }
    }
  };

  // Placing solid blocks.
  platformify(  ///
    [&](TileType _type, auto fb_tile) BF_FORCE_INLINE_LAMBDA { return fb_tile->solid(); },
    [&](Vector2Int pos, Vector2Int size)
      BF_FORCE_INLINE_LAMBDA { MakePlatform({.pos = pos, .size = size}); }
  );

  // Placing zones.
  if (fb_level->zones()) {  ///
    int zoneIndex = -1;
    for (auto fb_zone : *fb_level->zones()) {
      zoneIndex++;
      auto& z = *g.run.zones.Add();
      z       = {.c{.pos{fb_zone->px(), fb_zone->py()}, .width = fb_zone->w()}};
      if (fb_level->zones()->size() > 1) {
        FOR_RANGE (int, i, 3) {
          int needsZoneIndex = zoneIndex;
          while (needsZoneIndex == zoneIndex)
            needsZoneIndex = GRAND.Rand() % fb_level->zones()->size();
          *z.passengers.Add() = {.needsZoneIndex = needsZoneIndex};
        };
      }
    }
  }
}

void RunReset() {  ///
  ZoneScoped;

  b2DestroyWorld(g.run.world);

  for (auto& x : g.run.zones)
    x.passengers.Deinit();

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
#define X(type_, name_) .name_ = temp.name_,
    VECTORS_TABLE
#undef X
  };
}

void GamePreInit(GamePreInitOpts opts) {  ///
  ZoneScoped;

  ge.meta.logicRand = Random(SDL_GetPerformanceCounter());
  *opts.baseFont    = &g.meta.fontUI;
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

void GameInit() {  ///
  ZoneScoped;

  ReloadFontsIfNeeded();

  RunInit();
}

void GameInitAfterLoadingSavedata() {  ///
  ZoneScoped;

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

#define BF_UI_POST
#include "engine/bf_clay_ui.cpp"
}

void UpdateCamera() {  ///
  g.run.camera.pos = g.run.worldSizef / 2.0f;

  const auto v      = LOGICAL_RESOLUTIONf / g.run.worldSizef;
  g.run.camera.zoom = MIN(v.x, v.y);
}

f32 GetPassengerPickupProgress();

f32 GetPassengerPosX(int zoneIndex, int i, bool drawing) {  ///
  const auto& z = g.run.zones[zoneIndex];

  const auto& p       = z.passengers[i];
  auto        offsetX = glib->passenger_origin_offset_x() + glib->passenger_gap() * i;
  auto        origin  = z.c.pos.x;
  if (z.c.passengersRight) {
    origin += z.c.width;
    offsetX *= -1;
  }
  auto result = origin + offsetX;

  const auto& pl = g.run.player;
  if (drawing                                    //
      && (pl.state.v == PlayerState_PICKING_UP)  //
      && (zoneIndex == pl.state.zoneIndex)       //
      && (i == z.passengers.count - 1))
  {
    const auto p = GetPassengerPickupProgress();
    result       = Lerp(result, pl.pos.x, EaseInOutQuad(p));
  }
  return result;
}

f32 GetPassengerPickupProgress() {  ///
  const auto& pl = g.run.player;
  ASSERT(pl.state.v == PlayerState_PICKING_UP);

  const auto& z = g.run.zones[pl.state.zoneIndex];
  ASSERT(z.passengers.count >= 0);

  auto dist
    = abs(GetPassengerPosX(pl.state.zoneIndex, z.passengers.count - 1, false) - pl.pos.x);
  auto e      = pl.state.startedAt.Elapsed();
  auto result = e.Progress(lframe::FromSeconds(dist / glib->passenger_speed()));
  return result;
}

f32 PlayerGroundedRaycastCallback(
  b2ShapeId shapeId,
  b2Vec2    _point,
  b2Vec2    _normal,
  f32       fraction,
  void*     _context
) {  ///
  const auto shape = ShapeUserData::FromPointer(b2Shape_GetUserData(shapeId)).type;
  if (shape == ShapeUserDataType_STATIC)
    g.run.player.groundContacts += 1;
  return fraction;
}

void GameFixedUpdate() {
  ZoneScoped;

  // Setup. {  ///
  auto& pl = g.run.player;
  ReloadFontsIfNeeded();
  // }

  // Player movement input.
  {  ///
    Vector2 move{};

    if (g.meta.stickControl.controlling)
      move = g.meta.stickControl.calculatedDir;
    else {
      if (IsKeyDown(SDL_SCANCODE_D) || IsKeyDown(SDL_SCANCODE_RIGHT))
        move.x += 1;
      if (IsKeyDown(SDL_SCANCODE_A) || IsKeyDown(SDL_SCANCODE_LEFT))
        move.x -= 1;
      if (IsKeyDown(SDL_SCANCODE_W) || IsKeyDown(SDL_SCANCODE_UP))
        move.y += 1;
    }

    g.run.player.movement = move;
  }

  // TODO: DON'T FORGET THAT IT CAN'T BE DOWN!!!!
  // // Stick controls.
  // {  ///
  //   auto& c = g.meta.stickControl;
  //
  //   if (IsMousePressed(L)) {
  //     c.controlling = true;
  //     c.startPos    = ScreenPosToLogical(GetMouseScreenPos());
  //     c.targetPos   = c.startPos;
  //   }
  //   else if (ge.meta._latestActiveTouchID != InvalidTouchID) {
  //     const auto td = GetTouchData(ge.meta._latestActiveTouchID);
  //
  //     if (IsTouchPressed(ge.meta._latestActiveTouchID)) {
  //       c.controlling = true;
  //       c.startPos    = ScreenPosToLogical(td.screenPos);
  //       c.targetPos   = c.startPos;
  //     }
  //
  //     c.targetPos = ScreenPosToLogical(td.screenPos);
  //   }
  //   else if (IsMouseDown(L))
  //     c.targetPos = ScreenPosToLogical(GetMouseScreenPos());
  //   else
  //     c.controlling = false;
  //
  //   if (c.controlling && (c.startPos != c.targetPos)) {
  //     c.calculatedDir
  //             = Vector2Normalize(c.targetPos - c.startPos)
  //               * (MIN(
  //                 g.ui.touchControlMaxLogicalOffset, Vector2Distance(c.startPos,
  //                 c.targetPos)
  //               )
  //               / g.ui.touchControlMaxLogicalOffset);
  //   }
  //   else
  //     c.calculatedDir = {};
  //
  //   if (c.controlling)
  //     g.run.playerMovement = c.calculatedDir;
  // }

  // Player moving.
  {  ///
    ZoneScopedN("Player moving.");

    b2Body_ApplyForceToCenter(pl.body.id, {0, glib->gravity()}, true);

    auto mov = g.run.player.movement;
    if (!pl.CanMoveHorizontally())
      mov.x = 0;
    if (!pl.CanMove())
      mov = {};
    b2Body_ApplyForceToCenter(
      pl.body.id,
      ToB2Vec2({
        mov.x * glib->player_speed_x(),
        mov.y * (glib->player_speed_y() - glib->gravity()),
      }),
      true
    );
  }

  // Turning helicopter vertical.
  {  ///
    ZoneScopedN("Turning helicopter vertical.");

    const auto rot = b2Body_GetRotation(pl.body.id);

    b2Body_ApplyTorque(
      pl.body.id,
      -glib->player_vertical_restoration_stiffness() * pl.rotation
        - glib->player_vertical_restoration_damping()
            * b2Body_GetAngularVelocity(pl.body.id),
      true
    );
  }

  // Updating box2d world.
  {  ///
    ZoneScopedN("Updating box2d world.");
    b2World_Step(g.run.world, FIXED_DT, 4);
  }

  // Retrieving body positions and rotations.
  {  ///
    ZoneScopedN("Retrieving body positions and rotations.");

    pl.pos         = ToVector2(b2Body_GetPosition(pl.body.id));
    const auto rot = b2Body_GetRotation(pl.body.id);
    pl.rotation    = atan2f(rot.s, rot.c);
  }

  // Checking if player is grounded.
  {  ///
    pl.isGrounded       = false;
    pl.isReallyGrounded = false;
    pl.groundContacts   = 0;

    if (abs(pl.rotation) < 0.001f) {
      for (int i = -1; i <= 1; i += 1) {
        b2World_CastRay(
          g.run.world,
          ToB2Vec2({
            pl.pos.x + PLAYER_COLLIDER_SIZE.x * 0.98f * (int)i,
            pl.pos.y,
          }),
          {0, -PLAYER_COLLIDER_SIZE.y / 1.95f},
          {
            .categoryBits = ShapeCategory_PLAYER,
            .maskBits     = ShapeCategory_STATIC,
          },
          PlayerGroundedRaycastCallback,
          nullptr
        );
      }

      if (pl.groundContacts >= 2) {
        pl.isGrounded = true;
        pl.isReallyGrounded
          = abs(Vector2Length(ToVector2(b2Body_GetLinearVelocity(pl.body.id)))) < 0.001f;
      }
    }
  }

  if (pl.isReallyGrounded) {
    // Putting passenger down.
    {  ///
      if ((!pl.state.v || pl.state.v == PlayerState_PUTTING_DOWN)
          && (pl.passenger.needsZoneIndex >= 0))
      {
        const auto& z = g.run.zones[pl.passenger.needsZoneIndex];

        if (z.Rect().ContainsInside(pl.pos))
          pl.passenger = {};
      }
    }

    // Picking up passenger.
    if ((!pl.state.v || (pl.state.v == PlayerState_PICKING_UP))
        && (pl.passenger.needsZoneIndex < 0))
    {  ///
      int zoneIndex = -1;
      for (auto& z : g.run.zones) {
        zoneIndex++;

        if (!z.Rect().ContainsInside(pl.pos))
          continue;

        if (z.passengers.count <= 0) {
          ASSERT_FALSE(z.passengers.count);
          continue;
        }

        if (!pl.state.v) {
          LOGD("Started picking up");
          pl.state.v = PlayerState_PICKING_UP;
          pl.state.startedAt.SetNow();
          pl.state.zoneIndex = zoneIndex;
        }

        if (GetPassengerPickupProgress() >= 1) {
          LOGD("Picked up");
          pl.state     = {};
          pl.passenger = z.passengers.Pop();
        }
      }
    }
  }

  UpdateCamera();

  DoUI();

  ge.meta.frameGame++;
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

  LAMBDA (void, drawPassenger, (Vector2 pos, const Passenger& p, f32 rotation)) {  ///
    DrawGroup_CommandRect({
      .pos  = pos,
      .size = Vector2One() * glib->passenger_width(),
      .anchor{0.5f, 0},
      .rotation = rotation,
      .color    = Fade(ZONE_COLORS[p.needsZoneIndex], 0.5f),
    });
  };

  const auto fb_level = glib->levels()->Get(g.run.state.level);

  // Drawing tiles.
  {  ///
    DrawGroup_Begin(DrawZ_DEBUG_TILED_BACKGROUND);
    DrawGroup_SetSortY(0);

    const auto sx            = fb_level->sx();
    const auto fb_levelTiles = fb_level->tiles();
    const auto fb_tiles      = glib->tiles();

    FOR_RANGE (int, y, fb_level->sy()) {
      FOR_RANGE (int, x, sx) {
        const auto fb_tile = fb_tiles->Get(fb_levelTiles->Get(y * sx + x));
        if (!fb_tile->solid())
          continue;
        DrawGroup_CommandRect({
          .pos{(f32)x, (f32)y},
          .size{1, 1},
          .anchor{},
          .color = Fade(WHITE, 0.5f),
        });
      }
    }

    DrawGroup_End();
  }

  // Drawing zones.
  if (gdebug.drawZones) {  ///
    DrawGroup_Begin(DrawZ_DEBUG_TILED_BACKGROUND);
    DrawGroup_SetSortY(0);

    int zoneIndex = -1;
    for (const auto& z : g.run.zones) {
      zoneIndex++;

      DrawGroup_CommandRect({
        .pos = z.c.pos,
        .size{(f32)z.c.width, 1},
        .anchor{},
        .color = Fade(ZONE_COLORS[zoneIndex], 0.5f),
      });

      const int pcount = MIN(glib->passenger_max_shown(), z.passengers.count);
      FOR_RANGE (int, i_, pcount) {
        const int i = z.passengers.count - i_ - 1;
        drawPassenger(
          {GetPassengerPosX(zoneIndex, i, true), z.c.pos.y}, z.passengers[i], 0
        );
      }
    }

    DrawGroup_End();
  }

  // Drawing player.
  {  ///
    auto color = WHITE;
    if (pl.isGrounded)
      color = YELLOW;
    if (pl.isReallyGrounded)
      color = RED;

    DrawGroup_Begin(DrawZ_DEFAULT);
    DrawGroup_SetSortY(0);

    DrawGroup_CommandRect({
      .pos      = pl.pos,
      .size     = PLAYER_COLLIDER_SIZE,
      .rotation = pl.rotation,
      .color    = color,
    });
    if (pl.passenger.needsZoneIndex >= 0) {
      drawPassenger(
        pl.pos - Vector2Rotate(Vector2(0, PLAYER_COLLIDER_SIZE.y / 2.0f), pl.rotation),
        pl.passenger,
        pl.rotation
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

        IM::EndTabItem();
      }

      IM::EndTabBar();
    }

    IM::End();
  }
}

///
