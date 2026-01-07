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
  int color      = 0;
  f32 posX       = {};
  f32 posXVisual = f32_inf;

  operator bool() const {
    return color > 0;
  }
};

struct Zone {  ///
  Vector2Int                  pos        = {};
  int                         width      = {};
  PushableArray<Passenger, 8> passengers = {};

  Rect Rect() const {
    return {.pos = Vector2(pos), .size = Vector2(width, 2)};
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

    bool reload = {};
  } meta;

  struct Save {
    int level = 0;
  } save;

  struct Run {
    Vector2Int worldSize  = {};
    Vector2    worldSizef = {};
    b2WorldId  world      = {};

    Camera camera{};

    uint        passengersTotal = 0;
    FrameVisual won             = {};

    FrameVisual advanceScheduled = {};  // TODO: Transition.
    FrameVisual advancedAt       = {};

    struct Player {
      Vector2   pos       = {};
      f32       rotation  = {};
      Body      body      = {};
      Passenger passenger = {};
      int       zone      = -1;

      Vector2 movement         = {};
      int     groundContacts   = 0;
      bool    isGrounded       = false;
      bool    isReallyGrounded = false;

      FrameVisual reallyGroundedAt = {};
      FrameVisual diedAt           = {};

      struct {
        PlayerState v         = {};
        FrameGame   startedAt = {};
        int         zoneIndex = -1;
      } state;

      bool CanMove() const {  ///
        return !state.v && !g.run.won.IsSet();
      }

      bool CanMoveHorizontally() const {  ///
        return CanMove() && !isGrounded;
      }
    } player;

    // Using "X-macros". ref: https://www.geeksforgeeks.org/c/x-macros-in-c/
    // These containers preserve allocated memory upon resetting state of the run.

    PushableArray<Zone, 10> zones  = {};
    int                     colors = {};

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
    .radius = glib->nohotreload_platform_rect_radius(),
    .bodyData{
      .type     = BodyType_STATIC,
      .userData = ShapeUserData::Static(),
    },
  });
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
  // Creating box2d world.
  {  ///
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity    = {0, 0};
    g.run.world         = b2CreateWorld(&worldDef);
  }

  const auto fb_level      = GetFBLevel(g.save.level);
  const auto fb_tiles      = glib->tiles();
  const auto fb_levelTiles = fb_level->tiles();
  const int  sx            = fb_level->sx();
  const int  sy            = fb_level->sy();

  g.run.worldSize  = {sx, sy};
  g.run.worldSizef = (Vector2)g.run.worldSize;

  // Placing walls.
  {  ///
    Vector2Int p00{-1 + glib->extend_cells_horizontal(), -1};
    auto       p11 = g.run.worldSize - Vector2Int(glib->extend_cells_horizontal(), 0);

    Line lines_[]{
      // Walls around.
      {{p11.x, p00.y}, {p11.x, p11.y}},  // right
      {{p00.x, p11.y}, {p11.x, p11.y}},  // up
      {{p00.x, p00.y}, {p00.x, p11.y}},  // left
      // {{p00.x, p00.y}, {p11.x, p00.y}},  // down
    };
    VIEW_FROM_ARRAY_DANGER(lines);

    MakeWalls({.lines = lines});
  }

  // Placing solid blocks.
  FOR_RANGE (int, y, sy) {  ///
    int platformX = -1;
    int platformW = 0;

    FOR_RANGE (int, x, sx + 1) {
      if (x == sx) {
        if (platformW)
          MakePlatform({.pos{platformX, y}, .size{platformW, 1}});
        break;
      }

      const int  t       = y * sx + x;
      const auto fb_tile = fb_tiles->Get(fb_levelTiles->Get(t));

      if (fb_tile->solid()) {
        if (platformX == -1)
          platformX = x;
        platformW++;
      }
      else if (platformW) {
        MakePlatform({.pos{platformX, y}, .size{platformW, 1}});
        platformW = 0;
        platformX = -1;
      }
    }
  }

  // Placing zones.
  if (fb_level->zones()) {  ///
    const int PASSENGERS_PER_ZONE = 3;

    ASSERT_FALSE(g.run.zones.count);
    int zoneIndex = -1;
    for (auto fb_zone : *fb_level->zones()) {
      zoneIndex++;
      auto& z = *g.run.zones.Add();
      z       = {
              .pos{fb_zone->px(), fb_zone->py()},
              .width = fb_zone->w(),
      };
    }
    g.run.colors = g.run.zones.count + 0;

    // Filling zones with passengers.
    {
    zonesContinue:
      g.run.passengersTotal = 0;
      for (auto& z : g.run.zones)
        z.passengers.Reset();

      FOR_RANGE (int, color, g.run.colors) {
        FOR_RANGE (int, _passengerIndex, 3) {
          auto& z = g.run.zones[GRAND.Rand() % g.run.zones.count];
          if (z.passengers.count >= z.passengers.maxCount)
            goto zonesContinue;

          *z.passengers.Add() = {.color = color + 1};
          g.run.passengersTotal++;
        }
      }

      int minPassengersOnPlatform = int_max;
      int maxPassengersOnPlatform = 0;

      for (auto& z : g.run.zones) {
        minPassengersOnPlatform = MIN(minPassengersOnPlatform, z.passengers.count);
        maxPassengersOnPlatform = MAX(maxPassengersOnPlatform, z.passengers.count);

        FOR_RANGE (int, color, g.run.colors) {
          int passengersOfThisColor = 0;
          for (auto& x : z.passengers) {
            if (x.color == color)
              passengersOfThisColor++;
          }
          if (passengersOfThisColor >= 3)
            goto zonesContinue;
        }
      }

      if (maxPassengersOnPlatform - minPassengersOnPlatform > 3)
        goto zonesContinue;
    }
  }

  // Creating player.
  {  ///
    const auto pos
      = ToVector2(fb_level->player()) - Vector2(0, 2 - PLAYER_COLLIDER_SIZE.y) / 2.0f;
    g.run.player = {
      .pos  = pos,
      .body = MakeRectBody({
        .pos = pos,
        .size{1.5f, 1.5f},
        .radius = glib->nohotreload_player_rect_radius(),
        .bodyData{
          .type     = BodyType_CREATURE,
          .userData = ShapeUserData::Creature(0),
          .isPlayer = true,
        },
      }),
    };
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

void CheckGamelib() {
#if !BF_ENABLE_ASSERTS
  return;
#endif

  if (!glib) {
    glibFile = LoadFile(GAMELIB_PATH, nullptr);
    glib     = BFGame::GetGameLibrary(glibFile);
  }

  // Checking levels cycling and mirroring.
  {  ///
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
}

TEST_CASE ("CheckGamelib") {  ///
  CheckGamelib();
}

void GameInit() {  ///
  ZoneScoped;

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

  // Actions.
  if (0 && pl.isReallyGrounded && (pl.zone >= 0)) {
    // Screen.
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
      FLOATING_BEAUTIFY;

      // Bottom row of actions.
      CLAY(  ///
        {
          .layout{.childGap = GAP_SMALL},
          .floating{
            .zIndex = zIndex,
            .attachPoints{
              .element = CLAY_ATTACH_POINT_CENTER_BOTTOM,
              .parent  = CLAY_ATTACH_POINT_CENTER_BOTTOM,
            },
            .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_CAPTURE,
            .attachTo           = CLAY_ATTACH_TO_PARENT,
          },
        }
      ) {
        FLOATING_BEAUTIFY;

        auto& z = g.run.zones[pl.zone];

        struct CardData {  ///
          Loc loc = {};
        };

        LAMBDA (bool, card, (CardData data)) {  ///
          bool result = false;
          CLAY({
            .layout{
              .sizing{CLAY_SIZING_FIXED(100), CLAY_SIZING_FIXED(100)},
              BF_CLAY_PADDING_ALL(GAP_SMALL),
              BF_CLAY_CHILD_ALIGNMENT_CENTER_CENTER,
              .layoutDirection = CLAY_TOP_TO_BOTTOM,
            },
            BF_CLAY_CUSTOM_BEGIN{
              BF_CLAY_CUSTOM_SHADOW(glib->ui_frame_shadow_small_nine_slice(), true),
              BF_CLAY_CUSTOM_NINE_SLICE(
                glib->ui_frame_nine_slice(),
                ColorFromRGBA(glib->ui_frame_color()),
                TRANSPARENT_BLACK,
                true
              ),
            } BF_CLAY_CUSTOM_END,
          }) {
            BF_CLAY_SPACER_VERTICAL;
            BF_CLAY_TEXT_LOCALIZED(data.loc);
            result = clickedOrTouchedThisComponent();
          }
          return result;
        };

        // Putting passenger down to platform.
        if (pl.passenger) {  ///
          if (card({.loc = Loc_UI_PASSENGER_PUT_DOWN})) {
            *z.passengers.Add() = pl.passenger;
            pl.passenger        = {};
          }
        }

        int off   = 0;
        int total = z.passengers.count;
        // Swapping or picking up.
        FOR_RANGE (int, i, total) {  ///
          auto& p = z.passengers[i - off];
          Loc   loc{};
          if (pl.passenger) {
            loc = Loc_UI_PASSENGER_EXCHANGE;
            if (pl.passenger.color == p.color)
              continue;
          }
          else
            loc = Loc_UI_PASSENGER_PICK_UP;
          if (card({.loc = loc})) {
            if (pl.passenger) {
              // Swapping player's passenger with the one on the platform.
              auto t       = pl.passenger;
              pl.passenger = p;
              p            = t;
            }
            else {
              // Picking up passenger.
              pl.passenger = p;
              z.passengers.RemoveAt(i - off);
              off++;
            }
          }
        }
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
  const auto ws = g.run.worldSizef
                  - Vector2(
                    glib->extend_cells_horizontal() * 2,
                    glib->extend_cells_floor() + glib->extend_cells_ceiling()
                  );
  g.run.camera.pos
    = ws / 2.0f + Vector2(glib->extend_cells_horizontal(), glib->extend_cells_floor());

  const auto v               = LOGICAL_RESOLUTIONf / ws;
  g.run.camera.zoom          = MIN(v.x, v.y);
  g.run.camera.texturesScale = 4 * ASSETS_TO_LOGICAL_RATIO / 45.0f;
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

  if (g.meta.reload) {
    g.meta.reload = false;
    RunReset();
    RunInit();
    Save();
  }

  if (!ShouldGameplayStop()) {
    MarkGameplay();

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
        if (IsKeyDown(SDL_SCANCODE_W)      //
            || IsKeyDown(SDL_SCANCODE_UP)  //
            || IsKeyDown(SDL_SCANCODE_SPACE))
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
              pl.pos.x + PLAYER_COLLIDER_SIZE.x * 0.48f * (int)i,
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
            = abs(Vector2Length(ToVector2(b2Body_GetLinearVelocity(pl.body.id))))
              < 0.001f;
        }
      }

      if (pl.isReallyGrounded && !pl.reallyGroundedAt.IsSet())
        pl.reallyGroundedAt.SetNow();
      else if (!pl.isReallyGrounded && pl.reallyGroundedAt.IsSet())
        pl.reallyGroundedAt = {};
    }

    // Updating player's zone.
    {  ///
      pl.zone       = -1;
      int zoneIndex = -1;
      for (auto& z : g.run.zones) {
        zoneIndex++;
        if (z.Rect().ContainsInside(pl.pos)) {
          pl.zone = zoneIndex;
          break;
        }
      }
    }

    // Removing 3-matched passengers.
    for (auto& z : g.run.zones) {  ///
      for (int color = 1; color <= g.run.colors; color++) {
        int passengersOfThisColor = 0;
        for (auto& p : z.passengers) {
          if (p.color == color)
            passengersOfThisColor++;
        }

        if (passengersOfThisColor >= 3) {
          for (int i = z.passengers.count - 1; i >= 0; i--) {
            if (z.passengers[i].color == color) {
              z.passengers.RemoveAt(i);
              passengersOfThisColor--;
              g.run.passengersTotal--;
              if (!passengersOfThisColor)
                break;
            }
          }
        }
      }
    }

    // Checking if player's won.
    if (!g.run.passengersTotal && !g.run.won.IsSet()) {  ///
      g.run.won.SetNow();
      Save();
    }

    // if (!g.run.won.IsSet() && !pl.diedAt.IsSet()) {
    //   // Player dies below the map.
    //   if (pl.pos.y < -PLAYER_COLLIDER_SIZE.y / 2.0f)
    //     pl.diedAt.SetNow();
    // }

    // Balancing passenger positions.
    int zoneIndex = -1;
    for (auto& z : g.run.zones) {  ///
      zoneIndex++;

      const auto r     = z.Rect();
      f32        width = r.size.x;
      f32        gap   = width / (z.passengers.count + 1);

      FOR_RANGE (int, i, z.passengers.count) {
        auto& p = z.passengers[i];
        p.posX  = z.pos.x + gap * (i + 1);
        if (p.posXVisual == f32_inf)
          p.posXVisual = p.posX;
      }

      if (pl.zone == zoneIndex) {
        int lastPassengerToTheLeftOfPlayer   = -1;
        int firstPassengerToTheRightOfPlayer = -1;

        FOR_RANGE (int, i, z.passengers.count) {
          if (z.passengers[i].posX <= pl.pos.x)
            lastPassengerToTheLeftOfPlayer = i;
          else
            break;
        }
        FOR_RANGE (int, i_, z.passengers.count) {
          int i = z.passengers.count - i_ - 1;
          if (z.passengers[i].posX > pl.pos.x)
            firstPassengerToTheRightOfPlayer = i;
          else
            break;
        }

        // kRatio = K / k = Player's spring density is 1.5x bigger.
        const f32 kRatio = 1.8f;

        if (lastPassengerToTheLeftOfPlayer >= 0) {
          f32 width = pl.pos.x - r.pos.x;
          ASSERT(width >= 0);
          ASSERT(width <= r.size.x);

          int n        = lastPassengerToTheLeftOfPlayer + 1;
          f32 x        = width / (n + kRatio);
          f32 lastPosX = z.pos.x;
          FOR_RANGE (int, i, lastPassengerToTheLeftOfPlayer + 1) {
            auto& p = z.passengers[i];
            lastPosX += x;
            p.posX = lastPosX;
          }
        }

        if (firstPassengerToTheRightOfPlayer >= 0) {
          const f32 width = r.pos.x + r.size.x - pl.pos.x;
          ASSERT(width >= 0);
          ASSERT(width <= r.size.x);

          const int n        = z.passengers.count - firstPassengerToTheRightOfPlayer;
          f32       x        = width / (n + kRatio);
          f32       lastPosX = z.pos.x + r.size.x - width + x * kRatio;
          for (int i = firstPassengerToTheRightOfPlayer; i < z.passengers.count; i++) {
            auto& p = z.passengers[i];
            p.posX  = lastPosX;
            lastPosX += x;
          }
        }
      }

      for (auto& p : z.passengers)
        p.posXVisual = Lerp(p.posXVisual, p.posX, glib->passengers_pos_lerp_factor());
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

  const auto fb_tiles = glib->tiles();

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

  LAMBDA (void, drawPassenger, (int zone, Vector2 pos, const Passenger& p, f32 rotation))
  {  ///
    DrawGroup_CommandRect({
      .pos  = pos,
      .size = Vector2One() * glib->passenger_width(),
      .anchor{0.5f, 0},
      .rotation = rotation,
      .color    = Fade(ZONE_COLORS[p.color - 1], 0.5f),
    });

    if ((zone == pl.zone) && pl.isReallyGrounded) {
      DrawGroup_CommandCircleLines({
        .pos{p.posX, pos.y},
        .radius = glib->passenger_click_radius(),
        .color  = YELLOW,
      });
    }
  };

  int        actualLevelIndex = -1;
  const auto fb_level         = GetFBLevel(g.save.level, &actualLevelIndex);

  // Drawing tiles.
  {  ///
    const auto sx            = fb_level->sx();
    const auto sy            = fb_level->sy();
    const auto fb_levelTiles = fb_level->tiles();

    if (gdebug.drawTiles) {
      DrawGroup_Begin(DrawZ_DEBUG_TILED_BACKGROUND);
      DrawGroup_SetSortY(0);

      FOR_RANGE (int, y, sy) {
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

    if (0) {
      DrawGroup_Begin(DrawZ_TILES);
      DrawGroup_SetSortY(0);

      const int checksToTilesetTextureIndex_[16]{
        12, 0, 15, 11, 8, 14, 9, 7, 13, 3, 4, 2, 1, 5, 10, 6
      };
      VIEW_FROM_ARRAY_DANGER(checksToTilesetTextureIndex);

      FOR_RANGE (int, tileType, fb_tiles->size()) {
        const auto fb_tile = fb_tiles->Get(tileType);
        const auto texs    = fb_tile->texture_ids();
        if (!texs)
          continue;

        FOR_RANGE (int, y, sy - 1) {
          FOR_RANGE (int, x, sx - 1) {
            int        checks = 0;
            Vector2Int dd[]{{0, 0}, {0, 1}, {1, 1}, {1, 0}};
            FOR_RANGE (int, i, 4) {
              auto d = dd[i];
              if (fb_levelTiles->Get((y + d.y) * sx + x + d.x) == tileType)
                checks += 1 << i;
            }
            if (!checks)
              continue;
            DrawGroup_CommandTexture({
              .texID = texs->Get(checksToTilesetTextureIndex[checks]),
              .pos{(f32)x, (f32)y},
              .anchor{-0.5f, -0.5f},
            });
          }
        }
      }

      DrawGroup_End();
    }
  }

  // Drawing zones.
  if (gdebug.drawZones) {  ///
    DrawGroup_Begin(DrawZ_DEBUG_TILED_BACKGROUND);
    DrawGroup_SetSortY(0);

    int zoneIndex = -1;
    for (const auto& z : g.run.zones) {
      zoneIndex++;

      const auto r = z.Rect();

      DrawGroup_CommandRect({
        .pos  = r.pos,
        .size = r.size,
        .anchor{},
        .color = Fade(CYAN, 0.5f),
      });

      FOR_RANGE (int, i, z.passengers.count) {
        drawPassenger(
          zoneIndex, {z.passengers[i].posXVisual, z.pos.y}, z.passengers[i], 0
        );
      }
    }

    DrawGroup_End();
  }

  // Drawing player.
  {  ///
    auto color = WHITE;
    if (!pl.CanMove())
      color = ColorLerp(color, RED, 0.25f);

    DrawGroup_Begin(DrawZ_DEFAULT);
    DrawGroup_SetSortY(0);

    DrawGroup_CommandRect({
      .pos      = pl.pos,
      .size     = PLAYER_COLLIDER_SIZE,
      .rotation = pl.rotation,
      .color    = color,
    });
    if (pl.passenger) {
      drawPassenger(
        -1,
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
        IM::Checkbox("Draw Tiles", &gdebug.drawTiles);
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

        IM::EndTabItem();
      }

      IM::EndTabBar();
    }

    IM::End();
  }
}

///
