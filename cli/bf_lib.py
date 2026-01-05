# Imports.  {  ###
import colorsys
import csv
import hashlib
import re
import socket
import subprocess
import sys
from collections import defaultdict
from contextlib import contextmanager
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from pathlib import Path
from typing import Any, Iterator, Sequence, TypeVar

import fnvhash
import pyfiglet
import yaml
from bf_typer import log

# }

T = TypeVar("T")


@dataclass(slots=True)
class _GameSettings:
    # {  ###
    itch_target: str = "hulvdan/game-template"
    languages: list[str] = field(default_factory=lambda: ["russian", "english"])
    generate_flatbuffers_api_for: list[str] = field(default_factory=list)
    yandex_metrica_counter_id: int | None = None
    # }


game_settings = _GameSettings()


gamelib_processing_functions = []


def gamelib_processor(func):
    gamelib_processing_functions.append(func)
    return func


_gamelib = None


def load_gamelib_cached() -> dict:
    # {  ###
    global _gamelib
    if _gamelib is not None:
        return _gamelib
    _gamelib = yaml.safe_load((GAME_DIR / "gamelib.yaml").read_text(encoding="utf-8"))
    return _gamelib  # type: ignore[return-value]
    # }


class StrEnum(str, Enum):
    def __str__(self):
        return self.value


class BuildType(StrEnum):
    Debug = "Debug"
    RelWithDebInfo = "RelWithDebInfo"
    Release = "Release"


class BuildPlatform(StrEnum):
    Win = "Win"
    Web = "Web"
    WebYandex = "WebYandex"
    WebItch = "WebItch"

    def is_web(self) -> bool:
        return self.lower().startswith("web")


class BuildTarget(StrEnum):
    game = "game"
    tests = "tests"


ALLOWED_BUILDS = (  ###
    (BuildTarget.game, BuildPlatform.Win, BuildType.Debug),
    (BuildTarget.game, BuildPlatform.Win, BuildType.RelWithDebInfo),
    (BuildTarget.game, BuildPlatform.Win, BuildType.Release),
    (BuildTarget.game, BuildPlatform.Web, BuildType.Debug),
    (BuildTarget.game, BuildPlatform.Web, BuildType.Release),
    (BuildTarget.game, BuildPlatform.WebItch, BuildType.Release),
    (BuildTarget.game, BuildPlatform.WebYandex, BuildType.Release),
    (BuildTarget.tests, BuildPlatform.Win, BuildType.Debug),
)


REPLACING_SPACES_PATTERN = re.compile(r"\ +")
REPLACING_NEWLINES_PATTERN = re.compile(r"\n+")


# !banner: constants
#  ██████╗ ██████╗ ███╗   ██╗███████╗████████╗ █████╗ ███╗   ██╗████████╗███████╗
# ██╔════╝██╔═══██╗████╗  ██║██╔════╝╚══██╔══╝██╔══██╗████╗  ██║╚══██╔══╝██╔════╝
# ██║     ██║   ██║██╔██╗ ██║███████╗   ██║   ███████║██╔██╗ ██║   ██║   ███████╗
# ██║     ██║   ██║██║╚██╗██║╚════██║   ██║   ██╔══██║██║╚██╗██║   ██║   ╚════██║
# ╚██████╗╚██████╔╝██║ ╚████║███████║   ██║   ██║  ██║██║ ╚████║   ██║   ███████║
#  ╚═════╝ ╚═════╝ ╚═╝  ╚═══╝╚══════╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═══╝   ╚═╝   ╚══════╝

PROJECT_DIR = Path(__file__).parent.parent
TEMP_DIR = PROJECT_DIR / ".temp"
TEMP_ART_DIR = TEMP_DIR / "art"
CLI_DIR = Path("cli")
ASSETS_DIR = PROJECT_DIR / "assets"
ART_DIR = ASSETS_DIR / "art"
ART_TEXTURES_DIR = ART_DIR / "textures"
SRC_DIR = Path("src")
RES_DIR = PROJECT_DIR / "res"
VENDOR_DIR = PROJECT_DIR / "vendor"
GAME_DIR = PROJECT_DIR / "src" / "game"
HANDS_GENERATED_DIR = PROJECT_DIR / "codegen" / "hands"
FLATBUFFERS_GENERATED_DIR = PROJECT_DIR / "codegen" / "flatbuffers"
CMAKE_TESTS_PATH = Path(".cmake") / "vs17" / "Debug" / "tests.exe"

CLANG_FORMAT_PATH = "C:/Program Files/LLVM/bin/clang-format.exe"
CLANG_TIDY_PATH = "C:/Program Files/LLVM/bin/clang-tidy.exe"
CPPCHECK_PATH = "C:/Program Files/Cppcheck/cppcheck.exe"
FLATC_PATH = CLI_DIR / "flatc.exe"
SHADERC_PATH = str(PROJECT_DIR / "vendor/bgfx/.build/win64_vs2022/bin/shadercRelease.exe")
BUTLER_PATH = "C:/Users/user/Programs/butler/butler.exe"

MSBUILD_PATH = r"c:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
CLANG_CL_PATH = r"c:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\Llvm\x64\bin\clang-cl.exe"

AUDIO_EXTENSIONS = {".wav", ".mp3", ".flac", ".aac", ".m4a", ".wma", ".ogg"}


# !banner: utils
# ██╗   ██╗████████╗██╗██╗     ███████╗
# ██║   ██║╚══██╔══╝██║██║     ██╔════╝
# ██║   ██║   ██║   ██║██║     ███████╗
# ██║   ██║   ██║   ██║██║     ╚════██║
# ╚██████╔╝   ██║   ██║███████╗███████║
#  ╚═════╝    ╚═╝   ╚═╝╚══════╝╚══════╝


def replace_double_spaces(string: str) -> str:
    return re.sub(REPLACING_SPACES_PATTERN, " ", string)


def replace_double_newlines(string: str) -> str:
    return re.sub(REPLACING_NEWLINES_PATTERN, "\n", string)


def test_replace_double_spaces():
    # {  ###
    assert replace_double_spaces("") == ""
    assert replace_double_spaces(" ") == " "
    assert replace_double_spaces("  ") == " "
    assert replace_double_spaces("   ") == " "
    assert replace_double_spaces("\n") == "\n"
    assert replace_double_spaces("\n\n") == "\n\n"
    assert replace_double_spaces("\n\n\n") == "\n\n\n"
    # }


def test_replace_double_newlines():
    # {  ###
    assert replace_double_newlines("") == ""
    assert replace_double_newlines(" ") == " "
    assert replace_double_newlines("  ") == "  "
    assert replace_double_newlines("   ") == "   "
    assert replace_double_newlines("\n") == "\n"
    assert replace_double_newlines("\n\n") == "\n"
    assert replace_double_newlines("\n\n\n") == "\n"
    # }


def remove_spaces(string: str) -> str:
    return re.sub(REPLACING_SPACES_PATTERN, "", string)


def run_command(
    cmd: list[str | Path] | str,
    stdin_input: str | None = None,
    cwd=None,
    timeout_seconds: int | None = None,
) -> None:
    # {  ###
    if isinstance(cmd, str):
        cmd = replace_double_spaces(cmd.replace("\n", " ").strip())

    c = cmd
    if not isinstance(c, str):
        c = " ".join(str(i) for i in cmd)

    log.info(f"Executing command: {c}")

    p = subprocess.run(  # noqa: PLW1510
        cmd,
        shell=True,
        stdout=sys.stdout,
        stderr=sys.stderr,
        text=True,
        encoding="utf-8",
        input=stdin_input,
        cwd=cwd,
        timeout=timeout_seconds,
    )

    if p.returncode:
        log.critical(f'Failed to execute: "{c}"')
        exit(p.returncode)
    # }


def recursive_mkdir(path: Path | str) -> None:
    Path(path).mkdir(parents=True, exist_ok=True)


def batched(list_: list[T], n: int) -> Iterator[list[T]]:
    for i in range(0, len(list_), n):
        yield list_[i : i + n]


def check_duplicates(values: list) -> None:
    # {  ###
    for i in range(len(values)):
        for k in range(i + 1, len(values)):
            assert values[i] != values[k], f"Found duplicate value: {values[i]}"
    # }


def only_one_is_not_none(values: list) -> bool:
    # {  ###
    found = False
    for v in values:
        if v:
            if found:
                return False
            found = True
    return found
    # }


def test_only_one_is_not_none() -> None:
    # {  ###
    assert only_one_is_not_none([2, None, None])
    assert only_one_is_not_none([2])
    assert not only_one_is_not_none([])
    assert not only_one_is_not_none([1, 2])
    # }


def all_are_not_none(values: Iterator) -> bool:
    return all(v is not None for v in values)


def all_are_none(values: Iterator) -> bool:
    return all(v is None for v in values)


def get_local_ip() -> str:
    ip_address = socket.gethostbyname(socket.gethostname())
    assert ip_address != "127.0.0.1"
    return ip_address


# !banner: codegen
#  ██████╗ ██████╗ ██████╗ ███████╗ ██████╗ ███████╗███╗   ██╗
# ██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔════╝ ██╔════╝████╗  ██║
# ██║     ██║   ██║██║  ██║█████╗  ██║  ███╗█████╗  ██╔██╗ ██║
# ██║     ██║   ██║██║  ██║██╔══╝  ██║   ██║██╔══╝  ██║╚██╗██║
# ╚██████╗╚██████╔╝██████╔╝███████╗╚██████╔╝███████╗██║ ╚████║
#  ╚═════╝ ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚══════╝╚═╝  ╚═══╝


def generate_binary_file_header(genline, source_path: Path, variable_name: str) -> None:
    # {  ###
    data = source_path.read_bytes()
    genline(f"const u8 {variable_name}[] = {{")
    for i in range(0, len(data), 12):
        chunk = ", ".join(f"0x{b:02x}" for b in data[i : i + 12])
        genline(f"    {chunk},")
    genline("};\n")
    # }


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
    # {  ###
    assert not (hex_values and enumerate_values)
    assert not (override_values and enumerate_values)

    if add_count or hex_values:
        assert add_count != hex_values

    string = f"enum {name}"
    if enum_type:
        string += f" : {enum_type}"
    string += " {  ///"
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
        for value, value2 in zip(values, override_values, strict=True):
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
    # }


_call_stack: list[str | int] = []


_recursive_replace_transform_patterns: Any = None


def recursive_replace_transform(
    gamelib_recursed,
    key_postfix_single: str,
    key_postfix_list: str,
    codename_to_index: dict[str, int],
    *,
    root: bool = True,
) -> list[str] | None:
    # {  ###
    global _recursive_replace_transform_patterns
    errors = None

    if not isinstance(gamelib_recursed, dict):
        return None

    if root:
        _recursive_replace_transform_patterns = (
            re.compile(f"(.*_)?{key_postfix_single}(_\\d+)?$"),
            re.compile(f"(.*_)?{key_postfix_list}(_\\d+)?$"),
        )

    for key, value in gamelib_recursed.items():
        _call_stack.append(key)

        added_type = False

        if isinstance(value, dict):
            if "type" in value:
                _call_stack.append("(type={})".format(value["type"]))
                added_type = True

            more_errors = recursive_replace_transform(
                value, key_postfix_single, key_postfix_list, codename_to_index, root=False
            )
            if more_errors:
                if not errors:
                    errors = more_errors
                else:
                    errors.extend(more_errors)

        elif isinstance(key, str) and (
            re.match(_recursive_replace_transform_patterns[0], key)
            or re.match(_recursive_replace_transform_patterns[1], key)
        ):
            if re.match(_recursive_replace_transform_patterns[1], key):
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
                assert isinstance(value, str), "key: {}, value: {}, stack: {}".format(
                    key, value, _call_stack
                )
                try:
                    gamelib_recursed[key] = codename_to_index[value]
                except KeyError:
                    if not errors:
                        errors = []
                    errors.append(value)

        elif isinstance(value, list):
            for i, v in enumerate(value):
                if not isinstance(v, dict):
                    continue

                _call_stack.append(i)

                added_type2 = False
                if "type" in v:
                    _call_stack.append("(type={})".format(v["type"]))
                    added_type2 = True

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

                if added_type2:
                    _call_stack.pop()

                _call_stack.pop()

        if added_type:
            _call_stack.pop()
        _call_stack.pop()

    if root:
        _recursive_replace_transform_patterns = None

    if root and errors:
        message = "recursive_replace_transform({}, {}):\nNot found:\n{}".format(
            key_postfix_single, key_postfix_list, "\n".join(sorted(set(errors)))
        )
        raise AssertionError(message)

    return errors
    # }


def recursive_flattenizer(
    gamelib_recursed,
    key_postfix_single: str,
    key_postfix_list: str,
    root_list_field: str,
    *,
    list_to_fill: list | None = None,
) -> None:
    # {  ###
    if not isinstance(gamelib_recursed, dict):
        return

    should_emplace_list_in_gamelib = False
    if list_to_fill is None:
        assert root_list_field not in gamelib_recursed
        should_emplace_list_in_gamelib = True
        list_to_fill = [{}]

    for key, value in gamelib_recursed.items():
        if isinstance(key, str) and (
            key.endswith((key_postfix_single, key_postfix_list))
        ):
            if key.endswith(key_postfix_list):
                assert isinstance(value, list)
                start = len(list_to_fill)
                list_to_fill.extend(value)
                gamelib_recursed[key] = {"start": start, "end": len(list_to_fill)}
            else:
                list_to_fill.append(gamelib_recursed[key])
                gamelib_recursed[key] = len(list_to_fill) - 1

        elif isinstance(value, dict):
            recursive_flattenizer(
                value,
                key_postfix_single,
                key_postfix_list,
                root_list_field,
                list_to_fill=list_to_fill,
            )

        elif isinstance(value, list):
            for v in value:
                if not isinstance(v, dict):
                    continue

                recursive_flattenizer(
                    v,
                    key_postfix_single,
                    key_postfix_list,
                    root_list_field,
                    list_to_fill=list_to_fill,
                )

    if should_emplace_list_in_gamelib:
        gamelib_recursed[root_list_field] = list_to_fill
    # }


# !banner: git
#  ██████╗ ██╗████████╗
# ██╔════╝ ██║╚══██╔══╝
# ██║  ███╗██║   ██║
# ██║   ██║██║   ██║
# ╚██████╔╝██║   ██║
#  ╚═════╝ ╚═╝   ╚═╝


def git_check_no_unstashed() -> None:
    process = subprocess.run(
        "git status --porcelain", check=True, shell=True, capture_output=True, text=True
    )
    git_status_text = process.stdout.strip()
    assert not git_status_text, "You have unstashed changes! Won't proceed!"


@contextmanager
def git_stash():
    # {  ###
    process = subprocess.run(
        "git status --porcelain", check=True, shell=True, capture_output=True, text=True
    )
    git_status_text = process.stdout.strip()
    should_stash = bool(git_status_text)

    if should_stash:
        log.info("git_stash: stashing changes...")
        stash_message = datetime.now().strftime("%Y%m%d-%H%M%S template-update autostash")
        subprocess.run(f'git stash push -u -m "{stash_message}"', check=True, shell=True)
    else:
        log.info("git_stash: no changes - not stashing")

    try:
        yield

    finally:
        if should_stash:
            log.info("git_stash: applying previously stashed changes...")
            subprocess.run("git stash apply", check=True, shell=True)
    # }


def _git_get_current_commit_version_tag() -> str | None:
    # {  ###
    process = subprocess.run(
        'git tag -l "v1\\.*" --points-at HEAD',
        check=True,
        shell=True,
        capture_output=True,
        text=True,
    )
    return process.stdout.strip()
    # }


def _git_get_current_branch() -> str:
    # {  ###
    return subprocess.run(
        "git branch --show-current",
        check=True,
        shell=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    # }


def git_bump_tag() -> str:
    # {  ###
    assert _git_get_current_branch() in ("master", "main")

    if version := _git_get_current_commit_version_tag():
        log.info("Skipping bumping tag")
        return version

    version_tags = subprocess.run(
        'git tag -l "v1\\.*"', check=True, shell=True, capture_output=True, text=True
    ).stdout.strip()

    next_version = 0
    if version_tags:
        next_version = max(int(t.split(".")[-1]) for t in version_tags.split("\n")) + 1

    (
        SRC_DIR / "bf_version.cpp"
    ).write_text(f"""// automatically generated by bf_cli.py, do not modify
#pragma once

#define BF_VERSION "v1.{next_version}"
""")

    run_command("git reset")
    run_command("git add src/bf_version.cpp")
    run_command(f'git commit -m "Release v1.{next_version}"')
    run_command(f"git tag v1.{next_version}")
    # run_command("git push")
    # run_command(f"git push origin v1.{next_version}")
    return f"v1.{next_version}"
    # }


# !banner: color
#  ██████╗ ██████╗ ██╗      ██████╗ ██████╗
# ██╔════╝██╔═══██╗██║     ██╔═══██╗██╔══██╗
# ██║     ██║   ██║██║     ██║   ██║██████╔╝
# ██║     ██║   ██║██║     ██║   ██║██╔══██╗
# ╚██████╗╚██████╔╝███████╗╚██████╔╝██║  ██║
#  ╚═════╝ ╚═════╝ ╚══════╝ ╚═════╝ ╚═╝  ╚═╝


def hex_to_rgb_ints(hex_color: str) -> tuple[int, int, int]:
    # {  ###
    hex_color = hex_color.lstrip("#")
    r = int(hex_color[0:2], 16)
    g = int(hex_color[2:4], 16)
    b = int(hex_color[4:6], 16)
    return (r, g, b)
    # }


def hex_to_rgb_floats(hex_color: str) -> tuple[float, float, float]:
    # {  ###
    hex_color = hex_color.lstrip("#")
    r = int(hex_color[0:2], 16)
    g = int(hex_color[2:4], 16)
    b = int(hex_color[4:6], 16)
    return (r / 255, g / 255, b / 255)
    # }


def rgb_floats_to_hex(rgb_floats: tuple[float, float, float]) -> str:
    # {  ###
    r, g, b = rgb_floats
    r_int = round(r * 255)
    g_int = round(g * 255)
    b_int = round(b * 255)
    return "#{:02X}{:02X}{:02X}".format(r_int, g_int, b_int)
    # }


def transform_color(
    rgb: tuple[float, float, float],
    *,
    saturation_scale: float = 1,
    value_scale: float = 1.0,
) -> tuple[float, float, float]:
    # {  ###
    assert saturation_scale >= 0
    assert value_scale >= 0
    r, g, b = rgb
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    s = min(s * saturation_scale, 1.0)
    v = min(v * value_scale, 1.0)
    return colorsys.hsv_to_rgb(h, s, v)
    # }


# !banner: hashing
# ██╗  ██╗ █████╗ ███████╗██╗  ██╗██╗███╗   ██╗ ██████╗
# ██║  ██║██╔══██╗██╔════╝██║  ██║██║████╗  ██║██╔════╝
# ███████║███████║███████╗███████║██║██╔██╗ ██║██║  ███╗
# ██╔══██║██╔══██║╚════██║██╔══██║██║██║╚██╗██║██║   ██║
# ██║  ██║██║  ██║███████║██║  ██║██║██║ ╚████║╚██████╔╝
# ╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═══╝ ╚═════╝


def stable_hash(value: str | int) -> int:
    # {  ###
    if isinstance(value, int):
        value = str(value)
    if isinstance(value, str):
        return int(hashlib.md5(value.encode("utf-8")).hexdigest(), 16)
    assert False, "Not supported type of value"
    # }


def hash32(value: str) -> int:
    return fnvhash.fnv1a_32(value.encode(encoding="ascii"))


def hash32_utf8(value: str) -> int:
    return fnvhash.fnv1a_32(value.encode(encoding="utf-8"))


def hash32_file_utf8(filepath) -> int:
    with open(filepath, encoding="utf-8") as in_file:
        d = in_file.read()
    return hash32_utf8(d)


# !banner: banners
# ██████╗  █████╗ ███╗   ██╗███╗   ██╗███████╗██████╗ ███████╗
# ██╔══██╗██╔══██╗████╗  ██║████╗  ██║██╔════╝██╔══██╗██╔════╝
# ██████╔╝███████║██╔██╗ ██║██╔██╗ ██║█████╗  ██████╔╝███████╗
# ██╔══██╗██╔══██║██║╚██╗██║██║╚██╗██║██╔══╝  ██╔══██╗╚════██║
# ██████╔╝██║  ██║██║ ╚████║██║ ╚████║███████╗██║  ██║███████║
# ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═══╝╚══════╝╚═╝  ╚═╝╚══════╝

# {  ###
BANNERIFY_PATTERN = "!" + "banner: "


def bannerify(lines: list[str]) -> str:
    out = ""
    bannering_prefix = ""
    current_line_index = 0
    off = 0
    while current_line_index + off < len(lines):
        line = lines[current_line_index + off]

        if BANNERIFY_PATTERN in line:
            if bannering_prefix:
                print("Found line inside bannering")
                exit(1)

            if line.count(BANNERIFY_PATTERN) > 1:
                print("Found line with more than 1 pattern inside")
                exit(1)

            bannering_prefix = line[: line.find(BANNERIFY_PATTERN)]
            if not bannering_prefix:
                print("Found line with no prefix")
                exit(1)

            if not line.removeprefix(bannering_prefix + BANNERIFY_PATTERN).strip():
                print("Found line that has prefix but no content to bannerify")
                exit(1)

            for i in range(current_line_index + off + 1, len(lines)):
                if lines[i].startswith(bannering_prefix):
                    off += 1
                else:
                    out += lines[i]
                    break

            out += line
            out += "\n"
            out += "\n".join(
                bannering_prefix + x.rstrip()
                for x in pyfiglet.figlet_format(
                    line.removeprefix(bannering_prefix + BANNERIFY_PATTERN),
                    font="ansi_shadow",
                    width=90,
                ).splitlines()
                if x.strip()
            )
            out += "\n"
            bannering_prefix = ""

        else:
            out += line + "\n"

        current_line_index += 1

    return out


def test_bannerify():
    assert bannerify([]) == ""
    assert bannerify(["a"]) == "a\n"
    assert bannerify(["a", " b"]) == "a\n b\n"

    got = bannerify(
        [
            f"// {BANNERIFY_PATTERN}a",
            "// ASDASAD",
            "",
            f"// {BANNERIFY_PATTERN}b",
            "",
            "a",
            "b",
            "c",
            "",
            "",
            "d",
        ]
    )
    expected = "".join(
        x.rstrip() + "\n"
        for x in (
            f"// {BANNERIFY_PATTERN}a",
            "//  █████╗",
            "// ██╔══██╗",
            "// ███████║",
            "// ██╔══██║",
            "// ██║  ██║",
            "// ╚═╝  ╚═╝",
            "",
            f"// {BANNERIFY_PATTERN}b",
            "// ██████╗",
            "// ██╔══██╗",
            "// ██████╔╝",
            "// ██╔══██╗",
            "// ██████╔╝",
            "// ╚═════╝",
            "",
            "a",
            "b",
            "c",
            "",
            "",
            "d",
        )
    )
    if got != expected:
        print("EXPECTED:")
        print(expected)
        print("\nGOT:")
        print(got)
    assert got == expected


# }


@dataclass
class LocalizationResult:
    loc_ids: list[str] = field(default_factory=list)
    loc_by_languages: dict[str, list[str]] = field(
        default_factory=lambda: defaultdict(list)
    )


def read_localization_csv() -> LocalizationResult:
    # {  ###
    result = LocalizationResult()

    with open(ASSETS_DIR / "localization.csv", encoding="utf-8") as in_file:
        not_language_columns = ("id", "\ufeffid", "comment")
        for row in csv.DictReader(in_file, delimiter=";"):
            row_id = row.get("id") or row.get("\ufeffid")
            assert isinstance(row_id, str)
            result.loc_ids.append(row_id)
            for c in row:
                if c not in not_language_columns:
                    assert c in game_settings.languages
                    translation = row[c]
                    result.loc_by_languages[c].append(
                        translation.strip() or "<<NOT_TRANSLATED>>"
                    )

    return result
    # }


from bf_game import *  # noqa


###
