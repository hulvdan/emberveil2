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

struct Item {  ///
  int color = 0;

  operator bool() const {
    return color > 0;
  }
};

using ItemRow = Array<Item, 3>;

struct Shelf {  ///
  Vector2Int                        posi = {};
  PushableArray<ItemRow, MAX_DEPTH> rows = {};

  FrameGame updatedAt     = {};
  FrameGame lastMatchedAt = {};

  Vector2 pos() const;

  Rect Rect() const {
    return {
      .pos  = pos(),
      .size = ToVector2(glib->shelf_size()),
    };
  }

  bool IsLocked() const {
    return rows[0][0]                                 //
           && (rows[0][0].color == rows[0][1].color)  //
           && (rows[0][1].color == rows[0][2].color);
  }
};

enum PlayerAction {  ///
  PlayerAction_NONE,
  PlayerAction_PICKUP,
  PlayerAction_EXCHANGE,
  PlayerAction_PUT,
};

struct PlayerPos : public Equatable<PlayerPos> {  ///
  int shelf     = -1;
  int itemIndex = -1;

  [[nodiscard]] bool EqualTo(const PlayerPos& other) const {
    return (
      (shelf == other.shelf)  //
      && (itemIndex == other.itemIndex)
    );
  }

  operator bool() const {
    return shelf >= 0;
  }
};

struct GameData {
  struct Meta {
    Font            fontUI      = {};
    LoadFontsResult loadedFonts = {};

    Vector2 screenSizeUI       = {};
    Vector2 screenSizeUIMargin = {};

    bool playerUsesKeyboardOrController = false;

    Vector2Int worldSize  = {};
    Vector2    worldSizef = {};

    Camera camera{};

    bool reload = {};

    glm::mat3 mat                 = {};
    bool      verticalOrientation = {};
  } meta;

  struct Save {
    i32 level = 0;
  } save;

  struct Run {
    u32 randomSeedForLevelHotReload = 0;

    b2WorldId world = {};

    uint        remainingRows = 0;
    FrameVisual gameplayEnded = {};
    bool        won           = false;

    FrameVisual levelControlPressed           = {};
    bool        levelControlPressedSkip       = false;
    FrameVisual levelStartedAfterTransitionAt = {};

    PushableArray<Vector2, 12> bufferedActions = {};

    struct Player {
      PlayerPos posFrom = {};
      PlayerPos pos     = {};

      int          actionItemIndex = -1;
      PlayerAction action          = {};
      FrameGame    actionStartedAt = {};
      Item         item            = {};

      FrameGame infinityAt      = {};
      bool      infinityReverse = false;
    } player;

    // Using "X-macros". ref: https://www.geeksforgeeks.org/c/x-macros-in-c/
    // These containers preserve allocated memory upon resetting state of the run.

    PushableArray<Shelf, 15> shelves = {};

#define VECTORS_TABLE X(BodyShape, bodyShapes)

#define X(type_, name_) Vector<type_> name_ = {};
    VECTORS_TABLE;
#undef X
  } run;
} g = {};

Vector2 Shelf::pos() const {  ///
  return (Vector2)(g.meta.mat * Vector3(posi, 1));
}

lframe GetPlayerActionAndFlyingDuration() {  ///
  const auto& pl      = g.run.player;
  f32         seconds = glib->player_action_duration_seconds();
  if (pl.posFrom != pl.pos)
    seconds += glib->player_fly_duration_seconds();
  return lframe::FromSeconds(seconds);
}

lframe GetPlayerActionWithoutFlyingElapsed() {  ///
  const auto& pl = g.run.player;
  ASSERT(pl.actionStartedAt.IsSet());

  if (!pl.actionStartedAt.IsSet())
    return lframe::Unscaled(0);

  auto e = pl.actionStartedAt.Elapsed();

  if (pl.posFrom != pl.pos) {
    const auto flyDur = lframe::FromSeconds(glib->player_fly_duration_seconds());
    e.value -= flyDur.value;
    if (e.value < 0)
      return lframe::Unscaled(0);
  }

  return e;
}

f32 GetPlayerActionWithoutFlyingProgress() {  ///
  return MIN(
    1,
    GetPlayerActionWithoutFlyingElapsed().Progress(
      lframe::FromSeconds(glib->player_action_duration_seconds())
    )
  );
}

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
  if (g.run.gameplayEnded.IsSet() && g.run.won)
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

int SolvableRowOfThreeExists() {  ///
  FOR_RANGE (int, shelfIndex1, g.run.shelves.count) {
    FOR_RANGE (int, itemIndex1, 3) {
      const auto& item1 = g.run.shelves[shelfIndex1].rows[0][itemIndex1];
      if (!item1)
        continue;

      int countOfThisColor = 0;
      if (g.run.player.item.color == item1.color)
        countOfThisColor++;

      FOR_RANGE (int, shelfIndex2, g.run.shelves.count) {
        FOR_RANGE (int, itemIndex2, 3) {
          const auto& item2 = g.run.shelves[shelfIndex2].rows[0][itemIndex2];
          if (item1.color == item2.color)
            countOfThisColor++;
        }
      }

      if (countOfThisColor >= 3)
        return true;
    }
  }

  return false;
}

int CountEmptySpots() {  ///
  int result = 0;

  FOR_RANGE (int, shelfIndex, g.run.shelves.count) {
    const auto& s = g.run.shelves[shelfIndex];

    FOR_RANGE (int, itemIndex, 3) {
      if (!s.rows[0][itemIndex])
        result++;
    }
  }

  return result;
}

bool PushSpotBack(int shelfIndex, int itemIndex) {  ///
  auto& r = g.run.shelves[shelfIndex].rows;

  if (r.count >= r.maxCount) {
    if (r[r.count - 1][itemIndex])
      return false;  // blocked
  }

  if (r.count < r.maxCount)
    *r.Add() = {};

  for (int d = r.count - 1; d > 0; d--)
    r[d][itemIndex] = r[d - 1][itemIndex];

  r[0][itemIndex] = {};
  return true;
}

void RunInit() {
  ZoneScoped;

  const auto fb_level = GetFBLevel(g.save.level);

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

  // Making shelves.
  {  ///
    ASSERT_FALSE(g.run.shelves.count);

    // int shelfIndex = -1;
    for (auto fb_shelf : *fb_level->shelves()) {
      // shelfIndex++;
      auto& s = *g.run.shelves.Add();
      s       = {.posi = ToVector2(fb_shelf->pos())};
    }
  }

  // Filling shelves with items.
  {  ///
    ZoneScopedN("Filling shelves with items.");

    auto& shelves = g.run.shelves;

    for (auto& s : shelves) {
      s.rows        = {};
      *s.rows.Add() = {};
    }

    g.run.remainingRows
      = Round((f32)fb_level->shelves()->size() * glib->default_item_rows_per_shelf());
    if (fb_level->override_total_item_rows())
      g.run.remainingRows = fb_level->override_total_item_rows();

    int emptySpots = shelves.count * 3;

    FOR_RANGE (int, colorIndex, g.run.remainingRows) {
      FOR_RANGE (int, _, 3) {
        if (!emptySpots) {
          int guard = 0;
          while (1) {
            if (PushSpotBack(GRAND.Rand() % shelves.count, GRAND.Rand() % 3)) {
              emptySpots++;
              break;
            }
            if (guard++ >= 10000)
              INVALID_PATH;
          }
        }

        int shelfIndex, itemIndex = -1;
        int guard = 0;
        while (1) {
          shelfIndex = GRAND.Rand() % shelves.count;
          itemIndex  = GRAND.Rand() % 3;
          if (!g.run.shelves[shelfIndex].rows[0][itemIndex])
            break;
          if (guard++ >= 10000)
            INVALID_PATH;
        }
        ASSERT(shelfIndex >= 0);
        ASSERT(itemIndex >= 0);

        shelves[shelfIndex].rows[0][itemIndex].color = colorIndex + 1;
        emptySpots--;
      }
    }
  }
}

void RunReset() {  ///
  ZoneScoped;

  const bool levelControlPressed = g.run.levelControlPressed.IsSet();
  const auto world               = g.run.world;
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

  if (levelControlPressed)
    g.run.levelStartedAfterTransitionAt.SetNow();
}

void GamePreInit(GamePreInitOpts opts) {  ///
  ZoneScoped;

  ge.settings.backgroundColor = ColorFromRGBA(0xdad0f4ff);

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
  // Setup.
  // {  ///
#if !BF_ENABLE_ASSERTS
  return;
#endif

#ifdef TESTS
  glibFile = LoadFile(GAMELIB_PATH, nullptr);
  glib     = BFGame::GetGameLibrary(glibFile);
#endif
  // }

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

  // Teardown.
  // {  ///
#ifdef TESTS
  glib = nullptr;
  UnloadFile(glibFile);
  glibFile = nullptr;
#endif
  // }
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
  blockInput = g.run.levelControlPressed.IsSet();

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
    if (g.save.level) {  ///
      CLAY({}) {
        BF_CLAY_TEXT_LOCALIZED(Loc_UI_LEVEL_TOP_LEVEL__CAPS);
        BF_CLAY_TEXT(TextFormat(" %d", g.save.level + 1));
      }
    }
  }

  if (g.run.gameplayEnded.IsSet()) {
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
      LAMBDA (bool, componentButton, (Loc loc)) {  ///
        bool result = false;
        CLAY({}) {
          BF_CLAY_TEXT_LOCALIZED(loc);

          result = clickedOrTouchedThisComponent();
        };
        return result;
      };

      // Won / Lost. Next level / Restart button.
      CLAY(  ///
        {
          .layout{
            BF_CLAY_PADDING_HORIZONTAL_VERTICAL(GAP_BIG, GAP_SMALL),
            .childGap = GAP_BIG,
            BF_CLAY_CHILD_ALIGNMENT_CENTER_CENTER,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
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
        if (g.run.won) {
          if (componentButton(Loc_UI_LEVEL_NEXT__CAPS))
            g.run.levelControlPressed.SetNow();
        }
        else {
          if (componentButton(Loc_UI_LEVEL_RESTART__CAPS))
            g.run.levelControlPressed.SetNow();

          if (componentButton(Loc_UI_LEVEL_SKIP__CAPS)) {
            g.run.levelControlPressed.SetNow();
            g.run.levelControlPressedSkip = true;
          }
        }
      }
    }
  }

#undef GAP_SMALL
#undef GAP_BIG

#define BF_UI_POST
#include "engine/bf_clay_ui.cpp"
}

void UpdateCamera() {  ///
  g.meta.camera.pos = g.meta.worldSizef / 2.0f;

  const auto v                = LOGICAL_RESOLUTIONf / g.meta.worldSizef;
  g.meta.camera.zoom          = MIN(v.x, v.y);
  g.meta.camera.texturesScale = 1 / g.meta.camera.zoom;
}

f32 GetItemOffsetX(int itemIndex) {  ///
  ASSERT(itemIndex >= 0);
  ASSERT(itemIndex <= 2);
  const f32 off
    = (glib->shelf_size()->x() / 2 - glib->item_margin()->x() - glib->item_size()->x() / 2);
  return (itemIndex - 1) * off;
}

Vector2 ToWorld(PlayerPos pos) {  ///
  ASSERT(pos);
  return g.run.shelves[pos.shelf].pos()
         + Vector2(GetItemOffsetX(pos.itemIndex), glib->player_inside_shelf_offset_y());
}

Vector2 GetItemBottomPos(int shelf, int itemIndex) {  ///
  return g.run.shelves[shelf].pos()
         + Vector2(
           GetItemOffsetX(itemIndex),
           glib->item_margin()->y() - glib->shelf_size()->y() / 2
         );
}

Rect GetItemRect(int shelf, int itemIndex) {  ///
  return {
    .pos  = GetItemBottomPos(shelf, itemIndex) - Vector2(glib->item_size()->x() / 2, 0),
    .size = ToVector2(glib->item_size()),
  };
}

void EndGameplay() {  ///
  g.run.gameplayEnded.SetNow();
  g.run.bufferedActions.Reset();
}

void GameFixedUpdate() {
  ZoneScoped;

  g.meta.worldSize  = ToVector2Int(glib->world_size());
  g.meta.worldSizef = (Vector2)g.meta.worldSize;

  g.meta.verticalOrientation
    = (ge.meta.screenSize.x / ge.meta.screenSize.y < VERTICAL_RATIO_BREAKPOINT);

  auto& m = g.meta.mat;
  m       = glm::mat3(1);

  m = glm::translate(m, g.meta.worldSizef / 2.0f);
  m *= glm::mat3(
    glib->world_mat_x_scale(), 0, 0, 0, glib->world_mat_y_scale(), 0, 0, 0, 1
  );
  if (g.meta.verticalOrientation)
    m *= glm::mat3(0, -1, 0, 1, 0, 0, 0, 0, 1);
  m = glm::translate(m, -g.meta.worldSizef / 2.0f);

  m = glm::translate(m, {0.5f, 0.5f});

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

    // Buffering player actions.
    if (IsTouchPressed(ge.meta._latestActiveTouchID)) {  ///
      const auto td = GetTouchData(ge.meta._latestActiveTouchID);
      const auto wp = LogicalPosToWorld(ScreenPosToLogical(td.screenPos), &g.meta.camera);
      if (g.run.bufferedActions.count < g.run.bufferedActions.maxCount)
        *g.run.bufferedActions.Add() = wp;
    }

    // Setting player action.
    if (!pl.action) {  ///
      bool actionWasConsumed = false;

      while (g.run.bufferedActions.count) {
        const auto wp = g.run.bufferedActions[0];

        bool skip              = false;
        bool anyItemWasClicked = false;

        int shelf = -1;
        for (auto& s : g.run.shelves) {
          shelf++;

          FOR_RANGE (int, itemIndex, 3) {
            auto& p = s.rows[0][itemIndex];

            const auto r = GetItemRect(shelf, itemIndex);
            if (!r.ContainsInside(wp))
              continue;

            anyItemWasClicked = true;

            PlayerAction actionToSet{};

            if (p && pl.item && (p.color != pl.item.color))
              actionToSet = PlayerAction_EXCHANGE;
            else if (!pl.item && p)
              actionToSet = PlayerAction_PICKUP;
            else if (pl.item && !p)
              actionToSet = PlayerAction_PUT;

            if (actionToSet) {
              if (s.IsLocked())
                goto playerActionWasSet;
              else {
                actionWasConsumed = true;
                pl.action         = actionToSet;
                pl.actionStartedAt.SetNow();
                pl.actionItemIndex = itemIndex;
                pl.posFrom         = pl.pos;
                pl.pos             = {.shelf = shelf, .itemIndex = itemIndex};
                goto playerActionWasSet;
              }
            }
            else {
              skip = true;
              goto playerActionContinue;
            }
          }
        }

        if (!anyItemWasClicked)
          skip = true;

      playerActionContinue:
        if (skip)
          g.run.bufferedActions.RemoveAt(0);
      }

    playerActionWasSet:
      if (actionWasConsumed)
        g.run.bufferedActions.RemoveAt(0);
    }

    const auto actionDur = GetPlayerActionAndFlyingDuration();

    // Resetting player infinity progress and direction in the middle of action.
    if (pl.action) {  ///
      const auto e = pl.actionStartedAt.Elapsed();
      if (e.value == GetPlayerActionAndFlyingDuration().value / 2) {
        pl.infinityAt = {};
        pl.infinityAt.SetNow();
        pl.infinityReverse = !pl.infinityReverse;
      }
    }

    // Committing player action.
    if (pl.action && (pl.actionStartedAt.Elapsed() >= actionDur)) {  ///
      auto& s         = g.run.shelves[pl.pos.shelf];
      auto& shelfItem = s.rows[0][pl.actionItemIndex];

      switch (pl.action) {
      case PlayerAction_PICKUP: {
        pl.item   = shelfItem;
        shelfItem = {};
      } break;

      case PlayerAction_EXCHANGE: {
        std::swap(pl.item, shelfItem);
      } break;

      case PlayerAction_PUT: {
        shelfItem = pl.item;
        pl.item   = {};
      } break;

      default:
        INVALID_PATH;
      }

      pl.action          = {};
      pl.actionStartedAt = {};

      s.updatedAt = {};
      s.updatedAt.SetNow();

      if (s.IsLocked())
        s.lastMatchedAt = s.updatedAt;
    }

    auto parts = glib->matching_particles_parts_seconds();

    LAMBDA (void, makeMatchingParticles, (Vector2 shelfPos, lframe e)) {  ///
      auto pp = shelfPos + Vector2(0, glib->matching_particles_offset_y());

      LAMBDA (bool, part, (int i)) {
        return e == lframe::FromSeconds(parts->Get(i));
      };

      if (part(0))
        MakeParticles({
          .type = ParticleType_CIRCLE,
          .pos  = pp,
          .scalePlusMinus{},
          .rotationSpeedPlusMinus = 0,
        });

      if (part(1))
        MakeParticles({
          .type = ParticleType_DIAMOND_BIG,
          .pos  = pp,
          .scalePlusMinus{},
          .rotation               = glib->matching_particles_big_rotation(),
          .rotationSpeedPlusMinus = 0,
        });

      if (part(2))
        FOR_RANGE (int, i, 5) {
          auto r = glib->matching_particles_big_rotation() + i * 2 * PI32 / 5;
          MakeParticles({
            .type = ParticleType_STAR,
            .pos  = pp + Vector2Rotate({0, 0.6f}, r + PI32),
            .scalePlusMinus{},
            .rotation               = r,
            .rotationSpeedPlusMinus = 0,
          });
        }
    };

    // Shelf matching.
    for (auto& s : g.run.shelves) {  ///
      if (gdebug.matchingParticles) {
        makeMatchingParticles(
          s.pos(), lframe::Unscaled(ge.meta.frameVisual % (2 * FIXED_FPS))
        );
      }

      lframe e{};
      if (s.lastMatchedAt.IsSet()) {
        e = s.lastMatchedAt.Elapsed();
        makeMatchingParticles(s.pos(), e);
      }

      if (!s.IsLocked())
        continue;

      const auto dur = lframe::FromSeconds(glib->shelf_matching_duration_seconds());

      if (e < dur)
        continue;

      s.rows.RemoveAt(0);
      *s.rows.Add() = {};

      g.run.remainingRows--;
      ASSERT(g.run.remainingRows >= 0);
    }

    // Pushing back rows from shelves if front ones are empty.
    for (auto& s : g.run.shelves) {  ///
      auto& r = s.rows[0];
      if ((s.rows.count > 1) && !r[0] && !r[1] && !r[2])
        s.rows.RemoveAt(0);
    }

    if (!g.run.gameplayEnded.IsSet()) {
      // Checking if player's won.
      if (!g.run.remainingRows) {  ///
        EndGameplay();
        g.run.won = true;
        Save();
      }

      // Checking if player's lost.
      else if (!SolvableRowOfThreeExists()) {  ///
        const int requiredEmptySpots = (pl.item ? 3 : 2);
        if (CountEmptySpots() < requiredEmptySpots)
          EndGameplay();
      }
    }

    ge.meta.frameGame++;
  }

  // Advancing level.
  if (g.run.levelControlPressed.IsSet()) {  ///
    if ((g.run.levelControlPressed.Elapsed()
         >= lframe::FromSeconds(glib->level_advance_transition_seconds()))
        && !g.meta.reload)
    {
      if (g.run.won || g.run.levelControlPressedSkip)
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

  BeginMode2D(&g.meta.camera);

  // Drawing tiled background.
  if (gdebug.drawTiledBackground) {  ///
    FOR_RANGE (int, y, g.meta.worldSize.y) {
      FOR_RANGE (int, x, g.meta.worldSize.x) {
        if ((x + y) % 2)
          continue;
        DrawGroup_OneShotRect(
          {
            .pos   = g.meta.mat * Vector3(x, y, 1),
            .size  = g.meta.mat * Vector3(1, 1, 0),
            .color = Fade(WHITE, 0.1f),
          },
          DrawZ_DEBUG_TILED_BACKGROUND
        );
      }
    }
  }

  LAMBDA (
    void, drawItem, (Vector2 pos, const Item& p, f32 rotation, Vector2 scale, f32 depth)
  )
  {  ///
    const f32 dm = glib->item_draw_depth_show_total_rows();
    if (depth > dm)
      return;

    const f32  depthFactor = depth / dm;
    const auto fb          = glib->items()->Get(p.color);
    DrawGroup_CommandTexture({
      .texID    = fb->texture_id(),
      .rotation = rotation,
      .pos      = pos,
      .anchor{0.5f, 0},
      .scale = scale,
      .color = Fade(
        Darken(
          ColorFromRGBA(fb->color()),
          Lerp(1, glib->item_draw_depth_darken_min(), depthFactor)
        ),
        Lerp(1, glib->item_draw_depth_fade_min(), depthFactor)
      ),
    });
  };

  int        actualLevelIndex = -1;
  const auto fb_level         = GetFBLevel(g.save.level, &actualLevelIndex);

  Vector2    playerPos{};
  Vector2    playerItemPos{};
  const auto infinityDur = lframe::FromSeconds(glib->player_infinity_duration_seconds());
  f32        infinityP   = 0;
  if (pl.infinityAt.IsSet()) {
    infinityP
      = (f32)(pl.infinityAt.Elapsed().value % infinityDur.value) / (f32)infinityDur.value;
    if (pl.infinityReverse)
      infinityP *= -1;
  }
  else
    infinityP = (f32)(ge.meta.frameGame % infinityDur.value) / (f32)infinityDur.value;

  const auto playerPosInfinityOffset
    = InfinitySymbol(infinityP) * ToVector2(glib->player_infinity_symbol_offset());

  // f32 playerRotation = InfinitySymbolRotation(infinityP);
  f32 playerRotation = 0;

  // Calculating player data.
  {  ///
    playerPos = ToVector2(fb_level->player());
    if (g.meta.verticalOrientation)
      playerPos += Vector2(0.5f, -0.5f);
    else
      playerPos += Vector2(-0.5f, 0.5f);

    if (pl.posFrom)
      playerPos = ToWorld(pl.posFrom);

    f32 playerInfinityPosOffsetScale = 1;

    if (pl.pos) {
      const auto pos2 = ToWorld(pl.pos);
      if (pl.actionStartedAt.IsSet()) {
        const auto actionDur = lframe::FromSeconds(glib->player_fly_duration_seconds());
        f32        posP      = pl.actionStartedAt.Elapsed().Progress(actionDur);
        posP                 = MIN(1, posP);
        playerPos            = Vector2Lerp(playerPos, pos2, EaseInOutQuad(posP));
      }
      else
        playerPos = pos2;

      if (pl.actionStartedAt.IsSet()) {
        f32 posP
          = pl.actionStartedAt.Elapsed().Progress(GetPlayerActionAndFlyingDuration());
        playerInfinityPosOffsetScale = 1 - EaseInQuad(sinf(posP * PI32));
      }
    }

    playerPos += playerPosInfinityOffset * playerInfinityPosOffsetScale;

    playerItemPos
      = playerPos
        + Vector2Rotate({0, glib->item_inside_player_offset_y()}, playerRotation);
  }

  DrawGroup_Begin(DrawZ_DEFAULT);
  DrawGroup_SetSortY(0);

  // Drawing shelves.
  FOR_RANGE (int, mode, 2) {  ///
    // mode 0 - drawing shelves, 1 - drawing items.
    int shelf = -1;
    for (const auto& s : g.run.shelves) {
      shelf++;

      const auto r = s.Rect();

      if (!mode) {
        DrawGroup_CommandTexture({
          .texID = glib->game_shelf_texture_id(),
          .pos   = r.pos,
        });
      }
      else {
        for (int depth = s.rows.count - 1; depth >= 0; depth--) {
          Vector2 scale{1, 1};
          if (s.IsLocked() && !depth) {
            const auto dur = lframe::FromSeconds(glib->shelf_matching_duration_seconds());
            const auto p   = s.updatedAt.Elapsed().Progress(dur);
            scale *= Lerp(1, glib->shelf_matching_item_scale(), sinf(p * PI32));
          }

          FOR_RANGE (int, itemIndex, 3) {
            auto& p   = s.rows[depth][itemIndex];
            auto  pos = GetItemBottomPos(shelf, itemIndex);
            if (p) {
              Vector2 actionOffset{};
              if (!depth) {
                if ((pl.action == PlayerAction_PICKUP)
                    || (pl.action == PlayerAction_EXCHANGE))
                {
                  if ((shelf == pl.pos.shelf) && (itemIndex == pl.actionItemIndex)) {
                    f32 p        = GetPlayerActionWithoutFlyingProgress();
                    actionOffset = Vector2Lerp({}, playerItemPos - pos, EaseInOutQuad(p));
                  }
                }
              }

              drawItem(pos + actionOffset, p, 0, scale, (f32)depth);
            }

            if (gdebug.gizmos && !depth) {
              const auto r = GetItemRect(shelf, itemIndex);
              DrawGroup_CommandRectLines({
                .pos    = r.pos,
                .size   = r.size,
                .anchor = {},
                .color  = YELLOW,
              });
            }
          }
        }
      }
    }
  }

  // Drawing player.
  {  ///
    auto color = WHITE;

    if (pl.item) {
      Vector2 actionOffset{};
      if ((pl.action == PlayerAction_PUT) || (pl.action == PlayerAction_EXCHANGE)) {
        f32  p         = GetPlayerActionWithoutFlyingProgress();
        auto targetPos = GetItemBottomPos(pl.pos.shelf, pl.actionItemIndex);
        actionOffset   = Vector2Lerp({}, targetPos - playerItemPos, EaseInOutQuad(p));
      }

      drawItem(playerItemPos + actionOffset, pl.item, playerRotation, {1, 1}, 0);
    }

    DrawGroup_CommandTexture({
      .texID    = glib->player_texture_id(),
      .rotation = playerRotation,
      .pos      = playerPos,
      .color    = color,
    });
  }

  DrawParticles();

  DrawGroup_End();

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

  DoUI();

  // Level advancing transition.
  {  ///
    const auto transitionDur
      = lframe::FromSeconds(glib->level_advance_transition_seconds());

    if (g.run.levelStartedAfterTransitionAt.IsSet()) {
      ge.settings.screenFade = MAX(
        0, 1 - g.run.levelStartedAfterTransitionAt.Elapsed().Progress(transitionDur)
      );
    }
    if (g.run.levelControlPressed.IsSet()) {
      ge.settings.screenFade
        = MIN(1, g.run.levelControlPressed.Elapsed().Progress(transitionDur));
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
        IM::Checkbox("Draw Matching Particles", &gdebug.matchingParticles);

        if (IM::Button("Win") && !g.run.gameplayEnded.IsSet()) {
          EndGameplay();
          g.run.won = true;
        }

        if (IM::Button("Lose") && !g.run.gameplayEnded.IsSet())
          EndGameplay();

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

Color particleColors[]{
  ColorFromRGBA(0xe83b3bff),
  // ColorFromRGBA(0xfb6b1dff),
  ColorFromRGBA(0xfbb954ff),
  ColorFromRGBA(0xfbff86ff),
  ColorFromRGBA(0xfbb954ff),
  ColorFromRGBA(0xeaadedff),
};

void ParticleRender_COMMON(f32 p, lframe e, ParticleRenderData data) {  ///
  auto color    = particleColors[ProgressToIndex(p, ARRAY_COUNT(particleColors))];
  data.color->r = color.r;
  data.color->g = color.g;
  data.color->b = color.b;
}

///
