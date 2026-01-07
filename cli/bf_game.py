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
        d = bf.ldtk_load(bf.ASSETS_DIR / "level.ldtk")
        levels = []

        cycleable_levels_indices = []

        extf = gamelib["extend_cells_floor"]
        extc = gamelib["extend_cells_ceiling"]
        exth = gamelib["extend_cells_horizontal"]

        for level_index, level in enumerate(d.levels):
            walls = level.get_layer("Walls")
            sy = walls.cHei_
            zones = []
            layer_entities = level.get_layer("Entities")
            for entity in layer_entities.entityInstances:
                if entity.identifier_ == "Zone":
                    zones.append(
                        {
                            "px": entity.grid_[0] + exth,
                            "py": sy - entity.grid_[1] - 1 + extf,
                            "w": entity.width // 16,
                            "passengers_right": entity.field("RightLeft") == "Left",
                        }
                    )
            player = bf.ldtk_get_single_entity(layer_entities, "Player")
            tile_lines = list(bf.batched(walls.intGridCsv, walls.cWid_))
            for line in tile_lines:
                for _ in range(exth):
                    line.insert(0, line[0])
                    line.append(line[-1])
            for _ in range(extf):
                tile_lines.append(tile_lines[-1])
            tile_lines.reverse()
            for _ in range(extc):
                tile_lines.append(tile_lines[-1])

            tiles = [x for line in tile_lines for x in line]
            levels.append(
                {
                    "sx": walls.cWid_ + exth * 2,
                    "sy": sy + extc + extf,
                    "player": (
                        player.grid_[0] + 1 + exth,
                        sy - player.grid_[1] - 1 + extf,
                    ),
                    "zones": zones,
                    "tiles": tiles,
                }
            )
            if level.field("Cycle"):
                cycleable_levels_indices.append(level_index)

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
    # Spritesheetifying tileset.
    for f in (bf.ART_TEXTURES_DIR / "src").glob("game_tileset_*.png"):
        f.unlink()
    bf_image.spritesheetify(
        bf.ART_DIR / "src" / "tileset.png",
        gap=0,
        cell_size=108,
        size=(4, 4),
        out_filename_prefix="game_tileset_",
        out_dir=bf.ART_TEXTURES_DIR,
        trim_transparent=False,
        stop_on_finding_empty_sprite=False,
    )
    # }


@command
def make_swatch():
    # {  ###
    colors = [
        "#ffffff",
        "#2c4941",
        "#66a650",
        "#b9d850",
        "#82dcd7",
        "#208cb2",
        "#253348",
        "#1d1b24",
        "#3a3a41",
        "#7a7576",
        "#b59a66",
        "#cec7b1",
        "#edefe2",
        "#d78b98",
        "#a13d77",
        "#6d2047",
        "#3c1c43",
        "#2c2228",
        "#5e3735",
        "#885a44",
        "#b8560f",
        "#dc9824",
        "#efcb84",
        "#e68556",
        "#c02931",
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
