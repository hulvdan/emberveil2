"""
USAGE:

    from bf_lib import game_settings, gamelib_processor

    game_settings.itch_target = "hulvdan/cult-boy"
    game_settings.languages = ["russian", "english"]
    game_settings.generate_flatbuffers_api_for = ["bf_save.fbs"]
    game_settings.yandex_metrica_counter_id = 1

    @gamelib_processor
    def process_gamelib(_genline, gamelib, _localization_codepoints: set[int]) -> None:
        for tile in gamelib["tiles"]:
            t = tile["type"]
"""

# Imports.  {  ###
from collections import Counter
from itertools import groupby

import bf_lib as bf
import spacy
from bf_typer import command, timing
from PIL import Image, ImageDraw, ImageEnhance, ImageFont

# }

bf.game_settings.itch_target = "hulvdan/emberveil2"
bf.game_settings.languages = ["russian", "english"]
bf.game_settings.generate_flatbuffers_api_for = ["bf_save.fbs"]
bf.game_settings.yandex_metrica_counter_id = 106388631
bf.game_settings.colors = [  ###
    "#ffffff",
    "#fb6b1d",
    "#e83b3b",
    "#831c5d",
    "#c32454",
    "#f04f78",
    "#f68181",
    "#fca790",
    "#e3c896",
    "#ab947a",
    "#966c6c",
    "#625565",
    "#3e3546",
    "#0b5e65",
    "#0b8a8f",
    "#1ebc73",
    "#91db69",
    "#fbff86",
    "#fbb954",
    "#cd683d",
    "#9e4539",
    "#7a3045",
    "#6b3e75",
    "#905ea9",
    "#a884f3",
    "#eaaded",
    "#8fd3ff",
    "#4d9be6",
    "#4d65b4",
    "#484a77",
    "#30e1b9",
    "#8ff8e2",
    "#000000",
]


scoped_processing_args = ["None", "None"]


@bf.gamelib_processor
def process_gamelib(*args, **kwargs) -> None:
    # {  ###
    try:
        _process_gamelib(*args, **kwargs)
    except Exception:
        print("ERROR HAPPENED DURING PROCESSING:", ", ".join(scoped_processing_args))
        raise
    # }


def _process_gamelib(
    genline, gamelib, localization_codepoints: set[int], _warning
) -> None:
    def enumerate_table(field: str):
        # {  ###
        scoped_processing_args[0] = field

        for i, x in enumerate(gamelib[field]):
            scoped_processing_args[1] = x["type"]
            yield i, x

        scoped_processing_args[0] = "None"
        scoped_processing_args[1] = "None"
        # }

    transforms: list[tuple[str, str, str, dict[str, int]]] = []

    # Items.
    # ============================================================
    # {  ###
    items = []
    for f in list((bf.ART_TEXTURES_DIR / "items").glob("*.png")):
        items.append(
            {
                "texture_id": f.stem,
                "dark_texture_id": f.stem + "_dark",
                # "outline_texture_id": f.stem + "_outline",
            }
        )
    gamelib["items"] = items
    # }

    genline(f"constexpr int TOTAL_ITEMS = {len(gamelib['items'])};\n")

    # Levels.
    # ============================================================
    # {  ###
    if 1:
        d = bf.ldtk_load(bf.ASSETS_DIR / "level_ufo.ldtk")
        levels = []

        cycleable_levels_indices = []

        s = None
        sx = None
        sy = None

        for level_index, level in enumerate(d.levels):
            entities = level.get_layer("Entities")
            tiles = level.get_layer("Tiles")
            if s is None:
                sx = entities.cWid_
                sy = entities.cHei_
                s = (sx, sy)
            else:
                assert s == (entities.cWid_, entities.cHei_), (
                    "All levels must have the same size!"
                )

            assert s is not None
            assert sx is not None
            assert sy is not None

            manually_placed_items_ = []

            shelves = []
            added_tile_shelves = False
            for i, t in enumerate(tiles.intGridCsv):
                if not t:
                    continue
                if t == 1:
                    added_tile_shelves = True
                    shelves.append({"pos": (i % sx, sy - i // sx - 1)})
                else:
                    assert False

            for entity in entities.entityInstances:
                if entity.identifier_ == "Zone":
                    assert not added_tile_shelves
                    shelf = {
                        "pos": (entity.grid_[0], sy - entity.grid_[1] - 1),
                    }
                    for i in range(3):
                        item = entity.field(f"item{i + 1}")
                        shelf[f"manual_item{i + 1}"] = item
                        if item:
                            manually_placed_items_.append(item)
                    shelves.append(shelf)
            shelves.sort(key=lambda x: (x["pos"][1], x["pos"][0]))  # type: ignore
            player = bf.ldtk_get_single_entity(entities, "Player")

            manually_placed_items = Counter(manually_placed_items_)
            for v in manually_placed_items.values():
                assert not (v % 3), (
                    f"Level (index={level_index}) must have 3 of each hand placed items!\n"
                    f"{manually_placed_items=}"
                )

            total_item_rows = level.field("OverrideTotalItemRows")
            if not total_item_rows:
                total_item_rows = round(
                    len(shelves) * gamelib["default_item_rows_per_shelf"]
                )

            assert total_item_rows <= len(gamelib["items"]), (
                f"Level index={level_index} requires {total_item_rows} rows. "
                f"{len(gamelib['items'])} items are set in gamelib"
            )

            levels.append(
                {
                    "player": (player.grid_[0] + 1, sy - player.grid_[1] - 1),
                    "shelves": shelves,
                    "override_total_item_rows": total_item_rows,
                    "override_empty_item_rows": level.field("OverrideEmptyItemRows"),
                    "random_seed": level.field("RandomSeed"),
                    "manually_placed_rows": sum(manually_placed_items.values()) // 3,
                }
            )
            if level.field("Cycle"):
                cycleable_levels_indices.append(level_index)

        gamelib["world_size"] = s
        gamelib["levels"] = levels
        gamelib["cycleable_levels_indices"] = cycleable_levels_indices
    # }

    # Particles.
    # ============================================================
    # {  ###
    genline(
        "using ParticleRenderFunction_t = void(*)(f32 p, lframe e, ParticleRenderData data);\n"
    )

    particle_render_function_names = set()
    for _i, particle in enumerate_table("particles"):
        if n := particle.get("render_function_name"):
            particle_render_function_names.add(n)

    for n in particle_render_function_names:
        genline(f"void ParticleRender_{n}(f32 p, lframe e, ParticleRenderData data);")

    genline("")

    genline("constexpr ParticleRenderFunction_t particleRenderFunctions[]{")
    for _i, particle in enumerate_table("particles"):
        if n := particle.pop("render_function_name", None):
            genline(f"  ParticleRender_{n},")
        else:
            genline("  nullptr,")
    genline("};\n")
    # }

    # Placeholders.
    # ============================================================
    if 1:  # {  ###
        genline("// Placeholders. {  ///")
        genline("struct Placeholder;")
        params = "const Placeholder* placeholder"
        genline("using ClayPlaceholderFunction = void(*)({});".format(params))
        for i, x in enumerate(gamelib["placeholders"]):
            if i > 0:
                genline("void ClayPlaceholderFunction_{}({});".format(x["type"], params))
        genline("ClayPlaceholderFunction clayPlaceholderFunctions_[]{")
        for i, x in enumerate(gamelib["placeholders"]):
            if i > 0:
                genline("  ClayPlaceholderFunction_{},".format(x["type"]))
        genline("};")
        genline("VIEW_FROM_ARRAY_DANGER(clayPlaceholderFunctions);")
        genline("// }")
        genline("")
    # }

    with open(bf.SRC_DIR / "game" / "bf_gamelib.fbs", encoding="utf-8") as in_file:
        gamelib_fbs_lines = [l.strip() for l in in_file if l.strip()]

    # Texture bind.
    # ============================================================
    if 1:  # {  ###
        start = -1
        end = -1
        for i, line in enumerate(gamelib_fbs_lines):
            if "AUTOMATIC_TEXTURE_BIND_START" in line:
                assert start == -1
                start = i + 1
            if "AUTOMATIC_TEXTURE_BIND_END" in line:
                assert end == -1
                end = i
                break
        assert start >= 0
        assert end > start

        for i in range(start, end):
            field = gamelib_fbs_lines[i].split(":", 1)[0]
            gamelib[field] = field.split("_texture_id", 1)[0]
    # }

    # Tables.
    # ============================================================
    if 1:  # {  ###
        start = -1
        end = -1
        for i, line in enumerate(gamelib_fbs_lines):
            if "TABLE_GEN_START" in line:
                start = i + 1
            if "TABLE_GEN_END" in line:
                end = i
                break
        assert start >= 0
        assert end > start

        # tuples of field_name + type_name
        # ex: [("hostilities", "Hostility")]
        table_transform_data: list[tuple[str, str]] = []
        for i in range(start, end):
            line = gamelib_fbs_lines[i]  # ex: `    hostilities: [Hostility] (required);`
            field_name = line.split(":", 1)[0].strip()  # ex: `hostilities`
            type_name = line.split("[", 1)[-1].split("]", 1)[0].strip()  # ex: `Hostility`
            table_transform_data.append((field_name, type_name))

        for field_name, type_name in table_transform_data:
            types = [x["type"] for x in gamelib[field_name]]
            for t in types:
                assert t.upper() == t, (
                    "{}. {}. `type` fields must be in CONSTANT_CASE".format(field_name, t)
                )
            bf.check_duplicates(types)
            bf.genenum(genline, type_name + "Type", types, add_count=True)
            transforms.append(
                (
                    field_name,
                    f"{type_name.lower()}_type",
                    f"{type_name.lower()}_types",
                    {v: i for i, v in enumerate(types)},
                )
            )
    # }

    # Codepoints.
    # ============================================================
    if 1:  # {  ###
        ranges = [
            (ord(" "), ord(" ") + 1),  # Space character
            # (ord("−"), ord("−") + 1),  # Long minus character
            (33, 127),  # ASCII
            (1040, 1104),  # Cyrillic
        ]

        for r in ranges:
            localization_codepoints.update(set(range(r[0], r[1])))

        codepoints_with_groups: list[tuple[int, int]] = []
        for c in sorted(localization_codepoints):
            codepoints_with_groups.append((c // 10, c))

        genline("int g_codepoints[] {  ///")
        for _, group in groupby(codepoints_with_groups, lambda x: x[0]):
            g = list(group)
            genline(
                "  {},  // {}.".format(
                    ", ".join(str(i[1]) for i in g),
                    ", ".join(chr(i[1]) for i in g),
                )
            )
        genline("};\n")
    # }

    # Transforms.
    # ============================================================
    # {  ###
    for v in transforms:
        bf.recursive_replace_transform(gamelib, *(v[1:]))
    # }


@command
@timing
def process_images():
    # {  ###
    for f in list(bf.ART_TEXTURES_DIR.rglob("_*.png")):
        f.unlink()

    OUTLINE_WIDTH = 10

    bf.im_conveyor(
        "volume_bands",
        "volume_bands",
        bf.imc_prefix(""),
        bf.imc_scale(0.2127),
        bf.imc_outline(radius=1, color=(0, 0, 0, 0)),
        bf.imc_outline(radius=8),
        out_dir=bf.ART_TEXTURES_DIR,
    )

    bf.im_conveyor(
        "icons_audio",
        "icons_audio",
        bf.imc_prefix(""),
        bf.imc_outline(radius=1, color=(0, 0, 0, 0)),
        bf.imc_outline(radius=8),
        # bf.imc_scale(0.5),
        out_dir=bf.ART_TEXTURES_DIR,
    )

    # _ui_background_rect
    bf.im_rectangle((412, 582), radius=30).save(
        bf.ART_TEXTURES_DIR / "_ui_background_rect.png"
    )

    # _game_item_spot_shadow
    bf.im_ellipse((120, 120)).save(bf.ART_TEXTURES_DIR / "_game_item_spot_shadow.png")

    # _ui_button
    bf.im_outline(
        bf.im_remap(
            Image.open(bf.ART_TEXTURES_DIR / "other" / "ui_button.png"),
            bf.palette_color_tuple3("ORANGE"),
            bf.palette_color_tuple3("CASABLANCA"),
        ),
        radius=OUTLINE_WIDTH,
    ).save(bf.ART_TEXTURES_DIR / "_ui_button.png")

    image_star = Image.open(bf.ART_TEXTURES_DIR / "other" / "ui_star.png")

    # _ui_star_gold, _ui_star_gray, _ui_star_gold_small, _ui_star_gray_small
    for suf, c1, c2 in (
        ("gold", "ORANGE", "CASABLANCA"),
        ("gray", "SCORPION", "SCORPION"),
    ):
        image_star_processed = bf.im_outline(
            bf.im_remap(
                image_star, bf.palette_color_tuple3(c1), bf.palette_color_tuple3(c2)
            ),
            radius=int(OUTLINE_WIDTH * 1.4),
        )
        image_star_processed.save(bf.ART_TEXTURES_DIR / f"_ui_star_{suf}.png")

        small_star_scale = 0.1
        bf.im_outline(bf.im_scale(image_star_processed, small_star_scale), radius=2).save(
            bf.ART_TEXTURES_DIR / f"_ui_star_small_{suf}.png"
        )

    # _game_particle_star
    bf.im_outline(bf.im_star(270), radius=20, color=(255, 255, 255, 255)).save(
        bf.ART_TEXTURES_DIR / "_game_particle_star.png"
    )

    # _game_particle_circle_outline
    bf.im_ellipse(
        180, width=int(20 * 7 / 8), fill=(0, 0, 0, 0), outline=(255, 255, 255)
    ).save(bf.ART_TEXTURES_DIR / "_game_particle_circle_outline.png")

    # _game_feedback_circle
    bf.im_outline(
        bf.im_ellipse(60), radius=100, is_shadow=True, color=(255, 255, 255, 255)
    ).save(bf.ART_TEXTURES_DIR / "_game_feedback_circle.png")

    # _game_particle_diamond
    diamond_size = 320
    rh = Image.new("RGBA", (diamond_size, diamond_size), (255, 255, 255, 255))
    ell = bf.im_ellipse(diamond_size, fill=(0, 0, 0, 255))
    for off in ((0, 0), (1, 0), (0, 1), (1, 1)):
        rh.paste(
            ell,
            (
                off[0] * diamond_size - diamond_size // 2,
                off[1] * diamond_size - diamond_size // 2,
            ),
            ell,
        )
    bf.im_extract_white(rh).save(bf.ART_TEXTURES_DIR / "_game_particle_diamond.png")

    # _ui_icon_ad_small
    bf.im_outline(
        bf.im_scale(Image.open(bf.ART_DIR / "src" / "icons" / "ic_video_fill.png"), 0.25),
        radius=OUTLINE_WIDTH,
    ).save(bf.ART_TEXTURES_DIR / "_ui_icon_ad_small.png")

    # Spritesheetifying items.
    bf.im_spritesheetify(
        bf.ART_DIR / "src" / "main_001.png",
        cell_size=480,
        size=(7, 8),
        gap=10,
        out_dir=bf.ART_TEXTURES_DIR / "items",
        out_filename_prefix="_game_item_",
    )
    for f in (bf.ART_TEXTURES_DIR / "items").glob("*.png"):
        img = Image.open(f)
        img.save(bf.ART_TEXTURES_DIR / f.name)
        bf.im_white(img).save(bf.ART_TEXTURES_DIR / (f.stem + "_dark.png"))

    bf.im_conveyor(
        "icons",
        "Icons",
        bf.imc_prefix(""),
        bf.imc_scale(0.55),
        bf.imc_outline(radius=OUTLINE_WIDTH),
    )

    # Screenshots.
    # if 1:
    banner = Image.open(bf.ART_DIR / "src" / "dist_screenshot.png")
    # banner =
    # h = 185
    # # margin = 20
    # margin = 0
    # outline_width = 7
    # w = 1920 + 2 * outline_width
    # rect = bf.im_rectangle(
    #     (w - margin * 2, h - margin),
    #     fill="white",
    #     # radius=80,
    #     width=outline_width,
    #     outline="black",
    # )
    # banner.paste(rect, ((1920 - w) // 2 + margin, 1080 - h + outline_width))
    # # else:
    # #     banner = Image.open(bf.ART_DIR / "src" / "screenshot_text_banner.png")
    banner = bf.im_outline(
        banner,
        radius=40,
        color=(0, 0, 0, int(255 * 4 / 16)),
        is_shadow=True,
        extend=False,
    )
    banner_colors = [
        "8fd3ff",
        "eaaded",
        "91db69",
        "8fd3ff",
    ]
    _result = bf.read_localization_csv()
    screenshot_loc_id_indices = sorted(
        i for i, x in enumerate(_result.loc_ids) if x.startswith("YANDEX_SCREENSHOT_")
    )
    font = ImageFont.truetype(
        bf.ART_DIR / "src" / "screenshots" / "SeymourOne-Regular.ttf", size=150
    )

    for language, texts in _result.loc_by_languages.items():
        out_dir = bf.ART_DIR / "src" / "screenshots_processed" / language
        for f in out_dir.glob("*.png"):
            f.unlink()
        bf.recursive_mkdir(out_dir)

        for banner_color_, loc_id_index, f in zip(
            banner_colors,
            screenshot_loc_id_indices,
            (bf.ART_DIR / "src" / "screenshots").glob("*.png"),
            strict=True,
        ):
            banner_color = bf.hex_to_rgb_ints(banner_color_)

            text_image = Image.new("RGBA", (3840, 1550))
            draw = ImageDraw.Draw(text_image)
            draw.text(
                (1920, 1478),
                texts[loc_id_index],
                fill=tuple(
                    int(x * 255)
                    for x in bf.transform_color(
                        bf.hex_to_rgb_floats(banner_color_),
                        saturation_scale=0.23,
                        value_scale=2.5,
                    )
                ),
                anchor="ms",
                font=font,
                stroke_width=14,
                stroke_fill="black",
            )
            text_image = bf.im_outline(
                text_image,
                radius=40,
                color=(0, 0, 0, round(255 * 3 / 16)),
                is_shadow=True,
                extend=False,
            )

            brightness = 1.0
            contrast = 1.0
            if 1:
                brightness = 1.06
                contrast = 1.1

            img = Image.new("RGBA", (1920, 1080))
            img.paste(
                ImageEnhance.Brightness(
                    ImageEnhance.Contrast(Image.open(f)).enhance(contrast)
                ).enhance(brightness),
                (0, -100),
            )
            bf.im_draw_on_top(
                bf.im_draw_on_top(img, banner, (*banner_color, 255)),
                text_image.resize((1920, 1080)),
            ).save(out_dir / f.name)

    import bf_cli  # noqa

    bf_cli.do_generate(bf.BuildPlatform.Win, bf.BuildType.Debug)
    bf_cli.do_activate_game_ahk()

    # }


@command
def temp():
    nlp = spacy.load("ru_core_news_sm")
    text = input("enter: ")
    lemmas = [token.lemma_ for token in nlp(text) if token.is_alpha]
    values = []
    for k, v in Counter(lemmas).items():
        values.append((v, k))
    values.sort(key=lambda x: -x[0])
    for v, k in values:
        if v >= 3 and len(k) > 2:
            print(v, k)


###
