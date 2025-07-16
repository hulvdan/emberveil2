import os
import shutil
import tempfile
from collections import defaultdict
from math import radians
from pathlib import Path
from typing import Any, Sequence

import pyjson5 as json
import yaml
from bf_game import *  # noqa
from bf_lib import (
    ART_DIR,
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
    BuildPlatform,
    gamelib_processing_functions,
    log,
    recursive_mkdir,
    run_command,
    timing,
    timing_mark,
)
from PIL import Image


def texture_ids_recursive_transform(
    gamelib_recursed, transform_texture_id, transform_texture_ids_list
) -> None:
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


def recursive_replace_transform(
    gamelib_recursed,
    key_postfix_single: str,
    key_postfix_list: str,
    codename_to_index: dict[str, int],
    *,
    root=True,
) -> list[str] | None:
    errors = None

    if not isinstance(gamelib_recursed, dict):
        return None

    for key, value in gamelib_recursed.items():
        if isinstance(value, dict):
            more_errors = recursive_replace_transform(
                value, key_postfix_single, key_postfix_list, codename_to_index, root=False
            )
            if more_errors:
                if not errors:
                    errors = more_errors
                else:
                    errors.extend(more_errors)

        elif isinstance(key, str) and (
            key.endswith((key_postfix_single, key_postfix_list))
        ):
            if key.endswith(key_postfix_list):
                assert isinstance(value, list)
                for i in range(len(value)):
                    assert isinstance(value[i], str), f"value: {value[i]}"
                    v = value[i]
                    try:
                        value[i] = codename_to_index[v]
                    except KeyError:
                        if not errors:
                            errors = []
                        errors.append(v)
            else:
                assert isinstance(value, str), f"value: {value}"
                try:
                    gamelib_recursed[key] = codename_to_index[value]
                except KeyError:
                    if not errors:
                        errors = []
                    errors.append(value)

        elif isinstance(value, list):
            for v in value:
                if isinstance(v, dict):
                    more_errors = recursive_replace_transform(
                        v,
                        key_postfix_single,
                        key_postfix_list,
                        codename_to_index,
                        root=False,
                    )
                    if more_errors:
                        if not errors:
                            errors = more_errors
                        else:
                            errors.extend(more_errors)

    if root and errors:
        message = "recursive_replace_transform({}, {}):\nNot found:\n{}".format(
            key_postfix_single, key_postfix_list, "\n".join(errors)
        )
        raise AssertionError(message)

    return errors


def degrees_to_radians_recursive_transform(gamelib_recursed) -> None:
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


def index_default_minus_1(list_variable, value):
    try:
        return list_variable.index(value)
    except ValueError:
        return -1


def genenum(
    genline,
    name: str,
    values: Sequence[str],
    *,
    enum_type: str | None = None,
    add_count: bool = False,
    hex_values: bool = False,
    override_values: Sequence[Any] | None = None,
    enumerate_values: bool = False,
    add_to_string: bool = False,
    comments: list[str] | None = None,
) -> None:
    assert not (hex_values and enumerate_values)
    assert not (override_values and enumerate_values)

    if add_count or hex_values:
        assert add_count != hex_values

    string = f"enum {name}"
    if enum_type:
        string += f" : {enum_type}"
    string += " {"
    genline(string)

    def genline_with_comment(line: str, i: int) -> None:
        if comments and comments[i]:
            line += "  // " + comments[i]
        genline(line)

    if hex_values:
        for i, value in enumerate(values):
            genline_with_comment("  {}_{} = {},".format(name, value, hex(2**i)), i)
    elif override_values:
        i = 0
        for value, value2 in zip(values, override_values):
            genline_with_comment("  {}_{} = {},".format(name, value, value2), i)
            i += 1
    else:
        for i, value in enumerate(values):
            if enumerate_values:
                genline_with_comment("  {}_{} = {},".format(name, value, i), i)
            else:
                genline_with_comment("  {}_{},".format(name, value), i)

    if add_count:
        genline("  {}_COUNT,".format(name))

    genline("};\n")

    if add_to_string:
        genline(f"const char* {name}ToString({name} type) {{")
        genline("  ASSERT(type >= 0);")
        if add_count:
            genline(f"  ASSERT(type <= {len(values)});")
        else:
            genline(f"  ASSERT(type < {len(values)});")
        genline("  static constexpr const char* strings[] = {")
        for value in values:
            genline(f'    "{name}_{value}",')
        if add_count:
            genline(f'    "{name}_COUNT",')
        genline("  };")
        genline("  return strings[type];")
        genline("};\n")


@timing
def convert_gamelib_json_to_binary(
    texture_name_2_id: dict[str, int], genline, atlas_data
) -> None:
    gamelib = (
        yaml.safe_load((GAME_DIR / "gamelib.yaml").read_text(encoding="utf-8")) or {}
    )

    gamelib |= atlas_data

    for gamelib_processing_function in gamelib_processing_functions:
        gamelib_processing_function(genline, gamelib)

    transform_texture_id = lambda data, key: transform_to_texture_index(
        data, key, texture_name_2_index=texture_name_2_id
    )
    transform_texture_ids_list = lambda data, key: transform_to_texture_indexes_list(
        data, key, texture_name_2_index=texture_name_2_id
    )
    texture_ids_recursive_transform(
        gamelib, transform_texture_id, transform_texture_ids_list
    )
    degrees_to_radians_recursive_transform(gamelib)

    # Создание `gamelib.bin`.
    intermediate_path = TEMP_DIR / "gamelib.intermediate.jsonc"
    intermediate_path.write_text(json.dumps(gamelib))
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


@timing
def make_atlas(path: Path) -> tuple[dict[str, int], dict]:
    assert str(path).endswith(".ftpp")

    copy_from_base_dir = ART_DIR / "textures"
    for filepath in copy_from_base_dir.rglob("*.png"):
        s1 = filepath.stat()
        to_path = TEMP_ART_DIR / filepath.relative_to(copy_from_base_dir)

        if to_path.exists():
            s2 = to_path.stat()
            if s1.st_mtime_ns == s2.st_mtime_ns:
                continue

        recursive_mkdir(to_path.parent)

        shutil.copy(filepath, to_path)
        os.utime(to_path, ns=(s1.st_atime_ns, s1.st_mtime_ns))

    cache_filepath = TEMP_DIR / ".atlas.cache"

    old_cache_value = -1
    if cache_filepath.exists():
        old_cache_value = int(cache_filepath.read_text())

    cache_value = 0
    for filepath in Path(TEMP_ART_DIR).rglob("*.png"):
        cache_value = hash(cache_value + filepath.stat().st_mtime_ns)

    should_regenerate_atlas = False
    temp_atlas_path = TEMP_DIR / (path.stem + ".png")

    if (cache_value == old_cache_value) and temp_atlas_path.exists():
        log.info("Skipped atlas generation - no images changed")
        timing_mark("skipped png generation")
    else:
        # Генерируем атлас из .ftpp файла. Создаются .json спецификация и .png текстура.
        log.info("Generating atlas...")
        run_command("free-tex-packer-cli --project {}".format(path))

        cache_filepath.write_text(str(cache_value))

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

    texture_name_2_id = {}
    textures.sort(key=lambda x: (x["debug_name"] != "undefined", x["debug_name"]))

    for i, texture in enumerate(textures):
        texture["id"] = i
        name = texture["debug_name"]
        texture_name_2_id[name] = i

    recursive_mkdir(RESOURCES_DIR)

    out_atlas_path = RESOURCES_DIR / (path.stem + ".png")
    if not out_atlas_path.exists() or should_regenerate_atlas:
        # Compressing and saving 'atlas.png' to 'resources/'.
        picture = Image.open(temp_atlas_path)
        picture.save(out_atlas_path, optimize=True)
    else:
        timing_mark("skipped optimized copying")

    return texture_name_2_id, {
        "atlas_textures": textures,
        "atlas_size_x": json_data["meta"]["size"]["w"],
        "atlas_size_y": json_data["meta"]["size"]["h"],
    }


@timing
def check_no_excessive_images_in_temp_art_dir() -> None:
    temp_filepaths = [
        filepath.relative_to(TEMP_ART_DIR) for filepath in TEMP_ART_DIR.rglob("*.png")
    ]
    art_filepaths = [
        filepath.relative_to(ART_DIR / "textures")
        for filepath in (ART_DIR / "textures").rglob("*.png")
    ]

    for temp_filepath in temp_filepaths:
        if temp_filepath not in art_filepaths:
            p = TEMP_ART_DIR / temp_filepath
            log.info("Removing excessive image '{}'...".format(p))
            p.unlink()


@timing
def remove_intermediate_generation_files() -> None:
    for filepath in TEMP_DIR.rglob("*.intermediate*"):
        filepath.unlink()


def transform_to_texture_indexes_list(
    data: dict[str, Any],
    key: str,
    texture_name_2_index: dict[str, int],
) -> None:
    textures = data[key]
    assert isinstance(textures, list)

    for i, texture_name in enumerate(textures):
        if texture_name.lower() == "undefined":
            textures[i] = -1
        else:
            assert texture_name in texture_name_2_index, (
                f"Texture '{texture_name}' not found in atlas!"
            )
            textures[i] = texture_name_2_index[texture_name]


def transform_to_texture_index(
    data: dict[str, Any],
    key: str,
    texture_name_2_index: dict[str, int],
) -> None:
    texture_name = data[key]
    assert isinstance(texture_name, str)

    if texture_name.lower() == "undefined":
        data[key] = -1
        return

    assert texture_name in texture_name_2_index, (
        f"Texture '{texture_name}' not found in atlas!"
    )

    data[key] = texture_name_2_index[texture_name]


def listfiles_with_hashes_in_dir(path: str | Path) -> dict[str, int]:
    filepath_string_to_hash = {}

    for filepath in Path(path).rglob("*"):
        with open(filepath, "rb") as in_file:
            filepath_string_to_hash[str(filepath.relative_to(path))] = hash(
                in_file.read()
            )

    return filepath_string_to_hash


@timing
def generate_flatbuffer_files():
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

        to_reflect = "bf_boner.fbs"
        gen(
            (i for i in flatbuffer_files if i.name == to_reflect),
            "--gen-object-api",
            "--reflect-names",
        )
        gen(i for i in flatbuffer_files if i.name != to_reflect)

        for file, file_hash in listfiles_with_hashes_in_dir(td).items():
            # Костыль, чтобы MSBuild не ребилдился каждый раз.
            if file_hash != hashes_for_msbuild.get(file):
                shutil.copyfile(Path(td) / file, FLATBUFFERS_GENERATED_DIR / file)


@timing
def do_generate() -> None:
    for platform in BuildPlatform:
        platform_mapping = {
            # BuildPlatform.Win: [("windows", "s_5_0")],
            BuildPlatform.Win: [("windows", "s_4_0")],
            # BuildPlatform.Win: [("windows", "300_es")],
            # BuildPlatform.Win: [("windows", "330")],
            BuildPlatform.Web: [("asm.js", "100_es")],
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
            """// automatically generated by cli.py, do not modify
#pragma once

"""
        )

        def genline(value):
            codegen_file.write(value)
            codegen_file.write("\n")

        generate_flatbuffer_files()

        remove_intermediate_generation_files()

        check_no_excessive_images_in_temp_art_dir()

        texture_name_2_id, atlas_data = make_atlas(ART_DIR / "atlas.ftpp")

        convert_gamelib_json_to_binary(texture_name_2_id, genline, atlas_data)
