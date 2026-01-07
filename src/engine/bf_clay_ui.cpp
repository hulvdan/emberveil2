#if !defined(BF_UI_PRE) && !defined(BF_UI_POST) \
  || defined(BF_UI_PRE) && defined(BF_UI_POST)
#  error "You must define either BF_UI_PRE or BF_UI_POST!"
#endif

// !banner: PRE
// ██████╗ ██████╗ ███████╗
// ██╔══██╗██╔══██╗██╔════╝
// ██████╔╝██████╔╝█████╗
// ██╔═══╝ ██╔══██╗██╔══╝
// ██║     ██║  ██║███████╗
// ╚═╝     ╚═╝  ╚═╝╚══════╝

#ifdef BF_UI_PRE
if ((ge.meta.screenSize.x <= 0) || (ge.meta.screenSize.y <= 0))
  return;

ZoneScoped;

bool blockInput = false;

#  define SCOPED_CONTEXT(context_)                   \
    const auto _prevContext = currentContext;        \
    currentContext          = (context_);            \
    if (!draw) {                                     \
      controlsContexts[(context_)].thisFrame = true; \
      if (!controlsContexts[(context_)].prevFrame)   \
        controlsContexts[(context_)].focused = {};   \
    }                                                \
    DEFER {                                          \
      currentContext = _prevContext;                 \
    };

#  define SCOPED_CONTEXT_IF(context_, enabled_)              \
    const auto _prevContext = currentContext;                \
    if (!draw) {                                             \
      if (enabled_) {                                        \
        currentContext                         = (context_); \
        controlsContexts[(context_)].thisFrame = true;       \
      }                                                      \
      controlsContexts[(context_)].disabled = !(enabled_);   \
      if (!controlsContexts[(context_)].prevFrame)           \
        controlsContexts[(context_)].focused = {};           \
    }                                                        \
    DEFER {                                                  \
      currentContext = _prevContext;                         \
    };

#  define CURRENT_CONTEXT (controlsContexts[currentContext])

enum UIZIndexOffset {  ///
  UIZIndexOffset_HOVER_DETAILS              = 2,
  UIZIndexOffset_STATS                      = 4,
  UIZIndexOffset_CONFIRM_MODAL              = 6,
  UIZIndexOffset_JUST_UNLOCKED_ACHIEVEMENTS = 8,
};

// Setup.
// {  ///
TEMP_USAGE(&ge.meta.trashArena);
TEMP_USAGE(&ge.meta._transientDataArena);

const auto draw = ge.meta._drawing;

static ControlsContext lastFrameActiveControlsContext{};
ControlsContext        currentContext{};

static struct {
  bool           thisFrame = false;
  bool           prevFrame = false;
  Clay_ElementId focused   = {};
  bool           disabled  = false;
} controlsContexts[ControlsContext_COUNT];

static Direction uiElementSwitchDirectionPrevFrame = Direction_NONE;

Direction uiElementSwitchDirection = Direction_NONE;
if (!draw) {
  if (IsKeyPressed(SDL_SCANCODE_D) || IsKeyPressed(SDL_SCANCODE_RIGHT))
    uiElementSwitchDirection = Direction_RIGHT;
  if (IsKeyPressed(SDL_SCANCODE_W) || IsKeyPressed(SDL_SCANCODE_UP))
    uiElementSwitchDirection = Direction_UP;
  if (IsKeyPressed(SDL_SCANCODE_A) || IsKeyPressed(SDL_SCANCODE_LEFT))
    uiElementSwitchDirection = Direction_LEFT;
  if (IsKeyPressed(SDL_SCANCODE_S) || IsKeyPressed(SDL_SCANCODE_DOWN))
    uiElementSwitchDirection = Direction_DOWN;
}

DEFER {
  if (!draw)
    uiElementSwitchDirectionPrevFrame = uiElementSwitchDirection;
};

bool justFocusedDefaultControl = false;

LAMBDA (void, markControlAsDefault, (Clay_ElementId id)) {
  if (draw)
    return;

  const bool allowed = (CURRENT_CONTEXT.thisFrame || CURRENT_CONTEXT.disabled);
  ASSERT(allowed);

  if ((uiElementSwitchDirection || g.meta.playerUsesKeyboardOrController)
      && !CURRENT_CONTEXT.focused.id  //
      && !CURRENT_CONTEXT.disabled)
  {
    CURRENT_CONTEXT.focused   = id;
    justFocusedDefaultControl = true;
  }

  if (!CURRENT_CONTEXT.prevFrame                //
      && g.meta.playerUsesKeyboardOrController  //
      && !CURRENT_CONTEXT.focused.id            //
      && !CURRENT_CONTEXT.disabled)
    CURRENT_CONTEXT.focused = id;
};

const auto fb_atlas_textures      = glib->atlas_textures();
const auto original_texture_sizes = glib->original_texture_sizes();

static int _disallowTouchNumber = {};

LAMBDA (void, disallowTouch, ()) {
  ASSERT_FALSE(draw);
  if (GetTouchIDs().count) {
    ASSERT(ge.meta._latestActiveTouchID != InvalidTouchID);
    _disallowTouchNumber = GetTouchData(ge.meta._latestActiveTouchID).number;
  }
};

// Clay. Set UI dimensions + update mouse pos.
if (!draw) {
  {
    auto res = ge.meta.scaledLogicalResolution;
    auto r   = res.x / res.y;
    if (r > UI_ASPECT_RATIO_MAX)
      res.x = res.y * UI_ASPECT_RATIO_MAX;
    else if (r < UI_ASPECT_RATIO_MIN)
      res.y = res.x / UI_ASPECT_RATIO_MIN;
    g.meta.screenSizeUI = res;
    Clay_SetLayoutDimensions({res.x, res.y});
  }

  auto pos = ScreenPosToLogical(GetMouseScreenPos());

  if (ge.events.last == LastEventType_TOUCH) {
    pos = Vector2Inf();

    if (ge.meta._latestActiveTouchID != InvalidTouchID) {
      const auto td = GetTouchData(ge.meta._latestActiveTouchID);
      if (_disallowTouchNumber != td.number)
        pos = ScreenPosToLogical(td.screenPos);
    }
  }

  g.meta.screenSizeUIMargin = {
    (g.meta.screenSizeUI.x - LOGICAL_RESOLUTION.x) / 2.0f,
    (g.meta.screenSizeUI.y - LOGICAL_RESOLUTION.y) / 2.0f,
  };

  pos = Vector2(pos.x, LOGICAL_RESOLUTION.y - pos.y) + g.meta.screenSizeUIMargin;
  Clay_SetPointerState({pos.x, pos.y}, false);
}

const auto localization              = glib->localizations()->Get(ge.meta.localization);
const auto localization_strings      = localization->strings();
const auto localization_broken_lines = localization->broken_lines();

Clay_BeginLayout();

auto& beautifiers      = ge._ui.clayBeautifiers;
auto& beautifiersCount = ge._ui.clayBeautifiersCount;

int _wheel = 0;
if (!draw) {
  _wheel = GetMouseWheel();
  if (IsKeyDown(SDL_SCANCODE_LSHIFT) || IsKeyDown(SDL_SCANCODE_RSHIFT))
    _wheel *= 10;
}
const int wheel = _wheel;

auto& zIndex = ge._ui.clayZIndex;
// }

LAMBDA (void, BF_CLAY_TEXT_LOCALIZED, (int locale, ClayTextOptions opts = {})) {  ///
  ASSERT((int)locale >= 0);
  ASSERT((int)locale < Loc_COUNT);
  auto        string = localization_strings->Get(locale);
  Clay_String text{
    .isStaticallyAllocated = true,
    .length                = (i32)string->size(),
    .chars                 = string->c_str(),
  };
  BF_CLAY_TEXT(text, opts);
};

LAMBDA (void, componentOverlay, (auto innerLambda, f32 fade = 1)) {  ///
  CLAY({
    .layout{
      .sizing{
        .width  = CLAY_SIZING_FIXED(LOGICAL_RESOLUTION.x * 2),
        .height = CLAY_SIZING_FIXED(LOGICAL_RESOLUTION.y * 2),
      },
    },
    .floating{
      .zIndex = (i16)(zIndex - 1),
      .attachPoints{
        .element = CLAY_ATTACH_POINT_CENTER_CENTER,
        .parent  = CLAY_ATTACH_POINT_CENTER_CENTER,
      },
      .attachTo = CLAY_ATTACH_TO_PARENT,
    },
    BF_CLAY_CUSTOM_BEGIN{
      BF_CLAY_CUSTOM_OVERLAY(Fade(MODAL_OVERLAY_COLOR, MODAL_OVERLAY_COLOR_FADE * fade)),
    } BF_CLAY_CUSTOM_END,
  }) {
    innerLambda();
  }
};

bool _alreadyHandledClickOrTouch = blockInput;

LAMBDA (bool, touchedThisComponent, (bool preventFutureDispatch = true)) {  ///
  if (draw)
    return false;
  if (_alreadyHandledClickOrTouch)
    return false;
  const bool result = !g.meta.playerUsesKeyboardOrController  //
                      && Clay_Hovered()                       //
                      && IsTouchPressed(ge.meta._latestActiveTouchID);
  if (result && preventFutureDispatch)
    _alreadyHandledClickOrTouch = true;
  return result;
};

LAMBDA (bool, clickedOrTouchedThisComponent, (bool preventFutureDispatch = true)) {  ///
  if (draw)
    return false;
  if (_alreadyHandledClickOrTouch)
    return false;
  const bool result
    = !g.meta.playerUsesKeyboardOrController  //
      && Clay_Hovered()                       //
      && (IsMousePressed(L) || IsTouchPressed(ge.meta._latestActiveTouchID));
  if (result && preventFutureDispatch)
    _alreadyHandledClickOrTouch = true;
  return result;
};

bool _alreadyHandledActivation = false;

LAMBDA (bool, isCurrentContextActive, ()) {  ///
  return lastFrameActiveControlsContext == currentContext;
};

LAMBDA (bool, IsKeyPressedInCurrentContext, (SDL_Scancode key)) {  ///
  if (!isCurrentContextActive())
    return false;
  return IsKeyPressed(key);
};

LAMBDA (bool, didActivate, (SDL_Scancode key)) {  ///
  if (draw)
    return false;
  if (_alreadyHandledActivation)
    return false;
  bool result = IsKeyPressedInCurrentContext(key);
  if (result)
    _alreadyHandledActivation = true;
  return result;
};

#  undef BF_UI_PRE
#endif

// !banner: POST
// ██████╗  ██████╗ ███████╗████████╗
// ██╔══██╗██╔═══██╗██╔════╝╚══██╔══╝
// ██████╔╝██║   ██║███████╗   ██║
// ██╔═══╝ ██║   ██║╚════██║   ██║
// ██║     ╚██████╔╝███████║   ██║
// ╚═╝      ╚═════╝ ╚══════╝   ╚═╝

#ifdef BF_UI_POST
ASSERT_FALSE(currentContext);

// Control groups navigation.
if (!draw) {  ///
  ControlsContext currentContext = {};
  for (int i = 1; i < ControlsContext_COUNT; i++) {
    auto& context = controlsContexts[i];

    if (context.prevFrame && !context.thisFrame)
      context.focused = {};

    if (!context.disabled) {
      auto c = (ControlsContext)i;
      if (context.thisFrame && (currentContext < c))
        currentContext = c;
    }

    context.prevFrame = context.thisFrame;
    context.thisFrame = false;
  }

  lastFrameActiveControlsContext = currentContext;

  if (uiElementSwitchDirection && currentContext && !justFocusedDefaultControl) {
    auto toSelect = controlsContexts[currentContext].focused;

    auto group = ge._ui.controlsGroupsFirst;
    while (group) {
      ASSERT(group->next != group);
      ASSERT(group->prev != group);

      auto dim = group->first;

      int preferredDimIndex = -1;
      while (dim) {
        preferredDimIndex++;

        ASSERT(dim->next != dim);
        ASSERT(dim->prev != dim);

        auto elem = dim->first;

        int preferredElemIndex = -1;
        while (elem) {
          preferredElemIndex++;

          ASSERT(elem->next != elem);
          ASSERT(elem->prev != elem);

          if (elem->id.id == controlsContexts[currentContext].focused.id) {
            if (uiElementSwitchDirection == Direction_RIGHT) {
              if (elem->next)
                toSelect = elem->next->id;
              else {
                auto to = group->connectionsPerDirection[(int)Direction_RIGHT - 1];
                if (to) {
                  auto gto = _GetControlsGroup(to);
                  toSelect = gto->first->first->id;
                }
              }
            }
            else if (uiElementSwitchDirection == Direction_LEFT) {
              if (elem->prev)
                toSelect = elem->prev->id;
              else {
                auto to = group->connectionsPerDirection[(int)Direction_LEFT - 1];
                if (to) {
                  auto gto = _GetControlsGroup(to);
                  toSelect = gto->first->last->id;
                }
              }
            }
            else if (uiElementSwitchDirection == Direction_DOWN) {
              if (dim->next && dim->next->first) {
                toSelect
                  = _GetPreferredControlsEntry(dim->next->first, preferredElemIndex)->id;
              }
              else {
                auto to = group->connectionsPerDirection[(int)Direction_DOWN - 1];
                if (to) {
                  auto gto = _GetControlsGroup(to);
                  toSelect = gto->first->first->id;
                }
              }
            }
            else if (uiElementSwitchDirection == Direction_UP) {
              if (dim->prev) {
                toSelect
                  = _GetPreferredControlsEntry(dim->prev->first, preferredElemIndex)->id;
              }
              else {
                auto to = group->connectionsPerDirection[(int)Direction_UP - 1];
                if (to) {
                  auto gto = _GetControlsGroup(to);
                  toSelect = gto->last->first->id;
                }
              }
            }
            else
              INVALID_PATH;
          }

          elem = elem->next;
        }
        dim = dim->next;
      }
      group = group->next;
    }

    if (!draw && (controlsContexts[currentContext].focused.id != toSelect.id)) {
      controlsContexts[currentContext].focused = toSelect;
      PlaySound(Sound_UI_HOVER_SMALL);
    }
  }

  ge._ui.controlsGroupsFirst = nullptr;
  ge._ui.controlsGroupsLast  = nullptr;
}

// Game's version.
if (!gdebug.hideUIForVideo) {  ///
  bool showVersion = ge.meta.debugEnabled;
#  if BF_SHOW_VERSION
  showVersion = true;
#  endif

  CLAY({
    .floating{
      .offset{UI_PADDING_OUTER_HORIZONTAL, -UI_PADDING_OUTER_VERTICAL},
      .zIndex = i16_max,
      .attachPoints{
        .element = CLAY_ATTACH_POINT_LEFT_BOTTOM,
        .parent  = CLAY_ATTACH_POINT_LEFT_BOTTOM,
      },
      .attachTo = CLAY_ATTACH_TO_PARENT,
    },
  }) {
    FLOATING_BEAUTIFY;

    if (showVersion)
      BF_CLAY_TEXT(g_gameVersion);

    if (IsEmulatingMobile())
      BF_CLAY_TEXT("[EMULATING MOBILE]");

    if (BUILD_WARNINGS)
      BF_CLAY_TEXT(TextFormat("[WARNINGS: %d]", BUILD_WARNINGS));
  }
}

ASSERT(ge._ui.clayZIndex == 0);
ASSERT_FALSE(ge._ui.overriddenFont);
auto drawCommands = Clay_EndLayout();

// Drawing UI.
if (draw) {  ///
  ZoneScopedN("Drawing UI");

  auto& beautifiers      = ge._ui.clayBeautifiers;
  auto& beautifiersCount = ge._ui.clayBeautifiersCount;
  ASSERT(beautifiersCount == 0);

  FOR_RANGE (int, mode, 2) {
    // 0 - drawing ui.
    // 1 - drawing gizmos.

    DrawGroup_Begin(mode ? DrawZ_GIZMOS : DrawZ_UI);
    DrawGroup_SetSortY(0);

    FOR_RANGE (int, i, drawCommands.length) {
      auto& cmd = drawCommands.internalArray[i];

      Beautify beautify{};
      FOR_RANGE (int, k, beautifiersCount) {
        auto& b = beautifiers[k];
        beautify.alpha *= b.alpha;
        beautify.translate += b.translate;
        beautify.scale *= b.scale;
      }
      ASSERT(beautify.alpha <= 1.0f);

      cmd.boundingBox.x -= (g.meta.screenSizeUI.x - LOGICAL_RESOLUTION.x) / 2.0f;
      cmd.boundingBox.y -= (g.meta.screenSizeUI.y - LOGICAL_RESOLUTION.y) / 2.0f;

      cmd.boundingBox.x += beautify.translate.x;
      cmd.boundingBox.y += beautify.translate.y;

      auto bb = cmd.boundingBox;
      bb.y    = LOGICAL_RESOLUTION.y - bb.y - bb.height;

      if (!mode) {
        switch (cmd.commandType) {
        case CLAY_RENDER_COMMAND_TYPE_NONE:
          break;

        case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
          const auto& d = cmd.renderData.rectangle;
          DrawGroup_CommandRect({
            .pos{bb.x, bb.y},
            .size{bb.width, bb.height},
            .anchor{},
            .color{
              (u8)d.backgroundColor.r,
              (u8)d.backgroundColor.g,
              (u8)d.backgroundColor.b,
              (u8)((f32)d.backgroundColor.a * beautify.alpha)
            },
          });
        } break;

        case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
          const auto& d    = cmd.renderData.image;
          const auto& data = *(ClayImageData*)d.imageData;
          DrawGroup_CommandTexture({
            .texID = data.texID,
            .pos{
              bb.x + bb.width / 2 + data.offset.x,
              bb.y + bb.height / 2 + data.offset.y,
            },
            .anchor        = data.anchor,
            .scale         = data.scale,
            .sourceMargins = data.sourceMargins,
            .color{
              data.color.r,
              data.color.g,
              data.color.b,
              (u8)((f32)data.color.a * beautify.alpha)
            },
            .flash = data.flash,
          });
        } break;

        case CLAY_RENDER_COMMAND_TYPE_TEXT: {
          // TODO: letterSpacing
          // TODO: lineHeight
          auto& d    = cmd.renderData.text;
          auto  font = ge._ui.baseFont + d.fontId;

          Color c{
            (u8)d.textColor.r,
            (u8)d.textColor.g,
            (u8)d.textColor.b,
            (u8)((f32)d.textColor.a * beautify.alpha),
          };
          DrawGroup_CommandText({
            .pos{bb.x, bb.y},
            .anchor{},
            .font       = font,
            .text       = d.stringContents.chars,
            .bytesCount = d.stringContents.length,
            .color      = c,
          });
        } break;

        case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
          const auto& d    = cmd.renderData.custom;
          const auto& data = *(ClayCustomData*)d.customData;

          if (data.beautifierStart.set) {
            ASSERT_FALSE(data.beautifierEnd.set);
            const auto& d                   = data.beautifierStart;
            beautifiers[beautifiersCount++] = {
              .alpha     = d.alpha,
              .translate = d.translate,
              .scale     = d.scale,
            };
          }

          if (data.beautifierEnd.set) {
            ASSERT_FALSE(data.beautifierStart.set);
            ASSERT(beautifiersCount > 0);
            beautifiersCount--;
          }

          if (data.overlay.set) {
            const auto& d = data.overlay;

            DrawGroup_CommandRect({
              .pos   = LOGICAL_RESOLUTIONf / 2.0f,
              .size  = ge.meta.scaledLogicalResolution,
              .color = d.color,
            });
          }

#  ifdef BF_ENGINE_EXTEND_CLAY_CUSTOM_DATA
#    define X(_, fieldName)   \
      if (data.fieldName.set) \
        ClayRender_##fieldName(bb, beautify);
          BF_ENGINE_EXTEND_CLAY_CUSTOM_DATA
#    undef X
#  endif

          if (data.shadow.set) {
            const auto& d = data.shadow;

            const auto downscaleFactor = (f32)glib->atlas_downscale_factor();
            const auto fb              = d.nineSlice;

            DrawGroup_CommandTextureNineSlice({
              .texID = fb->texture_id(),
              .pos{
                bb.x - (f32)fb->outer_left() / downscaleFactor,
                bb.y - (f32)fb->outer_bottom() / downscaleFactor,
              },
              .anchor{},
              .color = Fade(WHITE, beautify.alpha * 3.0f / 5.0f),
              .nineSliceMargins{
                (f32)fb->left() / downscaleFactor,
                (f32)fb->right() / downscaleFactor,
                (f32)fb->top() / downscaleFactor,
                (f32)fb->bottom() / downscaleFactor,
              },
              .nineSliceSize{
                (f32)bb.width
                  + (f32)(fb->outer_left() + fb->outer_right()) / downscaleFactor,
                (f32)bb.height
                  + (f32)(fb->outer_top() + fb->outer_bottom()) / downscaleFactor,
              },
            });
          }

          if (data.nineSlice.set) {
            const auto& d = data.nineSlice;

            const auto downscaleFactor = (f32)glib->atlas_downscale_factor();
            const auto fb              = d.nineSlice;

            Color color{d.nineSliceColor.r, d.nineSliceColor.g, d.nineSliceColor.b, 255};
            color.a = (u8)((f32)d.nineSliceColor.a * beautify.alpha);

            DrawGroup_CommandTextureNineSlice({
              .texID = fb->texture_id(),
              .pos{bb.x, bb.y},
              .anchor{},
              .color = color,
              .flash = d.nineSliceFlash,
              .nineSliceMargins{
                (f32)fb->left() / downscaleFactor,
                (f32)fb->right() / downscaleFactor,
                (f32)fb->top() / downscaleFactor,
                (f32)fb->bottom() / downscaleFactor,
              },
              .nineSliceSize{(f32)bb.width, (f32)bb.height},
            });
          }
        } break;
        }
      }
      else {
        // UI elements' gizmos.
        if (gdebug.gizmos && bb.width && bb.height) {
          DrawGroup_CommandRectLines({
            .pos{bb.x, bb.y},
            .size{bb.width, bb.height},
            .anchor{0, 0},
            .color = GREEN,
          });
        }
      }
    }

    DrawGroup_End();
  }
}

#endif

///
