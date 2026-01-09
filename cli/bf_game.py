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
import bf_swatch
from bf_typer import command, timing

# }

bf.game_settings.itch_target = "hulvdan/emberveil2"
bf.game_settings.languages = ["russian", "english"]
bf.game_settings.generate_flatbuffers_api_for = ["bf_save.fbs"]
bf.game_settings.yandex_metrica_counter_id = 105874717


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

    # Levels.
    # ============================================================
    # {  ///
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

            zones = []
            for entity in entities.entityInstances:
                if entity.identifier_ == "Zone":
                    zones.append({"pos": (entity.grid_[0], sy - entity.grid_[1] - 1)})
            player = bf.ldtk_get_single_entity(entities, "Player")
            levels.append(
                {
                    "player": (player.grid_[0] + 1, sy - player.grid_[1] - 1),
                    "zones": zones,
                    "override_total_passenger_rows": level.field(
                        "OverrideTotalPassengerRows"
                    ),
                    "override_empty_passenger_rows": level.field(
                        "OverrideEmptyPassengerRows"
                    ),
                    "random_seed": level.field("RandomSeed"),
                }
            )
            if level.field("Cycle"):
                cycleable_levels_indices.append(level_index)

        gamelib["world_size"] = s
        gamelib["levels"] = levels
        gamelib["cycleable_levels_indices"] = cycleable_levels_indices
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
    UI_FRAME_RADIUS = 30

    # `ui_frame`.
    frame_image = bf_image.rectangle(
        112,
        radius=UI_FRAME_RADIUS,
        width=10,
        # outline=bf.hex_to_rgb_ints("000000"),
        fill=bf.hex_to_rgb_ints("ffffff"),
    )
    frame_image.save(bf.ART_TEXTURES_DIR / "ui_frame.png")

    DEBUG_SHADOWS = 0

    # `ui_frame_shadow_small`.
    bf_image.outline(
        image=bf_image.red(frame_image),
        radius=60,
        color=(0, 0, 0, 255),
        is_shadow=True,
        blend_image_on_top=DEBUG_SHADOWS,
    ).save(bf.ART_TEXTURES_DIR / "ui_frame_shadow_small.png")
    # }


@command
def make_swatch():
    # {  ###
    colors = [
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

    new_colors = ["#ffffff", "#000000"]

    for i in range(len(colors)):
        color = colors[i]
        if color in ("#000000", "#ffffff"):
            continue
        c = bf.rgb_floats_to_hex(
            bf.transform_color(
                bf.hex_to_rgb_floats(color),
                saturation_scale=1.2,
                value_scale=0.52,
            )
        )
        new_colors.append(color)
        new_colors.append(c)
        colors.append(c)

    def process_color(color: str) -> dict:
        return {
            "name": color,
            "type": "Global",
            "data": {
                "mode": "RGB",
                "values": bf.hex_to_rgb_floats(color),
            },
        }

    swatch_data = [process_color(c) for c in colors]
    bf_swatch.write(swatch_data, "aboba.ase")

    def process_color2(color: str) -> bf_swatch.RawColor:
        r, g, b = bf.hex_to_rgb_ints(color)
        r = int(r * 65535 / 255)
        g = int(g * 65535 / 255)
        b = int(b * 65535 / 255)
        assert r < 65536, r
        assert g < 65536, g
        assert b < 65536, b
        assert r >= 0, r
        assert g >= 0, g
        assert b >= 0, b

        return bf_swatch.RawColor(
            name=color,
            color_space=bf_swatch.ColorSpace.RGB,
            component_1=r,
            component_2=g,
            component_3=b,
            component_4=65535,
        )

    with open("aboba.aco", "wb") as out_file:
        bf_swatch.save_aco_file([process_color2(c) for c in new_colors], out_file)

    with open("aboba.pal", "w") as out_file_2:
        out_file_2.write(
            "JASC-PAL\n0100\n{}\n".format(
                len(new_colors),
            )
        )
        color_lines = []
        for color in new_colors:
            r, g, b = bf.hex_to_rgb_ints(color)
            color_lines.append(f"{r} {g} {b}")
        color_lines = color_lines[2:] + color_lines[:2]
        out_file_2.write("\n".join(color_lines))
    # }


@command
def temp():
    pass


###
