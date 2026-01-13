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
from itertools import groupby

import bf_image
import bf_lib as bf
from bf_typer import command, timing
from PIL import Image

# }

bf.game_settings.itch_target = "hulvdan/emberveil2"
bf.game_settings.languages = ["russian", "english"]
bf.game_settings.generate_flatbuffers_api_for = ["bf_save.fbs"]
bf.game_settings.yandex_metrica_counter_id = 105874717
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


OUTLINE_WIDTH = 10
BUTTON_RADIUS = 20
BUTTON_SIZE = 112


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

    # UI.
    # ============================================================
    gamelib["ui_button_nine_slice"] = {
        "texture_id": "_ui_button",
        "left": BUTTON_RADIUS + OUTLINE_WIDTH,
        "right": BUTTON_RADIUS + OUTLINE_WIDTH,
        "top": BUTTON_RADIUS + OUTLINE_WIDTH,
        "bottom": BUTTON_RADIUS + OUTLINE_WIDTH,
    }

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
            if s is None:
                sx = entities.cWid_
                sy = entities.cHei_
                s = (sx, sy)
            else:
                assert s == (entities.cWid_, entities.cHei_), (
                    "All levels must have the same size!"
                )

            shelves = []
            for entity in entities.entityInstances:
                if entity.identifier_ == "Zone":
                    shelves.append({"pos": (entity.grid_[0], sy - entity.grid_[1] - 1)})
            shelves.sort(key=lambda x: (x["pos"][1], x["pos"][0]))  # type: ignore
            player = bf.ldtk_get_single_entity(entities, "Player")

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

    # _ui_star_small
    star_image = Image.open(bf.ART_TEXTURES_DIR / "ui_star.png")
    star_image_scale = 0.1
    bf_image.outline(
        star_image.resize(
            (
                int(star_image.size[0] * star_image_scale),
                int(star_image.size[1] * star_image_scale),
            )
        ),
        radius=2,
    ).save(bf.ART_TEXTURES_DIR / "_ui_star_small.png")

    # _button
    bf_image.outline(
        bf_image.remap(
            Image.open(bf.ART_TEXTURES_DIR / "other" / "ui_button.png"),
            bf.palette_color_tuple3("CONIFER_DARK"),
            bf.palette_color_tuple3("CONIFER"),
        ),
        radius=OUTLINE_WIDTH,
    ).save(bf.ART_TEXTURES_DIR / "_ui_button.png")

    # _game_particle_star
    bf_image.outline(bf_image.star(320), radius=20, color=(255, 255, 255, 255)).save(
        bf.ART_TEXTURES_DIR / "_game_particle_star.png"
    )

    # _game_feedback_circle
    bf_image.outline(
        bf_image.ellipse(60), radius=100, is_shadow=True, color=(255, 255, 255, 255)
    ).save(bf.ART_TEXTURES_DIR / "_game_feedback_circle.png")

    # _game_particle_diamond
    diamond_size = 320
    rh = Image.new("RGBA", (diamond_size, diamond_size), (255, 255, 255, 255))
    ell = bf_image.ellipse(diamond_size, fill=(0, 0, 0, 255))
    for off in ((0, 0), (1, 0), (0, 1), (1, 1)):
        rh.paste(
            ell,
            (
                off[0] * diamond_size - diamond_size // 2,
                off[1] * diamond_size - diamond_size // 2,
            ),
            ell,
        )
    bf_image.extract_white(rh).save(bf.ART_TEXTURES_DIR / "_game_particle_diamond.png")

    bf_image.spritesheetify(
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
        bf_image.white(img).save(bf.ART_TEXTURES_DIR / (f.stem + "_dark.png"))

    bf_image.conveyor(
        "icons",
        "Icons",
        bf_image.conveyor_prefix(""),
        bf_image.conveyor_scale(0.5),
        bf_image.conveyor_outline(radius=OUTLINE_WIDTH),
    )
    # }


@command
def temp():
    pass


###
