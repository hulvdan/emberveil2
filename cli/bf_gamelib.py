# Imports.  {  ###
import csv
import json
import os
import shutil
import tempfile
from collections import defaultdict
from dataclasses import dataclass
from enum import Enum, unique
from functools import partial
from math import radians
from pathlib import Path
from typing import Any, TypeAlias

import pytest
from bf_game import *  # noqa
from bf_lib import (
    ART_DIR,
    ART_TEXTURES_DIR,
    ASSETS_DIR,
    FLATBUFFERS_GENERATED_DIR,
    FLATC_PATH,
    GAME_DIR,
    HANDS_GENERATED_DIR,
    PROJECT_DIR,
    RESOURCES_DIR,
    SHADERC_PATH,
    SRC_DIR,
    TEMP_ART_DIR,
    TEMP_DIR,
    VENDOR_DIR,
    BuildPlatform,
    BuildType,
    game_settings,
    gamelib_processing_functions,
    genenum,
    load_gamelib_cached,
    log,
    recursive_mkdir,
    recursive_replace_transform,
    replace_double_spaces,
    run_command,
    stable_hash,
)
from bf_typer import timing, timing_mark
from PIL import Image

# }


def texture_ids_recursive_transform(
    gamelib_recursed, transform_texture_id, transform_texture_ids_list
) -> None:
    # {  ###
    if not isinstance(gamelib_recursed, dict):
        return

    for key, value in gamelib_recursed.items():
        if isinstance(value, dict):
            texture_ids_recursive_transform(
                value, transform_texture_id, transform_texture_ids_list
            )
        elif isinstance(key, str) and ("texture_ids" in key or "texture_id" in key):
            if "texture_ids" in key:
                assert isinstance(value, list)
                for v in value:
                    assert isinstance(v, str)
                transform_texture_ids_list(gamelib_recursed, key)
            elif "texture_id" in key:
                assert isinstance(value, str)
                transform_texture_id(gamelib_recursed, key)
        elif isinstance(value, list):
            for v in value:
                if isinstance(v, dict):
                    texture_ids_recursive_transform(
                        v, transform_texture_id, transform_texture_ids_list
                    )
    # }


def degrees_to_radians_recursive_transform(gamelib_recursed) -> None:
    # {  ###
    if not isinstance(gamelib_recursed, dict):
        return

    keys_to_replace = [key for key in gamelib_recursed if key.endswith("_deg")]

    for key in keys_to_replace:
        new_key = key.replace("_deg", "_rad")
        value = gamelib_recursed.pop(key)
        assert isinstance(value, (int, float))
        gamelib_recursed[new_key] = radians(value)

    del keys_to_replace

    for value in gamelib_recursed.values():
        if isinstance(value, dict):
            degrees_to_radians_recursive_transform(value)

        elif isinstance(value, list):
            for v in value:
                if isinstance(v, dict):
                    degrees_to_radians_recursive_transform(v)
    # }


@unique
class BrokenStringDatumType(Enum):
    INVALID = 0
    STRING = 1
    PLACEHOLDER = 2
    SPACE = 3


@dataclass
class BrokenStringDatum:
    type: BrokenStringDatumType
    string: str | None


StringGroup: TypeAlias = list[BrokenStringDatum]
StringLine: TypeAlias = list[StringGroup]


class StringMalformedError(Exception): ...


def process_group(string: str) -> StringGroup:
    # {  ###
    assert string
    assert " " not in string
    assert "\t" not in string
    assert "\n" not in string

    result: StringGroup = []

    while "{" in string:
        l = string.find("{")
        r = string.find("}")
        if l:
            result.append(
                BrokenStringDatum(type=BrokenStringDatumType.STRING, string=string[:l])
            )
        result.append(
            BrokenStringDatum(
                type=BrokenStringDatumType.PLACEHOLDER,
                string=string[l + 1 : r].split("__", 1)[0],
            )
        )
        string = string[r + 1 :]

    if string:
        result.append(BrokenStringDatum(type=BrokenStringDatumType.STRING, string=string))

    return result
    # }


def process_string(string: str) -> list[StringLine]:
    # {  ###
    string = replace_double_spaces(string.strip().replace("\t", " ").replace("\r", ""))

    result: list[StringLine] = []

    sp = string.split("\n")
    if not any(sp):
        return result

    for line in sp:
        line_result: list[StringGroup] = []
        result.append(line_result)

        try:
            groups = line.split(" ")
            for i, group in enumerate(groups):
                if not line:
                    continue
                g = process_group(group)
                if i < len(groups) - 1:
                    g.append(
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None)
                    )
                line_result.append(g)

        except StringMalformedError:
            print(f"Line is malformed! `{line}`")
            raise

    return result
    # }


def test_process_group():
    # {  ###
    assert process_group("a") == [
        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="a")
    ]
    assert process_group("ab") == [
        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="ab")
    ]
    assert process_group("a,b") == [
        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="a,b")
    ]
    assert process_group("a{ABOBA}") == [
        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="a"),
        BrokenStringDatum(type=BrokenStringDatumType.PLACEHOLDER, string="ABOBA"),
    ]
    assert process_group("{ABOBA}a") == [
        BrokenStringDatum(type=BrokenStringDatumType.PLACEHOLDER, string="ABOBA"),
        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="a"),
    ]
    assert process_group("+{ABOBA},") == [
        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="+"),
        BrokenStringDatum(type=BrokenStringDatumType.PLACEHOLDER, string="ABOBA"),
        BrokenStringDatum(type=BrokenStringDatumType.STRING, string=","),
    ]
    assert process_group("{ABOBA},") == [
        BrokenStringDatum(type=BrokenStringDatumType.PLACEHOLDER, string="ABOBA"),
        BrokenStringDatum(type=BrokenStringDatumType.STRING, string=","),
    ]
    assert process_group("{A}{B}") == [
        BrokenStringDatum(type=BrokenStringDatumType.PLACEHOLDER, string="A"),
        BrokenStringDatum(type=BrokenStringDatumType.PLACEHOLDER, string="B"),
    ]
    assert process_group("{A}b{C}") == [
        BrokenStringDatum(type=BrokenStringDatumType.PLACEHOLDER, string="A"),
        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="b"),
        BrokenStringDatum(type=BrokenStringDatumType.PLACEHOLDER, string="C"),
    ]
    assert process_group("{AB}cd{EF}") == [
        BrokenStringDatum(type=BrokenStringDatumType.PLACEHOLDER, string="AB"),
        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="cd"),
        BrokenStringDatum(type=BrokenStringDatumType.PLACEHOLDER, string="EF"),
    ]
    # }


@pytest.mark.parametrize(  # {  ###
    ("string", "result"),
    [
        ("", []),
        ("\n\n", []),
        ("\t\t", []),
        (
            "ab",
            [
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="ab"),
                    ],
                ],
            ],
        ),
        (
            "a. b",
            [
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="a."),
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None),
                    ],
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="b"),
                    ],
                ],
            ],
        ),
        (
            "ab ke",
            [
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="ab"),
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None),
                    ],
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="ke"),
                    ],
                ],
            ],
        ),
        (
            "ab\tke",
            [
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="ab"),
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None),
                    ],
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="ke"),
                    ],
                ],
            ],
        ),
        (
            " ab  ke ",
            [
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="ab"),
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None),
                    ],
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="ke"),
                    ],
                ],
            ],
        ),
        (
            " +{CHANCE}% to explode ",
            [
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="+"),
                        BrokenStringDatum(
                            type=BrokenStringDatumType.PLACEHOLDER, string="CHANCE"
                        ),
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="%"),
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None),
                    ],
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="to"),
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None),
                    ],
                    [
                        BrokenStringDatum(
                            type=BrokenStringDatumType.STRING, string="explode"
                        ),
                    ],
                ],
            ],
        ),
        (
            " +{CHANCE__ALIAS}%, to explode ",
            [
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="+"),
                        BrokenStringDatum(
                            type=BrokenStringDatumType.PLACEHOLDER, string="CHANCE"
                        ),
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="%,"),
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None),
                    ],
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="to"),
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None),
                    ],
                    [
                        BrokenStringDatum(
                            type=BrokenStringDatumType.STRING, string="explode"
                        ),
                    ],
                ],
            ],
        ),
        (
            "{CHANCE}...",
            [
                [
                    [
                        BrokenStringDatum(
                            type=BrokenStringDatumType.PLACEHOLDER, string="CHANCE"
                        ),
                        BrokenStringDatum(
                            type=BrokenStringDatumType.STRING, string="..."
                        ),
                    ],
                ],
            ],
        ),
        (
            "b\na c",
            [
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="b"),
                    ],
                ],
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="a"),
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None),
                    ],
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="c"),
                    ],
                ],
            ],
        ),
        (
            "b\n\na c",
            [
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="b"),
                    ],
                ],
                [],
                [
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="a"),
                        BrokenStringDatum(type=BrokenStringDatumType.SPACE, string=None),
                    ],
                    [
                        BrokenStringDatum(type=BrokenStringDatumType.STRING, string="c"),
                    ],
                ],
            ],
        ),
    ],
)  # }
def test_process_string(string: str, result: list[StringLine]) -> None:
    x = process_string(string)
    assert x == result


def _do_localization(genline, gamelib) -> tuple[set[int], dict[str, int]]:
    # {  ###
    loc_ids: list[str] = []
    loc_by_languages: dict[str, list[str]] = defaultdict(list)

    with open(ASSETS_DIR / "localization.csv", encoding="utf-8") as in_file:
        not_language_columns = ("id", "\ufeffid", "comment")
        for row in csv.DictReader(in_file, delimiter=";"):
            row_id = row.get("id") or row.get("\ufeffid")
            assert isinstance(row_id, str)
            loc_ids.append(row_id)
            for c in row:
                if c not in not_language_columns:
                    assert c in game_settings.languages
                    translation = row[c]
                    loc_by_languages[c].append(
                        translation.strip() or "<<NOT_TRANSLATED>>"
                    )

    locale_caps_offset = -1

    l = len(loc_ids)
    for i in range(l):
        loc = loc_ids[i]
        if loc.startswith("UI_"):
            if locale_caps_offset == -1:
                locale_caps_offset = len(loc_ids) - i
            loc_ids.append("{}__CAPS".format(loc))

    genline("constexpr int LOCALE_CAPS_OFFSET = {};\n".format(locale_caps_offset))
    genenum(genline, "Loc", ["INVALID", *loc_ids], add_count=True)

    for strings in loc_by_languages.values():
        l = len(strings)
        for i in range(l):
            if loc_ids[i].startswith("UI_"):
                strings.append(strings[i].upper())
        strings.insert(0, "<<INVALID>>")

    loc_ids.insert(0, "<<INVALID>>")

    for strings in loc_by_languages.values():
        assert len(loc_ids) == len(strings)

    gamelib["localization_debug_strings"] = loc_ids

    locale_to_index: dict[str, int] = {key: i for i, key in enumerate(loc_ids)}
    index_to_locale = {i: codename for codename, i in locale_to_index.items()}

    SKIP_CHARACTERS = ("\n", "\t", "\r")
    codepoints: set[int] = set()

    for strings in loc_by_languages.values():
        for string in strings:
            codepoints.update(ord(c) for c in string if c not in SKIP_CHARACTERS)

    gamelib["localizations"] = [{"strings": x} for x in loc_by_languages.values()]

    # Making `broken_lines`.
    if 1:
        genenum(genline, "BrokenStringDatumType", [x.name for x in BrokenStringDatumType])
        all_russian_placeholders: list[tuple[int, str]] = []

        for loc_index, loc in enumerate(gamelib["localizations"]):
            is_russian = not loc_index
            broken_lines: list = []
            loc["broken_lines"] = broken_lines

            for string_index, string in enumerate(loc["strings"]):
                string_lines: list = []
                broken_lines.append({"lines": string_lines})
                string_placeholders_to_verify = set()

                for line in process_string(string):
                    grs: list = []
                    string_lines.append({"groups": grs})

                    for group in line:
                        gr: list = []
                        grs.append({"strings": gr})

                        for datum in group:
                            placeholder: str | None = None

                            if datum.type == BrokenStringDatumType.PLACEHOLDER:
                                assert datum.string
                                placeholder_data = (string_index, datum.string)
                                placeholder = datum.string

                                if is_russian and (
                                    placeholder_data not in all_russian_placeholders
                                ):
                                    all_russian_placeholders.append(placeholder_data)

                                string_placeholders_to_verify.add(datum.string)

                            gr.append(
                                {
                                    "type": datum.type.value,
                                    "placeholder": placeholder,
                                    "string": datum.string,
                                }
                            )

                if not is_russian:
                    russian_placeholders = set(
                        x[1] for x in all_russian_placeholders if x[0] == string_index
                    )
                    assert string_placeholders_to_verify == russian_placeholders, (
                        "Translated string differs in placeholders",
                        game_settings.languages[loc_index],
                        index_to_locale[string_index],
                    )

    return codepoints, locale_to_index
    # }


@timing
def convert_gamelib_json_to_binary(
    texture_name_2_id: dict[str, int], genline, atlas_data, original_texture_sizes
) -> None:
    # {  ###
    gamelib = load_gamelib_cached()

    gamelib["original_texture_sizes"] = original_texture_sizes

    # Enriching gamelib with sounds.
    if 1:
        do_audio()

        sound_paths = list(RESOURCES_DIR.glob("*.ogg"))

        m = 2**32
        sound_types_ = [
            (t, (stable_hash(t) % m))
            for t in {
                sound_path.stem.split("__", 1)[0].upper() for sound_path in sound_paths
            }
        ]
        sound_types_.sort(key=lambda x: x[1])
        sound_types = [x[0] for x in sound_types_]
        sound_enum_values = [x[1] for x in sound_types_]

        genenum(
            genline,
            "Sound",
            sound_types,
            override_values=sound_enum_values,
            enum_type="u32",
        )
        genline("constexpr int SOUNDS_COUNT = {};\n".format(len(sound_enum_values)))
        genline("constexpr int INDEX_TO_SOUND_[]{  ///")
        for t in sound_types:
            genline(f"  Sound_{t},")
        genline("};")
        genline("VIEW_FROM_ARRAY_DANGER(INDEX_TO_SOUND);\n")

        sound_variations_per_type: dict[str, list[Path]] = defaultdict(list)
        for sound_path in sound_paths:
            sound_variations_per_type[sound_path.stem.split("__", 1)[0].upper()].append(
                "resources/" + sound_path.name
            )

        sounds: list[Any] = []
        gamelib["sounds"] = sounds
        for sound_type, enum_value_id in sound_types_:
            x = {
                "enum_value_id": enum_value_id,
                "variations": sound_variations_per_type[sound_type],
            }
            sounds.append(x)

    gamelib |= atlas_data
    genenum(genline, "DrawZ", gamelib.pop("draw_z"), add_count=True)

    localization_codepoints, locale_to_index = _do_localization(genline, gamelib)

    for gamelib_processing_function in gamelib_processing_functions:
        gamelib_processing_function(genline, gamelib, localization_codepoints)

    # Asserting on not found textures.
    if 1:
        not_found_textures: list[str] = []
        transform_texture_id = lambda data, key: transform_to_texture_index(
            data,
            key,
            texture_name_2_index=texture_name_2_id,
            not_found_textures=not_found_textures,
        )
        transform_texture_ids_list = lambda data, key: transform_to_texture_indexes_list(
            data,
            key,
            texture_name_2_index=texture_name_2_id,
            not_found_textures=not_found_textures,
        )
        texture_ids_recursive_transform(
            gamelib, transform_texture_id, transform_texture_ids_list
        )
        if not_found_textures:
            assert False, "Couldn't find textures:\n{}".format(
                "\n".join(f"- {x}" for x in not_found_textures)
            )

    degrees_to_radians_recursive_transform(gamelib)

    recursive_replace_transform(gamelib, "locale", "locales", locale_to_index)

    # Creation of `gamelib.bin`.
    intermediate_path = TEMP_DIR / "gamelib.intermediate.jsonc"
    intermediate_path.write_text(json.dumps(gamelib, indent=4))
    run_command(
        [
            FLATC_PATH,
            "-o",
            TEMP_DIR,
            "-b",
            GAME_DIR / "bf_gamelib.fbs",
            intermediate_path,
        ]
    )

    intermediate_binary_path = Path(str(intermediate_path).rsplit(".", 1)[0] + ".bin")
    shutil.move(intermediate_binary_path, RESOURCES_DIR / "gamelib.bin")
    # }}


def downscale_images(downscale_factors: list[int]) -> None:
    # {  ###
    assert downscale_factors, downscale_factors

    images_to_downscale = list(ART_TEXTURES_DIR.glob("*.png"))

    for factor in downscale_factors:
        assert factor >= 1, factor

        log.info("Downscaling by {}".format(factor))

        export_dir = TEMP_ART_DIR / f"d{factor}"
        recursive_mkdir(export_dir)

        for image_path in images_to_downscale:
            s1 = image_path.stat()

            export_image_path = export_dir / image_path.name

            if export_image_path.exists() and (
                s1.st_mtime_ns == export_image_path.stat().st_mtime_ns
            ):
                continue

            im = Image.open(image_path)
            im.thumbnail(
                (im.size[0] // factor, im.size[1] // factor), Image.Resampling.LANCZOS
            )
            im.save(export_image_path, "PNG")
            os.utime(export_image_path, ns=(s1.st_atime_ns, s1.st_mtime_ns))
    # }


def texture_cmp_key(x: dict, factor: int) -> tuple[bool, str]:
    return (x["debug_name"] != f"d{factor}/undefined", x["debug_name"])


@timing
def make_atlases(downscale_factors: list[int]) -> tuple[dict[str, int], list[dict]]:
    # {  ###
    assert downscale_factors, downscale_factors

    texture_name_2_id: dict[str, int] = {}
    atlases_data = []

    for factor in downscale_factors:
        path = ART_DIR / f"atlas_d{factor}.ftpp"

        cache_filepath = TEMP_DIR / f".atlas_d{factor}.cache"

        old_cache_value = -1
        if cache_filepath.exists():
            old_cache_value = int(cache_filepath.read_text())

        cache_value = 0
        for filepath in Path(TEMP_ART_DIR / f"d{factor}").rglob("*.png"):
            cache_value = stable_hash(
                cache_value
                + stable_hash(filepath.stat().st_mtime_ns)
                + stable_hash(filepath.name)
            )

        should_regenerate_atlas = False
        temp_atlas_path = TEMP_DIR / (path.stem + ".png")

        if (cache_value == old_cache_value) and temp_atlas_path.exists():
            log.info("Skipped atlas generation - no images changed")
            timing_mark("skipped png generation")
        else:
            # Генерируем атлас из .ftpp файла. Создаются .json спецификация и .png текстура.
            log.info("Generating atlas...")
            run_command("free-tex-packer-cli --project {}".format(path))

            cache_filepath.write_text(str(cache_value), newline="\n")

            should_regenerate_atlas = True

        # Подгоняем спецификацию под наш формат.
        json_path = TEMP_DIR / (path.stem + ".json")
        with open(json_path) as json_file:
            json_data = json.load(json_file)

        found_textures: set[str] = set()

        textures: list[Any] = []
        for name_, data in json_data["frames"].items():
            name = name_.removeprefix("art/")

            assert name not in found_textures
            found_textures.add(name)

            texture_data = {
                "id": -1,
                "debug_name": name,
                "size_x": data["frame"]["w"],
                "size_y": data["frame"]["h"],
                "atlas_x": data["frame"]["x"],
                "atlas_y": data["frame"]["y"],
                "baseline": 0,
            }
            textures.append(texture_data)

        textures.sort(key=partial(texture_cmp_key, factor=factor))

        if not texture_name_2_id:
            assert textures, textures

            for i, texture in enumerate(textures):
                texture["id"] = i
                name = texture["debug_name"].removeprefix(f"d{factor}/")
                texture_name_2_id[name] = i

        recursive_mkdir(RESOURCES_DIR)

        out_atlas_path = RESOURCES_DIR / (path.stem + ".basis")
        if not out_atlas_path.exists() or should_regenerate_atlas:
            run_command(
                [
                    PROJECT_DIR / "cli" / "basisu.exe",
                    "-uastc",
                    "-slower",
                    "-file",
                    temp_atlas_path,
                    "-output_file",
                    out_atlas_path,
                ],
                cwd=TEMP_DIR,
            )

            run_command(
                [
                    PROJECT_DIR / "cli" / "basisu.exe",
                    "-validate",
                    "-file",
                    out_atlas_path,
                ],
                cwd=TEMP_DIR,
            )

        else:
            timing_mark(f"{factor}: skipped optimized copying")

        atlases_data.append(
            {
                "atlas_downscale_factor": factor,
                "atlas_textures": textures,
                "atlas_size_x": json_data["meta"]["size"]["w"],
                "atlas_size_y": json_data["meta"]["size"]["h"],
            }
        )

    return texture_name_2_id, atlases_data
    # }


@timing
def check_no_excessive_images_in_temp_art_dir(downscale_factors: list[int]) -> None:
    # {  ###
    for factor in downscale_factors:
        TEMP_ART_DOWNSCALED_DIR = TEMP_ART_DIR / f"d{factor}"

        temp_filepaths = [
            filepath.relative_to(TEMP_ART_DOWNSCALED_DIR)
            for filepath in TEMP_ART_DOWNSCALED_DIR.rglob("*.png")
        ]
        art_filepaths = [
            filepath.relative_to(ART_TEXTURES_DIR)
            for filepath in ART_TEXTURES_DIR.glob("*.png")
        ]

        for temp_filepath in temp_filepaths:
            if temp_filepath not in art_filepaths:
                p = TEMP_ART_DOWNSCALED_DIR / temp_filepath
                log.info("Removing excessive image '{}'...".format(p))
                p.unlink()
    # }


@timing
def remove_intermediate_generation_files() -> None:
    for filepath in TEMP_DIR.rglob("*.intermediate*"):
        filepath.unlink()


def transform_to_texture_indexes_list(
    data: dict[str, Any],
    key: str,
    texture_name_2_index: dict[str, int],
    not_found_textures: list[str],
) -> None:
    # {  ###
    textures = data[key]
    assert isinstance(textures, list)

    for i, texture_name in enumerate(textures):
        if texture_name.lower() == "undefined":
            textures[i] = -1
        else:
            if texture_name in texture_name_2_index:
                textures[i] = texture_name_2_index[texture_name]
            else:
                not_found_textures.append(texture_name)
    # }


def transform_to_texture_index(
    data: dict[str, Any],
    key: str,
    texture_name_2_index: dict[str, int],
    not_found_textures: list[str],
) -> None:
    # {  ###
    texture_name = data[key]
    assert isinstance(texture_name, str)

    if texture_name.lower() == "undefined":
        data[key] = -1
        return

    if texture_name in texture_name_2_index:
        data[key] = texture_name_2_index[texture_name]
    else:
        not_found_textures.append(texture_name)
    # }


def listfiles_with_hashes_in_dir(path: str | Path) -> dict[str, int]:
    # {  ###
    filepath_string_to_hash = {}

    for filepath in Path(path).rglob("*"):
        with open(filepath, "rb") as in_file:
            filepath_string_to_hash[str(filepath.relative_to(path))] = hash(
                in_file.read()
            )

    return filepath_string_to_hash
    # }


@timing
def generate_flatbuffer_files():
    # {  ###
    recursive_mkdir(FLATBUFFERS_GENERATED_DIR)

    hashes_for_msbuild = listfiles_with_hashes_in_dir(FLATBUFFERS_GENERATED_DIR)

    # Генерируем cpp файлы из FlatBuffer (.fbs) файлов.
    flatbuffer_files = list(SRC_DIR.rglob("*.fbs"))

    with tempfile.TemporaryDirectory() as td:

        def gen(filepaths, *args):
            command = [
                FLATC_PATH,
                *args,
                "-o",
                td,
                "--cpp",
                *list(filepaths),
            ]

            run_command(command)

        gen(
            (
                i
                for i in flatbuffer_files
                if i.name in game_settings.generate_flatbuffers_api_for
            ),
            "--gen-object-api",
            "--reflect-names",
        )
        gen(
            i
            for i in flatbuffer_files
            if i.name not in game_settings.generate_flatbuffers_api_for
        )

        for file, file_hash in listfiles_with_hashes_in_dir(td).items():
            # Костыль, чтобы MSBuild не ребилдился каждый раз.
            if file_hash != hashes_for_msbuild.get(file):
                shutil.copyfile(Path(td) / file, FLATBUFFERS_GENERATED_DIR / file)
    # }


@timing
def remove_orphan_resources_files(platform: BuildPlatform, build_type: BuildType) -> None:
    # {  ###
    match platform:
        case BuildPlatform.Win:
            target_dir_ = f".cmake/vs17/{build_type}/resources"

        case BuildPlatform.Web | BuildPlatform.WebYandex:
            target_dir_ = f".cmake/{platform}_{build_type}/resources"

        case _:
            assert False, f"Not supported platform: {platform}"

    src_files = {f.name for f in RESOURCES_DIR.iterdir() if f.is_file()}

    target_dir = Path(target_dir_)

    if not target_dir.exists():
        return

    for file in target_dir.iterdir():
        if file.is_file() and file.name not in src_files:
            file.unlink()
            log.info(f"Removed orphan resources/ file '{file}'")
    # }


@timing
def do_audio() -> None:
    # {  ###
    AUDIO_SRC_DIR = ASSETS_DIR / "sfx"
    AUDIO_DST_DIR = RESOURCES_DIR

    src_files = {p for p in AUDIO_SRC_DIR.glob("*.ogg") if p.is_file()}

    if 1:
        # Making symlinks.
        for src_file in src_files:
            dst_file = AUDIO_DST_DIR / (src_file.stem + ".ogg")
            if dst_file.exists():
                if dst_file.is_symlink():
                    continue
                else:
                    dst_file.unlink()
            dst_file.symlink_to(src_file)

        log.info(f"Make symlinks for {len(src_files)} audio files")

    else:
        # Copying.
        copied = 0
        for src_file in src_files:
            dst_file = AUDIO_DST_DIR / (src_file.stem + ".ogg")

            if dst_file.exists():
                src_mtime = src_file.stat().st_mtime_ns
                dst_mtime = dst_file.stat().st_mtime_ns
                if dst_mtime == src_mtime:
                    continue

            copied += 1
            log.info(f"Copying {src_file} -> {dst_file}")

            shutil.copyfile(src_file, dst_file)
            shutil.copystat(src_file, dst_file)

        log.info(
            f"Found {len(src_files)} total files. (copied {copied}, others have the same modified time)"
        )

    # Removing orphan audio files from `resources` dir.
    orphans = []
    for dst_file in AUDIO_DST_DIR.glob("*.ogg"):
        src_file = AUDIO_SRC_DIR / dst_file.relative_to(AUDIO_DST_DIR)
        if not src_file.exists():
            orphans.append(dst_file.name)
            dst_file.unlink()

    if orphans:
        log.info(
            "Removed {} orphan audio files:\n{}".format(
                len(orphans), "\n".join(x for x in orphans)
            )
        )

    # }


@timing
def do_generate(platform: BuildPlatform, build_type: BuildType) -> None:
    # {  ###
    remove_orphan_resources_files(platform, build_type)

    if build_type == BuildType.Release and platform in (
        BuildPlatform.Web,
        BuildPlatform.WebYandex,
    ):
        shell_file = VENDOR_DIR / "shell_release.html"
        shell_contents = shell_file.read_text(encoding="utf-8")

        variables = {
            BuildPlatform.Web: {
                "EXTEND_BODY_START": "",
                "EXTEND_MAIN_SCRIPT": "",
            },
            BuildPlatform.WebYandex: {
                "EXTEND_BODY_START": """
                    <script src="/sdk.js"></script>
                """,
                "EXTEND_MAIN_SCRIPT": """
                    const moduleReady = new Promise(resolve => {
                        Module.onRuntimeInitialized = resolve;
                    });

                    (async () => {
                        window.ysdk = await YaGames.init();
                        await moduleReady;
                        window.player = await window.ysdk.getPlayer();
                        Module.ccall('mark_ysdk_loaded_from_js', null, [], []);

                        window.ysdk.on('game_api_pause', () => {
                            Module.ccall('pause_from_js', null, [], []);
                        });
                        window.ysdk.on('game_api_resume', () => {
                            Module.ccall('resume_from_js', null, [], []);
                        });

                        var lang = 1;
                        const languages_map = {
                            ru: 0,
                            be: 0,
                            kk: 0,
                            uk: 0,
                            uz: 0,
                            en: 1,
                        };
                        const l = window.ysdk.environment.i18n.lang;
                        if (l in languages_map)
                            lang = languages_map[l];
                        Module.ccall(
                            'set_localization_from_js', null, ['number'], [lang]
                        );
                    })();
                """,
            },
        }[platform]

        for variable, value in variables.items():
            var = f"##{variable}##"
            assert var in shell_contents
            shell_contents = shell_contents.replace(var, value)

        assert "##" not in shell_contents

        out_file = {
            BuildPlatform.Web: TEMP_DIR / "shell_release.html",
            BuildPlatform.WebYandex: TEMP_DIR / "shell_release_yandex.html",
        }[platform]
        out_file.write_text(shell_contents, encoding="utf-8")

    # Shaders.
    if 1:
        platform_mapping = {
            # BuildPlatform.Win: [("windows", "s_5_0")],
            # BuildPlatform.Win: [("windows", "s_4_0")],
            BuildPlatform.Win: [("windows", "100_es")],
            # BuildPlatform.Win: [("windows", "300_es")],
            # BuildPlatform.Win: [("windows", "330")],
            BuildPlatform.Web: [("asm.js", "100_es")],
            BuildPlatform.WebYandex: [("asm.js", "100_es")],
        }

        assert platform in platform_mapping, f"Not supported platform: {platform}"

        output_directory = PROJECT_DIR / "codegen" / "shaders"

        found_shader_names = set()
        all_shaders_by_type = defaultdict(list)

        for base in (
            PROJECT_DIR / "src" / "engine" / "shaders",
            PROJECT_DIR / "src" / "game" / "shaders",
        ):
            for shader_type, shaders in (
                ("vertex", list(base.glob("*_vs.sc"))),
                ("fragment", list(base.glob("*_fs.sc"))),
            ):
                for shader in shaders:
                    assert shader.stem not in found_shader_names, (
                        f"Shader '{shader.stem}' is an engine's shader. Rename it!"
                    )
                    found_shader_names.add(shader.stem)
                    all_shaders_by_type[shader_type].append(shader)

        for shaderc_platform_name, profile in platform_mapping[platform]:
            for shader_type, shaders in all_shaders_by_type.items():
                for shader in shaders:
                    varyingdef = str(shader).rsplit("_", 1)[0] + "_var.def.sc"

                    out_file = output_directory / (shader.stem + f"_{profile}.bin")
                    recursive_mkdir(output_directory)

                    run_command(
                        [
                            SHADERC_PATH,
                            "-f",
                            shader,
                            "-o",
                            out_file,
                            "--type",
                            shader_type,
                            "--platform",
                            shaderc_platform_name,
                            "--profile",
                            profile,
                            "-i",
                            PROJECT_DIR / "vendor" / "bgfx" / "src",
                            "--varyingdef",
                            varyingdef,
                            "--bin2c",
                            "-O3",
                            "--Werror",
                        ]
                    )

    recursive_mkdir(HANDS_GENERATED_DIR)

    with open(
        HANDS_GENERATED_DIR / "bf_codegen.cpp", "w", encoding="utf-8"
    ) as codegen_file:
        codegen_file.write(
            """// automatically generated by bf_cli.py, do not modify
#pragma once

"""
        )

        def genline(value):
            codegen_file.write(value)
            codegen_file.write("\n")

        generate_flatbuffer_files()

        remove_intermediate_generation_files()

        # TODO: downscale_factors = [1, 2, 4]
        downscale_factors = [2, 1]

        check_no_excessive_images_in_temp_art_dir(downscale_factors)
        downscale_images(downscale_factors)
        downscale_factors.pop()

        textures = [
            {
                "debug_name": f"d1/{filepath.stem}",
                "size": Image.open(TEMP_ART_DIR / "d1" / filepath).size,
            }
            for filepath in Path(TEMP_ART_DIR / "d1").glob("*.png")
        ]
        textures.sort(key=partial(texture_cmp_key, factor=1))
        original_texture_sizes = [x["size"] for x in textures]

        texture_name_2_id, atlases_data = make_atlases(downscale_factors)
        assert len(downscale_factors) == 1
        convert_gamelib_json_to_binary(
            texture_name_2_id, genline, atlases_data[0], original_texture_sizes
        )
        genline("///")
    # }


###
