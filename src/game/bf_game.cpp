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

Color CLOUD_COLORS_[]{
  PAL_ORANGE,            // #fb6b1d
  PAL_ALIZARIN_CRIMSON,  // #e83b3b
  // PAL_DISCO,             // #831c5d
  PAL_MAROON_FLUSH,     // #c32454
  PAL_WILD_WATERMELON,  // #f04f78
  PAL_GERALDINE,        // #f68181
  PAL_MONA_LISA,        // #fca790
  PAL_CALICO,           // #e3c896
  // PAL_SANDRIFT,          // #ab947a
  PAL_COPPER_ROSE,  // #966c6c
  // PAL_SCORPION,          // #625565
  // PAL_SHIP_GRAY,         // #3e3546
  // PAL_CHATHAMS_BLUE,     // #0b5e65
  // PAL_TEAL,              // #0b8a8f
  PAL_JADE,        // #1ebc73
  PAL_CONIFER,     // #91db69
  PAL_DOLLY,       // #fbff86
  PAL_CASABLANCA,  // #fbb954
  // PAL_PIPER,             // #cd683d
  // PAL_MULE_FAWN,         // #9e4539
  // PAL_TAWNY_PORT,        // #7a3045
  // PAL_COSMIC,            // #6b3e75
  // PAL_TRENDY_PINK,       // #905ea9
  PAL_DULL_LAVENDER,  // #a884f3
  PAL_MAUVE,          // #eaaded
  PAL_CORNFLOWER,     // #8fd3ff
  PAL_HAVELOCK_BLUE,  // #4d9be6
  // PAL_SAN_MARINO,        // #4d65b4
  // PAL_EAST_BAY,          // #484a77
  PAL_SHAMROCK,  // #30e1b9
  PAL_ANAKIWA,   // #8ff8e2
};
VIEW_FROM_ARRAY_DANGER(CLOUD_COLORS);

struct Shelf {  ///
  Vector2Int                        posi = {};
  PushableArray<ItemRow, MAX_DEPTH> rows = {};

  FrameGame updatedAt     = {};
  FrameGame lastMatchedAt = {};

  int color               = {};
  int cloudPartIndices[3] = {};

  FrameVisual clickedAt[3] = {};

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

struct PlayerBufferedAction {  ///
  Vector2 pos     = {};
  TouchID touchID = {};
  bool    press   = {};
};

struct PlayerLastAction : public Equatable<PlayerLastAction> {  ///
  TouchID touchID = {};
  int     shelf   = -1;
  int     item    = -1;

  [[nodiscard]] bool EqualTo(const PlayerLastAction& other) const {
    return (
      (touchID == other.touchID)  //
      && (shelf == other.shelf)   //
      && (item == other.item)
    );
  }
};

struct FirstLevelTutorMove {  ///
  int shelf = {};
  int item  = {};
};

FirstLevelTutorMove FIRST_LEVEL_TUTOR_MOVES_[]{
  {.shelf = 0, .item = 1},
  {.shelf = 2, .item = 0},
  {.shelf = 0, .item = 2},
  {.shelf = 1, .item = 2},
  {.shelf = -1, .item = -1},
};
VIEW_FROM_ARRAY_DANGER(FIRST_LEVEL_TUTOR_MOVES);

constexpr int MAX_SHELVES = 15;

#define BF_ACTIVATE_TOUCH_SPOTS_UPON_CONSUMING (0)
#define BF_ACTIVATE_TOUCH_SPOTS_UPON_CLICKING_AS_OPPOSED_TO_CONSUMING (0)

struct GameData {
  struct Meta {
    Font            fontUI             = {};
    Font            fontWinLabel       = {};
    Font            fontWinDescription = {};
    LoadFontsResult loadedFonts        = {};

    Vector2 screenSizeUI       = {};
    Vector2 screenSizeUIMargin = {};

    bool playerUsesKeyboardOrController = false;

    Vector2Int worldSize  = {};
    Vector2    worldSizef = {};

    Camera camera{};

    bool reload = {};

    glm::mat3 mat                 = {};
    bool      verticalOrientation = {};

    f32 cloudsBreathingP = {};
    int perlinIndex      = {};
  } meta;

  struct Save {
    i32 level = 0;

    int volumeSFX   = 3;
    int volumeMusic = 2;
  } save;

  struct Run {
    u32 randomSeedForLevelHotReload = 0;

    b2WorldId world = {};

    uint                       remainingRows                     = 0;
    FrameVisual                gameplayEnded                     = {};
    bool                       won                               = false;
    int                        lostFrontUniqueColorsCount        = 0;
    Array<lframe, TOTAL_ITEMS> lostFrontUniqueColorsStartFlashes = {};
    int                        wonOrLostLabelIndex               = -1;

    FrameVisual levelControlPressed           = {};
    bool        levelControlPressedSkip       = false;
    FrameVisual levelStartedAfterTransitionAt = {};

    PushableArray<PlayerBufferedAction, 12> bufferedActions = {};

    int         firstLevelTutorMoveIndex = 0;
    FrameVisual firstLevelTutorPressed   = {};
    int         visuallyLastPressedItem  = -1;

    struct Player {
      PlayerPos posFrom = {};
      PlayerPos pos     = {};

      int          actionItemIndex = -1;
      PlayerAction action          = {};
      FrameGame    actionStartedAt = {};
      Item         item            = {};

      FrameGame infinityAt      = {};
      bool      infinityReverse = false;

      PlayerLastAction lastAction = {};
    } player;

    PerlinParams perlinParams = {};

    // Using "X-macros". ref: https://www.geeksforgeeks.org/c/x-macros-in-c/
    // These containers preserve allocated memory upon resetting state of the run.

    Array<int, TOTAL_ITEMS>           colorIndexToItemIndex = {};
    PushableArray<Shelf, MAX_SHELVES> shelves               = {};

    Array<Array<u16, 8192>, MAX_SHELVES> perlinX = {};
    Array<Array<u16, 8192>, MAX_SHELVES> perlinY = {};

    int backgroundColorIndex = {};

#define VECTORS_TABLE X(BodyShape, bodyShapes)

#define X(type_, name_) Vector<type_> name_ = {};
    VECTORS_TABLE;
#undef X
  } run;
} g = {};

f32 CalculateItemScaleInsidePlayer(const Item& item) {  ///
  ASSERT(item);
  const auto itemIndex = g.run.colorIndexToItemIndex[item.color];
  const auto fb_item   = glib->items()->Get(itemIndex);
  return glib->item_inside_player_size_y_for_scale_calculation()
         / glib->original_texture_sizes()->Get(fb_item->texture_id())->y();
}

Vector2 ShelfPos(int shelf) {  ///
  const auto& s  = g.run.shelves[shelf];
  const auto  px = (f32)g.run.perlinX[shelf][g.meta.perlinIndex] / (f32)u16_max;
  const auto  py = (f32)g.run.perlinY[shelf][g.meta.perlinIndex] / (f32)u16_max;
  return (Vector2)(g.meta.mat * Vector3(s.posi, 1))
         + Vector2(Lerp(-1, 1, px), Lerp(-1, 1, py))
             * ToVector2(glib->clouds_perlin_offset());
}

Rect ShelfRect(int shelf) {  ///
  return {.pos = ShelfPos(shelf), .size = ToVector2(glib->shelf_size())};
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
      auto& s               = *g.run.shelves.Add();
      s                     = {.posi = ToVector2(fb_shelf->pos())};
      s.cloudPartIndices[0] = VRAND.Rand() % glib->clouds_left()->size();
      s.cloudPartIndices[1] = VRAND.Rand() % glib->clouds_center()->size();
      s.cloudPartIndices[2] = VRAND.Rand() % glib->clouds_right()->size();
    }
  }

  auto& shelves = g.run.shelves;
  for (auto& s : shelves) {
    s.rows        = {};
    *s.rows.Add() = {};
  }

  // Placing items manually.
  if (fb_level->manually_placed_rows()) {  ///
    g.run.remainingRows = fb_level->manually_placed_rows();

    int shelf = -1;

    LAMBDA (void, placeItem, (int item, int color)) {
      ASSERT(color);
      shelves[shelf].rows[0][item].color = color;
    };

    for (auto s : *fb_level->shelves()) {
      shelf++;

      if (s->manual_item1())
        placeItem(0, s->manual_item1());
      if (s->manual_item2())
        placeItem(1, s->manual_item2());
      if (s->manual_item3())
        placeItem(2, s->manual_item3());
    }
  }
  // Automatically filling shelves with items.
  else {  ///
    ZoneScopedN("Automatically filling shelves with items.");

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

  FOR_RANGE (int, i, TOTAL_ITEMS)
    g.run.colorIndexToItemIndex[i] = i;
  GRAND.Shuffle(g.run.colorIndexToItemIndex.base, TOTAL_ITEMS);

  int colorsArr[g.run.shelves.maxCount]{};
  FOR_RANGE (int, i, g.run.shelves.maxCount)
    colorsArr[i] = i;
  GRAND.Shuffle(colorsArr, g.run.shelves.maxCount);
  FOR_RANGE (int, i, g.run.shelves.count)
    g.run.shelves[i].color = colorsArr[i];

  // g.run.backgroundColorIndex = GRAND.Rand() % glib->background_colors2()->size();
  g.run.backgroundColorIndex = g.save.level % glib->background_colors2()->size();
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

  ge.settings.backgroundColor = TextifyColor(PAL_MAUVE);

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

  static auto fontpath = "res/arco.ttf";

  // static int  numberCodepoints[]{
  //   ' ', '+', '-', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'x'
  // };

  static LoadFontData loadFontData_[]{
    // fontUI.
    {
      .filepath        = fontpath,
      .size            = 18,
      .FIXME_sizeScale = 43.0f / 30.0f,
      .codepoints      = g_codepoints,
      .codepointsCount = ARRAY_COUNT(g_codepoints),
      .outlineWidth    = 3,
      .outlineAdvance  = 1,
    },
    // fontWinLabel.
    {
      .filepath        = fontpath,
      .size            = 65,
      .FIXME_sizeScale = 43.0f / 30.0f,
      .codepoints      = g_codepoints,
      .codepointsCount = ARRAY_COUNT(g_codepoints),
      .outlineWidth    = 5,
      .outlineAdvance  = 5,
    },
    // fontWinDescription.
    {
      .filepath        = fontpath,
      .size            = 40,
      .FIXME_sizeScale = 30.0f / 30.0f,
      .codepoints      = g_codepoints,
      .codepointsCount = ARRAY_COUNT(g_codepoints),
      .outlineWidth    = 4,
      .outlineAdvance  = 3,
    },
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

bool PlayerCanHandleControls() {  ///
  if (gdebug.drawWin || gdebug.drawLost)
    return false;
  if (g.run.gameplayEnded.IsSet())
    return false;
  if (!ge.soundManager.unlocked.IsSet())
    return false;
  return true;
}

// NOTE: Logic must be executed only when `ge.meta._drawing` (`draw`) is false!
// e.g. updating mouse position, processing `clicked()`,
// logically reacting to `Clay_Hovered()`, changing game's state, etc.
void DoUI() {
  // Setup.
  // {  ///
#define BF_UI_PRE
#include "engine/bf_clay_ui.cpp"

  auto& pl   = g.run.player;
  blockInput = g.run.levelControlPressed.IsSet();

  const f32 breathingScale
    = 1 + 0.05f * BreathingP(ge.meta.frameVisual, lframe::FromSeconds(2.5f));

#define GAP_SMALL (8)
#define GAP_BIG (20)

  enum ButtonID {
    ButtonID_INVALID,
    ButtonID_NEXT,
    ButtonID_RESTART,
    ButtonID_SKIP,
    ButtonID_SFX,
    ButtonID_MUSIC,
    ButtonID_COUNT,
  };

  static FrameVisual buttonPressedAt_[ButtonID_COUNT]{};
  VIEW_FROM_ARRAY_DANGER(buttonPressedAt);
  // }

  LAMBDA (f32, progressify, (lframe * e, const lframe& dur)) {  ///
    auto result = Clamp01(e->Progress(dur));
    e->value -= dur.value * 2 / 3;
    return result;
  };

  struct ComponentButtonData {  ///
    ButtonID id           = {};
    int      iconTexID    = {};
    int      iconTexID2   = {};
    lframe*  e            = {};
    bool     noBackground = {};
  };

  LAMBDA (bool, componentButton, (ComponentButtonData data, auto innerLambda)) {  ///
    ASSERT(data.id);
    ASSERT(data.iconTexID);

    bool result = false;
    auto color  = WHITE;

    auto& pressedAt = buttonPressedAt[data.id];
    if (pressedAt.IsSet()
        && pressedAt.Elapsed()
             <= lframe::FromSeconds(glib->button_press_duration_seconds()))
      color = Darken(color, glib->button_press_darken());

    f32 p = 1;
    if (data.e)
      p = progressify(data.e, ANIMATION_1_FRAMES);

    CLAY({}) {
      auto backgroundColor = color;
      if (data.noBackground)
        backgroundColor.a = 0;
      BF_CLAY_IMAGE(
        {
          .texID = glib->ui_button_texture_id(),
          .scale = Vector2One() * EaseBounceSmallSmooth(p) * breathingScale,
          .color = backgroundColor,
          .dontCareAboutScaleWhenCalculatingSize = true,
        },
        [&]() BF_FORCE_INLINE_LAMBDA {
          CLAY({.layout{
            BF_CLAY_SIZING_GROW_XY,
            BF_CLAY_CHILD_ALIGNMENT_CENTER_CENTER,
          }})
          BF_CLAY_IMAGE({
            .texID = data.iconTexID,
            .scale = Vector2One() * EaseBounceSmallSmooth(p) * breathingScale,
            .color = color,
            .dontCareAboutScaleWhenCalculatingSize = true,
          });
          if (data.iconTexID2) {
            BF_CLAY_IMAGE({
              .texID = data.iconTexID2,
              .scale = Vector2One() * EaseBounceSmallSmooth(p) * breathingScale,
              .color = color,
              .dontCareAboutScaleWhenCalculatingSize = true,
            });
          }
        }
      );

      innerLambda(p);

      if (p > 0.3f) {
        result = clickedOrTouchedThisComponent();
        if (result) {
          pressedAt = {};
          pressedAt.SetNow();
        }
      }
    };

    return result;
  };

  LAMBDA (void, componentVolumeButtons, ()) {  ///
    LAMBDA (void, componentButtonVolume, (int iconTexID, ButtonID id, int* var)) {
      const bool clicked = componentButton(
        {
          .id           = id,
          .iconTexID    = iconTexID,
          .iconTexID2   = glib->ui_icon_volume_bands_texture_ids()->Get(MAX(0, *var - 1)),
          .noBackground = true,
        },
        [](f32 p) {}
      );

      if (clicked) {
        *var = *var - 1;
        if (*var < 0)
          *var = 3;
        // PlaySound(Sound_UI_CLICK);
        Save();
      }
    };

    componentButtonVolume(
      glib->ui_icon_sfx_texture_id(), ButtonID_SFX, &g.save.volumeSFX
    );
    componentButtonVolume(
      glib->ui_icon_music_texture_id(), ButtonID_MUSIC, &g.save.volumeMusic
    );
  };

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
  ) {
    FLOATING_BEAUTIFY;

    CLAY({.layout{BF_CLAY_SIZING_GROW_XY}}) {
      // Current level.
      if (g.save.level) {  ///
        CLAY({}) {
          BF_CLAY_TEXT_LOCALIZED(Loc_UI_LEVEL_TOP_LEVEL__CAPS);
          BF_CLAY_TEXT(TextFormat(" %d", g.save.level + 1));
        }
      }
    }
  }

  if (gdebug.drawWin || gdebug.drawLost || g.run.gameplayEnded.IsSet()) {
    lframe endE{};
    if (gdebug.drawWin || gdebug.drawLost)
      endE.value = ge.meta.frameVisual % (FIXED_FPS * 6);
    else
      endE = g.run.gameplayEnded.Elapsed();
    endE.value
      -= lframe::FromSeconds(
           (glib->lost_items_flashing_seconds() + glib->lost_items_flashing_gap_seconds())
           * g.run.lostFrontUniqueColorsCount
      )
           .value;

    // dummy wait.
    progressify(&endE, ANIMATION_0_FRAMES);
    progressify(&endE, ANIMATION_0_FRAMES);

    const auto stripP = progressify(&endE, ANIMATION_1_FRAMES);

    componentOverlay([]() {}, stripP);

    CLAY(  ///
      {
        .layout{
          BF_CLAY_SIZING_GROW_XY,
          BF_CLAY_CHILD_ALIGNMENT_CENTER_CENTER,
        },
        .floating{
          .zIndex             = zIndex,
          .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
          .attachTo           = CLAY_ATTACH_TO_PARENT,
        },
      }
    ) {
      FLOATING_BEAUTIFY;

      LAMBDA (void, componentVerticalBlackStrip, ()) {  ///
        CLAY({
          .layout{.sizing{.width = CLAY_SIZING_FIXED(6), .height = CLAY_SIZING_GROW(0)}},
          .backgroundColor = ToClayColor(Fade(BLACK, stripP)),
        }) {}
      };

      // Won / Lost. Next level / Restart button.
      CLAY(  ///
        {
          .layout{
            .sizing{
              .width  = CLAY_SIZING_FIXED(glib->ui_sizing_win_x()),
              .height = CLAY_SIZING_GROW(0),
            },
          },
          .backgroundColor = ToClayColor(Fade(PAL_COPPER_ROSE, stripP)),
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
        FLOATING_BEAUTIFY;

        const bool won = (g.run.won || gdebug.drawWin);

        // Assert.
        if (g.run.wonOrLostLabelIndex < 0) {  ///
          g.run.wonOrLostLabelIndex = 0;
          auto allowed              = gdebug.drawWin || gdebug.drawLost;
          ASSERT(allowed);
        }

        componentVerticalBlackStrip();

        CLAY(  ///
          {.layout{
            BF_CLAY_SIZING_GROW_XY,
            BF_CLAY_PADDING_HORIZONTAL_VERTICAL(GAP_BIG, GAP_SMALL),
            .childGap = GAP_BIG,
            BF_CLAY_CHILD_ALIGNMENT_CENTER_CENTER,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
          }}
        ) {
          // dummy wait.
          progressify(&endE, ANIMATION_0_FRAMES);

          // Stars.
          CLAY({.layout{
            .sizing{.height = CLAY_SIZING_FIXED(glib->ui_stars_element_height())}
          }}) {  ///
            CLAY({
              .layout{.childGap = glib->ui_stars_child_gap()},
              .floating{
                .zIndex = zIndex,
                .attachPoints{
                  .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                  .parent  = CLAY_ATTACH_POINT_CENTER_CENTER,
                },
                .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                .attachTo           = CLAY_ATTACH_TO_PARENT,
              },
            }) {
              FLOATING_BEAUTIFY;

              FOR_RANGE (int, i, 3) {
                const auto rot = -(i - 1) * PI32 / 8;
                BF_CLAY_IMAGE(
                  {
                    .texID    = glib->ui_star_gray_texture_id(),
                    .rotation = rot,
                    .offset{0, (i - 1 ? 0 : 40)},
                    .color = Fade(WHITE, stripP),
                  },
                  [&]() BF_FORCE_INLINE_LAMBDA {
                    if (!won)
                      return;
                    CLAY({
                      .floating{
                        .zIndex = zIndex,
                        .attachPoints{
                          .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                          .parent  = CLAY_ATTACH_POINT_CENTER_CENTER,
                        },
                        .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                        .attachTo           = CLAY_ATTACH_TO_PARENT,
                      },
                    }) {
                      FLOATING_BEAUTIFY;

                      BF_CLAY_IMAGE({
                        .texID    = glib->ui_star_gold_texture_id(),
                        .rotation = rot,
                        .offset{0, (i - 1 ? 0 : 40)},
                        .scale
                        = Vector2One()
                          * EaseBounceSmallSmooth(progressify(&endE, ANIMATION_1_FRAMES))
                          * breathingScale,
                        .dontCareAboutScaleWhenCalculatingSize = true,
                      });
                    }
                  }
                );
              }
            }
          }

          // Label.
          CLAY(  ///
            {.layout{
              BF_CLAY_SIZING_GROW_X,
              .childGap = GAP_BIG,
              BF_CLAY_CHILD_ALIGNMENT_CENTER_CENTER,
              .layoutDirection = CLAY_TOP_TO_BOTTOM,
            }}
          )  //
          {  ///
            CLAY({.layout{
              .sizing{.height = CLAY_SIZING_FIXED(glib->ui_label_element_height())}
            }})
            CLAY({
              .floating{
                .zIndex = zIndex,
                .attachPoints{
                  .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                  .parent  = CLAY_ATTACH_POINT_CENTER_CENTER,
                },
                .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                .attachTo           = CLAY_ATTACH_TO_PARENT,
              },
            }) {
              FLOATING_BEAUTIFY;

              FontBegin(&g.meta.fontWinLabel);
              BF_CLAY_TEXT_LOCALIZED(
                (Loc)((int)(won ? Loc_UI_WON_1__CAPS : Loc_UI_LOST_1__CAPS)
                      + g.run.wonOrLostLabelIndex),
                {
                  .scale = Vector2One()
                           * EaseBounceSmallSmooth(progressify(&endE, ANIMATION_1_FRAMES))
                           * breathingScale,
                  .color    = Fade(won ? PAL_CASABLANCA : PAL_ALIZARIN_CRIMSON, stripP),
                  .wrapMode = CLAY_TEXT_WRAP_NONE,
                  .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                }
              );
              FontEnd();
            }

            FontBegin(&g.meta.fontWinDescription);
            BF_CLAY_TEXT_LOCALIZED(
              (won ? Loc_UI_WON_DESCRIPTION : Loc_UI_LOST_DESCRIPTION),
              {
                .scale = Vector2One()
                         * EaseBounceSmallSmooth(progressify(&endE, ANIMATION_1_FRAMES))
                         * breathingScale,
                .color         = Fade(WHITE, stripP),
                .textAlignment = CLAY_TEXT_ALIGN_CENTER,
              }
            );
            FontEnd();
          }

          BF_CLAY_SPACER_VERTICAL;

          // Buttons.
          CLAY({.layout{.childGap = GAP_BIG * 4}}) {  ///
            Beautify b{.alpha = stripP};
            BEAUTIFY(b);

            if (won) {
              if (componentButton(
                    {
                      .id        = ButtonID_NEXT,
                      .iconTexID = glib->ui_icon_next_texture_id(),
                      .e         = &endE,
                    },
                    [](f32 p) {}
                  ))
                g.run.levelControlPressed.SetNow();
            }
            else {
              if (componentButton(
                    {
                      .id        = ButtonID_RESTART,
                      .iconTexID = glib->ui_icon_restart_texture_id(),
                      .e         = &endE,
                    },
                    [](f32 p) {}
                  ))
                g.run.levelControlPressed.SetNow();

#if BF_DEBUG || defined(BF_PLATFORM_WebYandex)
              if (componentButton(
                    {
                      .id        = ButtonID_SKIP,
                      .iconTexID = glib->ui_icon_skip_texture_id(),
                      .e         = &endE,
                    },
                    [&](f32 p) BF_FORCE_INLINE_LAMBDA {
                      CLAY({.floating{
                        .zIndex = zIndex,
                        .attachPoints{
                          .element = CLAY_ATTACH_POINT_CENTER_CENTER,
                          .parent  = CLAY_ATTACH_POINT_RIGHT_TOP,
                        },
                        .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                        .attachTo           = CLAY_ATTACH_TO_PARENT,
                      }}) {
                        BF_CLAY_IMAGE({
                          .texID = glib->ui_icon_ad_small_texture_id(),
                          .scale
                          = Vector2One() * EaseBounceSmallSmooth(p) * breathingScale,
                          .color                                 = PAL_CASABLANCA,
                          .dontCareAboutScaleWhenCalculatingSize = true,
                        });
                      }
                    }
                  ))
                ShowAdReward(&g.run.levelControlPressedSkip);
#endif
            }
          }

          BF_CLAY_SPACER_VERTICAL;
        }

        componentVerticalBlackStrip();
      }
    }
  }

  if (g.run.levelControlPressedSkip && !g.run.levelControlPressed.IsSet()) {
    g.run.levelControlPressed = {};
    g.run.levelControlPressed.SetNow();
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
  {
    constexpr f32 MAX_ZOOM = 1.8f;
    auto&         u        = ge.soundManager.unlocked;
    if (u.IsSet()) {
      if (u._value != 0) {
        auto p = u.Elapsed().Progress(ANIMATION_2_FRAMES);
        p      = MIN(1, p);
        p      = EaseInOutQuad(p);
        g.meta.camera.zoom *= Lerp(MAX_ZOOM, 1, p);
      }
    }
    else
      g.meta.camera.zoom *= MAX_ZOOM;
  }
}

f32 GetItemOffsetX(int itemIndex) {  ///
  ASSERT(itemIndex >= 0);
  ASSERT(itemIndex <= 2);
  const f32 off
    = (glib->shelf_size()->x() / 2 - glib->item_margin()->x() - glib->item_collider_size()->x() / 2);
  return (itemIndex - 1) * off;
}

Vector2 ToWorld(PlayerPos pos) {  ///
  ASSERT(pos);
  return ShelfPos(pos.shelf)
         + Vector2(GetItemOffsetX(pos.itemIndex), glib->player_inside_shelf_offset_y());
}

Vector2 GetItemBottomPos(int shelf, int itemIndex, bool visual = false) {  ///
  auto result
    = ShelfPos(shelf)
      + Vector2(
        GetItemOffsetX(itemIndex), glib->item_margin()->y() - glib->shelf_size()->y() / 2
      );

  if (visual) {
    result += Vector2(glib->item_breathing_offset_x() * (itemIndex - 1), 0)
              * (1 - g.meta.cloudsBreathingP);
  }

  return result;
}

Rect GetItemRect(int shelf, int itemIndex) {  ///
  return {
    .pos = GetItemBottomPos(shelf, itemIndex)
           - Vector2(glib->item_collider_size()->x() / 2, glib->item_collider_offset_y()),
    .size = ToVector2(glib->item_collider_size()),
  };
}

void EndGameplay(bool won) {  ///
  g.run.gameplayEnded.SetNow();
  g.run.bufferedActions.Reset();

  g.run.won = won;

  if (!won) {
    PushableArray<int, TOTAL_ITEMS> frontItemColors{};

    for (const auto& s : g.run.shelves) {
      FOR_RANGE (int, i, 3) {
        const auto& item = s.rows[0][i];
        if (!item)
          continue;
        if (!frontItemColors.Contains(item.color))
          *frontItemColors.Add() = item.color;
      }
    }
    VRAND.Shuffle(frontItemColors._base, frontItemColors.count);
    lframe e{.value = 0};
    auto   interval = lframe::FromSeconds(
      glib->lost_items_flashing_seconds() + glib->lost_items_flashing_gap_seconds()
    );
    g.run.lostFrontUniqueColorsCount = frontItemColors.count;
    FOR_RANGE (int, i, frontItemColors.count) {
      g.run.lostFrontUniqueColorsStartFlashes[frontItemColors[i]] = e;
      e.value += interval.value;
    }
  }

  // while (1) {
  //   const int v = VRAND.Rand() % 4;
  //   if (g.run.wonOrLostLabelIndex != v) {
  //     g.run.wonOrLostLabelIndex = v;
  //     break;
  //   }
  // }
  g.run.wonOrLostLabelIndex = 0;
}

void GameFixedUpdate() {
  ZoneScoped;

  // Setup. {  ///
  g.meta.worldSize  = ToVector2Int(glib->world_size());
  g.meta.worldSizef = (Vector2)g.meta.worldSize;

  g.meta.verticalOrientation
    = (ge.meta.screenSize.x / ge.meta.screenSize.y < VERTICAL_RATIO_BREAKPOINT);

  {
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
  }

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

  bool canGameplay = true;
  if (ShouldGameplayStop())
    canGameplay = false;
  if (g.run.gameplayEnded.IsSet()
      && (g.run.gameplayEnded.Elapsed() >= ANIMATION_2_FRAMES))
    canGameplay = false;

  if (canGameplay) {
    MarkGameplay();

    // Buffering player actions.
    if (PlayerCanHandleControls()) {  ///
      bool handle = false;
      bool press  = false;

      if (IsTouchPressed(ge.meta._latestActiveTouchID)) {
        handle = true;
        press  = true;
      }
      else if (IsTouchReleased(ge.meta._latestActiveTouchID))
        handle = true;

      if (handle) {
        const auto td = GetTouchData(ge.meta._latestActiveTouchID);
        const auto wp
          = LogicalPosToWorld(ScreenPosToLogical(td.screenPos), &g.meta.camera);

        if (BF_ACTIVATE_TOUCH_SPOTS_UPON_CLICKING_AS_OPPOSED_TO_CONSUMING) {
          int shelf = -1;
          for (auto& s : g.run.shelves) {
            shelf++;
            FOR_RANGE (int, itemIndex, 3) {
              auto index = shelf * 3 + itemIndex;
              if (!press && (index == g.run.visuallyLastPressedItem))
                continue;

              if (GetItemRect(shelf, itemIndex).ContainsInside(wp)) {
                auto& f = s.clickedAt[itemIndex];
                f       = {};
                f.SetNow();

                if (press)
                  g.run.visuallyLastPressedItem = index;
                break;
              }
            }
          }
        }

        if (g.run.bufferedActions.count < g.run.bufferedActions.maxCount) {
          *g.run.bufferedActions.Add() = {
            .pos     = wp,
            .touchID = ge.meta._latestActiveTouchID,
            .press   = press,
          };
        }
      }
    }

    // Setting player action.
    if (!pl.action) {  ///
      bool actionWasConsumed = false;

      while (g.run.bufferedActions.count) {
        const auto ba = g.run.bufferedActions[0];

        bool skip              = false;
        bool anyItemWasClicked = false;

        int shelf = -1;
        for (auto& s : g.run.shelves) {
          shelf++;

          FOR_RANGE (int, itemIndex, 3) {
            auto& p = s.rows[0][itemIndex];

            const auto r = GetItemRect(shelf, itemIndex);
            if (!r.ContainsInside(ba.pos))
              continue;

            anyItemWasClicked = true;

            const PlayerLastAction currentAction{
              .touchID = ba.touchID, .shelf = shelf, .item = itemIndex
            };

            PlayerAction actionToSet{};

            if (p && pl.item) {
              if (ba.press || (!ba.press && (pl.lastAction != currentAction)))
                actionToSet = PlayerAction_EXCHANGE;
            }
            else if (!pl.item && p) {
              if (ba.press)
                actionToSet = PlayerAction_PICKUP;
            }
            else if (pl.item && !p) {
              if (ba.press || (!ba.press && (pl.lastAction != currentAction)))
                actionToSet = PlayerAction_PUT;
            }

            if (!g.save.level) {
              const auto& m = FIRST_LEVEL_TUTOR_MOVES[g.run.firstLevelTutorMoveIndex];
              if ((m.shelf == shelf) && (m.item == itemIndex)) {
                g.run.firstLevelTutorMoveIndex++;
                g.run.firstLevelTutorPressed = {};
                g.run.firstLevelTutorPressed.SetNow();
              }
              else
                actionToSet = {};
            }

            if (actionToSet) {
              if (s.IsLocked())
                goto playerActionWasSet;
              else {
                actionWasConsumed = true;

                if (BF_ACTIVATE_TOUCH_SPOTS_UPON_CONSUMING) {
                  auto& f = s.clickedAt[itemIndex];
                  f       = {};
                  f.SetNow();
                }

                pl.action = actionToSet;
                pl.actionStartedAt.SetNow();
                pl.actionItemIndex = itemIndex;
                pl.posFrom         = pl.pos;
                pl.pos             = {.shelf = shelf, .itemIndex = itemIndex};
                pl.lastAction      = {
                       .touchID = ba.touchID,
                       .shelf   = shelf,
                       .item    = itemIndex,
                };
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
    int shelf = -1;
    for (auto& s : g.run.shelves) {  ///
      shelf++;
      if (gdebug.matchingParticles) {
        makeMatchingParticles(
          ShelfPos(shelf), lframe::Unscaled(ge.meta.frameVisual % (2 * FIXED_FPS))
        );
      }

      lframe e{};
      if (s.lastMatchedAt.IsSet()) {
        e = s.lastMatchedAt.Elapsed();
        makeMatchingParticles(ShelfPos(shelf), e);
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
        EndGameplay(true);
        Save();
      }

      // Checking if player's lost.
      else if (!SolvableRowOfThreeExists()) {  ///
        const int requiredEmptySpots = (pl.item ? 3 : 2);
        if (CountEmptySpots() < requiredEmptySpots)
          EndGameplay(false);
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
      if (!g.run.levelControlPressedSkip)
        ShowAdInter();
    }
  }

  // Updating clouds perlin.
  {  ///
    PerlinParams perlinParams{
      .octaves    = glib->clouds_perlin_octaves(),
      .smoothness = glib->clouds_perlin_smoothness(),
    };
    if (g.run.perlinParams != perlinParams) {
      g.run.perlinParams = perlinParams;
      LAMBDA (void, normalizedPerlin, (View<u16> container)) {
        CycledPerlin2D(
          VRAND, container, &ge.meta.trashArena, perlinParams, {container.count, 1}
        );
        u16 valueMin = u16_max;
        u16 valueMax = 0;
        for (auto v : container) {
          valueMin = MIN(v, valueMin);
          valueMax = MAX(v, valueMax);
        }
        for (auto& v : container)
          v = (u16)Remap(v, valueMin, valueMax, 0, u16_max);
      };
      FOR_RANGE (int, i, g.run.shelves.count) {
        normalizedPerlin(g.run.perlinX[i].ToView());
        normalizedPerlin(g.run.perlinY[i].ToView());
      }
    }

    const auto perlinDur = lframe::FromSeconds(glib->clouds_perlin_seconds());
    const auto perlinP
      = (f32)(ge.meta.frameVisual % perlinDur.value) / (f32)perlinDur.value;
    g.meta.perlinIndex = ProgressToIndex(perlinP, g.run.perlinX[0].count);
  }

  UpdateCamera();

  DoUI();

  ge.meta.frameVisual++;
}

Color ToColor(const BFGame::Color2* fb) {  ///
  auto c = ColorFromRGBA(fb->color());
  if ((fb->s() != 1) || (fb->v() != 1)) {
    auto hsv = ColorToHSV(c);
    hsv.g *= fb->s();
    hsv.g = MIN(1, hsv.g);
    hsv.b *= fb->v();
    hsv.b  = MIN(1, hsv.b);
    auto a = c.a;
    c      = ColorFromHSV(hsv);
    c.a    = a;
  }
  if (fb->b() != 1)
    c = ColorBrightness(c, fb->b());
  return c;
}

void GameDraw() {
  ZoneScoped;

  // Setup.
  // {  ///
  const auto  localization         = glib->localizations()->Get(ge.meta.localization);
  const auto  localization_strings = localization->strings();
  const auto& pl                   = g.run.player;

  const auto _breathingDur = (int)(2.5f * FIXED_FPS);
  const f32 _breathingP = (f32)(ge.meta.frameVisual % _breathingDur) / (f32)_breathingDur;
  const f32 breathingScale = 1 + 0.1f * sinf(_breathingP * 2 * PI32);

  g.meta.cloudsBreathingP = BreathingP(
    ge.meta.frameVisual, lframe::FromSeconds(glib->clouds_breathing_seconds())
  );
  // }

  f32 audioUnlockP_ = 0;
  if (ge.soundManager.unlocked.IsSet()) {  ///
    if (ge.soundManager.unlocked._value) {
      audioUnlockP_
        = MIN(1, ge.soundManager.unlocked.Elapsed().Progress(ANIMATION_1_FRAMES));
    }
    else
      audioUnlockP_ = 1;
  }
  const auto audioUnlockP = audioUnlockP_;

  // Background.
  {  ///
    DrawGroup_Begin(DrawZ_SCREEN_BACKGROUND);
    DrawGroup_SetSortY(0);

    auto fb_colors = glib->background_colors2();
    if (g.run.backgroundColorIndex >= fb_colors->size())
      g.run.backgroundColorIndex = 0;
    if (g.run.backgroundColorIndex < 0)
      g.run.backgroundColorIndex = fb_colors->size() - 1;

    const auto fb = fb_colors->Get(g.run.backgroundColorIndex);

    DrawTiledBackgroundRects({
      .backgroundColor = ToColor(fb->fill()),
      .rectColor       = ToColor(fb->rect()),
      .rectTexID       = glib->ui_background_rect_texture_id(),
      .cycleDur        = 30 * FIXED_FPS,
    });

    DrawGroup_End();
  }

  BeginMode2D(&g.meta.camera);

  // Drawing debug tiles.
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

  const auto depthItemColor
    = Fade(ColorFromRGBA(glib->item_draw_depth_color()), glib->item_draw_depth_fade());

  const auto lostItemFlashingDur
    = lframe::FromSeconds(glib->lost_items_flashing_seconds());

  LAMBDA (
    void,
    drawItem,
    (Vector2 pos, const Item& item, f32 rotation, Vector2 scale, int depth, f32 fade)
  )
  {  ///
    ASSERT(item);
    if (depth > 1)
      return;

    const auto fb = glib->items()->Get(g.run.colorIndexToItemIndex[item.color]);

    auto tex   = fb->texture_id();
    auto color = WHITE;
    if (depth) {
      tex   = fb->dark_texture_id();
      color = depthItemColor;
      pos += ToVector2(glib->item_draw_depth_offset());
    }

    auto flash = TRANSPARENT_BLACK;

    if (!depth && g.run.gameplayEnded.IsSet() && !g.run.won) {
      auto e = g.run.gameplayEnded.Elapsed();
      e.value -= g.run.lostFrontUniqueColorsStartFlashes[item.color].value;
#if 0
      // Flashing once.
      if ((e.value >= 0) && (e <= lostItemFlashingDur)) {
        auto p = e.Progress(lostItemFlashingDur);
        p *= 2;
        if (p > 1)
          p = 2 - p;
        flash = Fade(PAL_MAROON_FLUSH, EaseInOutQuart(p));
      }
#else
      // Flash holds.
      if (e.value >= 0) {
        auto p = e.Progress(lostItemFlashingDur);
        p      = MIN(1, p);
        flash  = Fade(PAL_MAROON_FLUSH, EaseInOutQuart(p));
      }
#endif
    }

    DrawGroup_Begin(depth ? DrawZ_DEEP_ITEMS : DrawZ_ITEMS);
    DrawGroup_SetSortY(pos.y);

    DrawGroup_CommandTexture({
      .texID    = tex,
      .rotation = rotation,
      .pos      = pos,
      .anchor{0.5f, 0},
      .scale = scale,
      .color = Fade(color, fade),
      .flash = flash,
    });

    DrawGroup_End();
  };

  int        actualLevelIndex = -1;
  const auto fb_level         = GetFBLevel(g.save.level, &actualLevelIndex);

  Vector2 playerPos{};
  Vector2 playerItemPos{};
  // f32 playerRotation = InfinitySymbolRotation(infinityP);
  f32 playerRotation = 0;

  // Calculating.
  {  ///
    const auto infinityDur
      = lframe::FromSeconds(glib->player_infinity_duration_seconds());
    f32 infinityP = 0;
    if (pl.infinityAt.IsSet()) {
      infinityP = (f32)(pl.infinityAt.Elapsed().value % infinityDur.value)
                  / (f32)infinityDur.value;
      if (pl.infinityReverse)
        infinityP *= -1;
    }
    else
      infinityP = (f32)(ge.meta.frameGame % infinityDur.value) / (f32)infinityDur.value;

    const auto playerPosInfinityOffset
      = InfinitySymbol(infinityP) * ToVector2(glib->player_infinity_symbol_offset());

    playerPos = ToVector2(fb_level->player());
    if (g.meta.verticalOrientation)
      playerPos += Vector2(0.5f, -0.5f);
    else
      playerPos += Vector2(-0.5f, 0.5f);

    if (pl.posFrom)
      playerPos = ToWorld(pl.posFrom) + Vector2(0, glib->player_offset_y());

    f32 playerInfinityPosOffsetScale = 1;

    if (pl.pos) {
      const auto pos2 = ToWorld(pl.pos) + Vector2(0, glib->player_offset_y());
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

  FirstLevelTutorMove firstLevelTutorMove
    = FIRST_LEVEL_TUTOR_MOVES[FIRST_LEVEL_TUTOR_MOVES.count - 1];
  if (!g.save.level)
    firstLevelTutorMove = FIRST_LEVEL_TUTOR_MOVES[g.run.firstLevelTutorMoveIndex];

  const flatbuffers::Vector<flatbuffers::Offset<BFGame::Cloud>, flatbuffers::uoffset_t>*
    fb_clouds[3]{glib->clouds_left(), glib->clouds_right(), glib->clouds_center()};

  // Drawing shelves.
  FOR_RANGE (int, mode, 3) {  ///
    // mode 0 - drawing shelves, 1 - drawing items, 2 - drawing 1st level tutor.

    if (!mode) {
      DrawGroup_Begin(DrawZ_CLOUDS);
      DrawGroup_SetSortY(0);
    }

    int shelf = -1;
    for (const auto& s : g.run.shelves) {
      shelf++;

      const auto r = ShelfRect(shelf);

      if (!mode) {
        const auto cloudsScale = Vector2Lerp(
          ToVector2(glib->clouds_breathing_scale_from()),
          ToVector2(glib->clouds_breathing_scale_to()),
          g.meta.cloudsBreathingP
        );

        // Drawing shelf.
        FOR_RANGE (int, i, 3) {
          const auto fb_cloud = fb_clouds[i]->Get(s.cloudPartIndices[i]);
          DrawGroup_CommandTexture({
            .texID  = fb_cloud->texture_id(),
            .pos    = r.pos + Vector2(0, glib->clouds_offset_y()),
            .anchor = ToVector2(fb_cloud->anchor()),
            .scale  = cloudsScale,
            .color
            = ColorLerp(WHITE, CLOUD_COLORS[s.color], glib->clouds_color_lerp_factor()),
          });
        }
        // Drawing spot shadows.
        FOR_RANGE (int, i, 3) {
          auto  color = ColorFromRGBA(glib->item_draw_depth_color());
          f32   scale = 1;
          f32   fade  = glib->item_spot_shadow_fade();
          auto& f     = s.clickedAt[i];
          auto  pos
            = GetItemBottomPos(shelf, i) + Vector2(0, glib->item_spot_shadow_offset_y());
          if (f.IsSet()) {
            const auto e = f.Elapsed();
            const auto dur
              = lframe::FromSeconds(glib->item_spot_shadow_activated_seconds());
            auto p = e.Progress(dur);
            if (e < dur) {
              pos += Vector2(0, glib->item_spot_shadow_activated_offset_y() * p);
              scale *= Lerp(1, glib->item_spot_shadow_activated_scale(), EaseOutQuad(p));
              color = ColorLerp(color, PAL_CASABLANCA, MIN(1, p * 5));
              fade  = Lerp(
                fade, glib->item_spot_shadow_activated_fade(), EaseOutQuad(sinf(p * PI32))
              );
              if (p > 0.4)
                fade *= MAX(0, 1 - (p - 0.3f) / (1 - 0.3f));
            }
            else {
              p = p - 1;
              fade *= MIN(p, 1);
            }
          }
          DrawGroup_CommandTexture({
            .texID = glib->game_item_spot_shadow_texture_id(),
            .pos   = pos,
            .scale = Vector2One() * scale,
            .color = Fade(color, fade),
          });
        }
      }
      else if ((mode == 1) || !g.save.level) {
        for (int depth = s.rows.count - 1; depth >= 0; depth--) {
          Vector2 scale{1, 1};
          if (s.IsLocked() && !depth) {
            const auto dur = lframe::FromSeconds(glib->shelf_matching_duration_seconds());
            const auto p   = s.updatedAt.Elapsed().Progress(dur);
            scale *= Lerp(1, glib->shelf_matching_item_scale(), sinf(p * PI32));
          }

          FOR_RANGE (int, itemIndex, 3) {
            auto& item = s.rows[depth][itemIndex];
            auto  pos  = GetItemBottomPos(shelf, itemIndex, true);

            f32     targetScale = 1;
            Vector2 actionOffset{};
            if (!depth) {
              if ((pl.action == PlayerAction_PICKUP)
                  || (pl.action == PlayerAction_EXCHANGE))
              {
                if ((shelf == pl.pos.shelf) && (itemIndex == pl.actionItemIndex)) {
                  f32 p        = GetPlayerActionWithoutFlyingProgress();
                  actionOffset = Vector2Lerp({}, playerItemPos - pos, EaseInOutQuad(p));
                  targetScale  = Lerp(1, CalculateItemScaleInsidePlayer(item), p);
                }
              }
            }

            if ((mode == 1) && item) {
              // Drawing item.
              drawItem(pos + actionOffset, item, 0, scale * targetScale, depth, 1);
            }
            else if ((mode == 2) && ge.soundManager.unlocked.IsSet()) {
              // Drawing 1st level touch tutor.
              if ((firstLevelTutorMove.shelf == shelf)
                  && (firstLevelTutorMove.item == itemIndex))
              {
                f32 p = audioUnlockP;
                if (g.run.firstLevelTutorPressed.IsSet()) {
                  p *= MIN(
                    1, g.run.firstLevelTutorPressed.Elapsed().Progress(ANIMATION_1_FRAMES)
                  );
                }
                DrawGroup_OneShotTexture(
                  {
                    .texID  = glib->ui_input_touch_down_texture_id(),
                    .pos    = pos + ToVector2(glib->move_tutor_finger_offset()),
                    .anchor = ToVector2(glib->tutor_finger_anchor()),
                    .scale  = Vector2One() * breathingScale,
                    .color  = Fade(WHITE, p),
                  },
                  DrawZ_TUTOR
                );
              }
            }

            if (gdebug.gizmos && !depth && (mode == 1)) {
              const auto r = GetItemRect(shelf, itemIndex);
              DrawGroup_OneShotRectLines({
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

    if (!mode)
      DrawGroup_End();
  }

  // Drawing player.
  {  ///

    auto texs = glib->player_texture_ids();

    f32 fade = 1.0f;
    if (g.run.gameplayEnded.IsSet())
      fade = MAX(0, 1 - g.run.gameplayEnded.Elapsed().Progress(ANIMATION_1_FRAMES));

    FOR_RANGE (int, mode, 3) {
      // 0 - draw item, 1 - draw player glass, 2 - draw player front.

      if (mode == 1) {
        DrawGroup_Begin(DrawZ_PLAYER);
        DrawGroup_SetSortY(0);
      }

      auto color = WHITE;
      if (mode == 1) {
        color = ColorFromRGBA(glib->player_glass_color());
        color.a *= glib->player_glass_fade();
      }

      color.a *= fade;
      color.a *= audioUnlockP;

      if (mode) {
        DrawGroup_CommandTexture({
          .texID    = texs->Get(mode - 1),
          .rotation = playerRotation,
          .pos      = playerPos,
          .color    = color,
        });
      }
      else if (pl.item) {
        f32 targetScale = CalculateItemScaleInsidePlayer(pl.item);

        Vector2 actionOffset{};
        if ((pl.action == PlayerAction_PUT) || (pl.action == PlayerAction_EXCHANGE)) {
          f32  p         = GetPlayerActionWithoutFlyingProgress();
          auto targetPos = GetItemBottomPos(pl.pos.shelf, pl.actionItemIndex, true);
          actionOffset   = Vector2Lerp({}, targetPos - playerItemPos, EaseInOutQuad(p));

          targetScale = Lerp(targetScale, 1, p);
        }

        drawItem(
          playerItemPos + actionOffset,
          pl.item,
          playerRotation,
          Vector2One() * targetScale,
          0,
          fade
        );
      }

      if (mode == 2)
        DrawGroup_End();
    }
  }

  DrawGroup_Begin(DrawZ_PARTICLES);
  DrawGroup_SetSortY(0);
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

  // Dim screen if audio is not unlocked yet.
  if (audioUnlockP < 1) {  ///
    DrawGroup_OneShotRect(
      {
        .pos   = LOGICAL_RESOLUTIONf / 2.0f,
        .size  = ge.meta.scaledLogicalResolution,
        .color = Fade(ge.settings.screenFadeColor, Lerp(0.5f, 0, audioUnlockP)),
      },
      DrawZ_SCREEN_FADE
    );
  }

  // Start button for web (to enable audio).
  if (0 || !ge.soundManager.unlocked.IsSet() && ge.soundManager._works) {  ///
    DrawGroup_Begin(DrawZ_WEB_AUDIO_BUTTON_PROMPT);
    DrawGroup_SetSortY(0);

    const auto text = localization_strings->Get(Loc_UI_WEB_AUDIO_BUTTON_PROMPT__CAPS);

    DrawGroup_CommandText({
      .pos        = LOGICAL_RESOLUTIONf / 2.0f + Vector2(0, glib->tutor_text_offset_y()),
      .scale      = Vector2One() * breathingScale,
      .font       = &g.meta.fontWinDescription,
      .text       = text->c_str(),
      .bytesCount = (int)text->size(),
    });

    DrawGroup_CommandTexture({
      .texID  = glib->ui_input_touch_down_texture_id(),
      .pos    = LOGICAL_RESOLUTIONf / 2.0f + ToVector2(glib->audio_tutor_finger_offset()),
      .anchor = ToVector2(glib->tutor_finger_anchor()),
      .scale  = Vector2One() * breathingScale,
    });

    DrawGroup_End();
  }

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
        IM::Checkbox("Draw Win", &gdebug.drawWin);
        IM::Checkbox("Draw Lost", &gdebug.drawLost);

        if (!g.run.gameplayEnded.IsSet()) {
          if (IM::Button("Win"))
            EndGameplay(true);
          if (IM::Button("Lose"))
            EndGameplay(false);
        }

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
        if (IM::Button("Background color--"))
          g.run.backgroundColorIndex--;
        if (IM::Button("Background color++"))
          g.run.backgroundColorIndex++;

        IM::Text("Actual level index %d", actualLevelIndex);

        // if (IM::Button(
        //       TextFormat("Level Seed (%d) ++",
        //       (int)g.run.randomSeedForLevelHotReload)
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
  PAL_ALIZARIN_CRIMSON,
  PAL_CASABLANCA,
  PAL_DOLLY,
  PAL_CASABLANCA,
  PAL_MAUVE,
};

void ParticleRender_COMMON(f32 p, lframe e, ParticleRenderData data) {  ///
  auto color    = particleColors[ProgressToIndex(p, ARRAY_COUNT(particleColors))];
  data.color->r = color.r;
  data.color->g = color.g;
  data.color->b = color.b;
}

///
