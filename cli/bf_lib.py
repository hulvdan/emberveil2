import json
import logging
import re
import subprocess
import sys
from contextlib import contextmanager
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from functools import wraps
from pathlib import Path
from time import time
from typing import Any, Callable, Iterator, NoReturn, ParamSpec, Sequence, TypeVar

import fnvhash


@dataclass(slots=True)
class _DataValues:
    itch_target: str = "hulvdan/game-template"
    languages: list[str] = field(default_factory=lambda: ["russian", "english"])


data_values = _DataValues()

gamelib_processing_functions = []


def gamelib_processor(func):
    gamelib_processing_functions.append(func)
    return func


from bf_game import *  # noqa


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


class BuildTarget(StrEnum):
    game = "game"
    tests = "tests"


P = ParamSpec("P")
T = TypeVar("T")

_exiting = False

timings_stack: list[Any] = []
_root_timings_stack = timings_stack
_timing_marks: list[Any] = []
_timing_recursion_depth = 0


def timing(f: Callable[P, T]) -> Callable[P, T]:
    @wraps(f)
    def wrap(*args: P.args, **kw: P.kwargs) -> T:
        global timings_stack
        global _timing_marks
        global _timing_recursion_depth

        started_at = time()

        old_stack = timings_stack
        timings_stack = []

        old_timing_marks = _timing_marks
        _timing_marks = []

        _timing_recursion_depth += 1

        try:
            result = f(*args, **kw)
        finally:
            _timing_recursion_depth -= 1

            elapsed = time() - started_at
            log.info("Running '{}' took: {:.2f} ms".format(f.__name__, elapsed * 1000))

            old_stack.append((f.__name__, elapsed, timings_stack, _timing_marks))

            timings_stack = old_stack
            _timing_marks = old_timing_marks

            if _exiting:
                print_timings()

        return result

    return wrap


def timing_mark(text):
    _timing_marks.append(text)


def print_timings():
    total_elapsed = sum(i[1] for i in _root_timings_stack)
    if total_elapsed == 0:
        total_elapsed = 0.000001

    timings_string = "Timings:\n"

    def process_value(i, depth):
        nonlocal timings_string

        function_name = i[0]
        elapsed = i[1]
        nested_function_calls = i[2]
        timing_marks_list = i[3]
        timing_marks_joined = ", ".join(timing_marks_list)

        timings_string += "{}- {}".format("  " * depth, function_name).ljust(
            52
        ) + " {: 9.2f} ms, {:4.1f}%{}\n".format(
            elapsed * 1000,
            elapsed * 100 / total_elapsed,
            " ({})".format(timing_marks_joined) if timing_marks_list else "",
        )

        for v in nested_function_calls:
            process_value(v, depth + 1)

    for i in _root_timings_stack:
        process_value(i, 0)

    log.info(timings_string)

    log.info("RUNNING TOOK: {:.2f} SEC".format(time() - _started_at))


_started_at = None


@contextmanager
def timing_manager():
    global _exiting
    global _started_at

    _started_at = time()

    yield

    _exiting = True

    if _timing_recursion_depth == 0:
        print_timings()


global_timing_manager_instance = timing_manager()


old_exit = exit


def timed_exit(code: int) -> NoReturn:
    if global_timing_manager_instance is not None:
        global_timing_manager_instance.__exit__(None, None, None)
        console_handler.flush()

    old_exit(code)


globals()["exit"] = timed_exit


REPLACING_SPACES_PATTERN = re.compile(r"\s+")


LOG_FILE_POSITION = False


# -----------------------------------------------------------------------------------
# Constants.
# -----------------------------------------------------------------------------------

PROJECT_DIR = Path(__file__).parent.parent
TEMP_DIR = PROJECT_DIR / ".temp"
TEMP_ART_DIR = TEMP_DIR / "art"
CLI_DIR = Path("cli")
SRC_DIR = Path("src")
ASSETS_DIR = PROJECT_DIR / "assets"
ART_DIR = ASSETS_DIR / "art"
RESOURCES_DIR = PROJECT_DIR / "resources"
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


# -----------------------------------------------------------------------------------
# Logging.
# -----------------------------------------------------------------------------------


class CustomLoggingFormatter(logging.Formatter):
    _grey = "\x1b[30;20m"
    _green = "\x1b[32;20m"
    _yellow = "\x1b[33;20m"
    _red = "\x1b[31;20m"
    _bold_red = "\x1b[31;1m"

    @staticmethod
    def _get_format(color: str | None) -> str:
        reset = "\x1b[0m"
        if color is None:
            color = reset

        suffix = ""
        if LOG_FILE_POSITION:
            suffix = " (%(filename)s:%(lineno)d)"

        return f"{color}[%(levelname)s] %(message)s{suffix}{reset}"

    _FORMATS = {
        logging.NOTSET: _get_format(None),
        logging.DEBUG: _get_format(None),
        logging.INFO: _get_format(_green),
        logging.WARNING: _get_format(_yellow),
        logging.ERROR: _get_format(_red),
        logging.CRITICAL: _get_format(_bold_red),
    }

    def format(self, record):
        log_fmt = self._FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt)
        return formatter.format(record)


log = logging.getLogger(__file__)
log.setLevel(logging.DEBUG)
console_handler = logging.StreamHandler()
console_handler.setLevel(logging.DEBUG)
console_handler.setFormatter(CustomLoggingFormatter())
log.addHandler(console_handler)


def replace_double_spaces(string: str) -> str:
    return re.sub(REPLACING_SPACES_PATTERN, " ", string)


def remove_spaces(string: str) -> str:
    return re.sub(REPLACING_SPACES_PATTERN, "", string)


def run_command(
    cmd: list[str | Path] | str,
    stdin_input: str | None = None,
    cwd=None,
    timeout_seconds: int | None = None,
) -> None:
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


def recursive_mkdir(path: Path) -> None:
    parents = list(path.parents)
    parents.insert(0, path)
    count = len(parents)

    for i in range(count):
        parents[-i - 1].mkdir(exist_ok=True)


def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def better_json_dump(data, path):
    with open(path, "w", encoding="utf-8") as out_file:
        json.dump(data, out_file, indent="\t", ensure_ascii=False)


def hash32(value: str) -> int:
    return fnvhash.fnv1a_32(value.encode(encoding="ascii"))


def hash32_utf8(value: str) -> int:
    return fnvhash.fnv1a_32(value.encode(encoding="utf-8"))


def hash32_file_utf8(filepath) -> int:
    with open(filepath, encoding="utf-8") as in_file:
        d = in_file.read()
    return hash32_utf8(d)


def batched(list_: list[T], n: int) -> Iterator[list[T]]:
    for i in range(0, len(list_), n):
        yield list_[i : i + n]


def generate_binary_file_header(genline, source_path: Path, variable_name: str) -> None:
    data = source_path.read_bytes()
    genline(f"const u8 {variable_name}[] = {{")
    for i in range(0, len(data), 12):
        chunk = ", ".join(f"0x{b:02x}" for b in data[i : i + 12])
        genline(f"    {chunk},")
    genline("};\n")


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


# Git.
# ==================================================
@contextmanager
def git_stash():
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

    yield

    if should_stash:
        log.info("git_stash: applying previously stashed changes...")
        subprocess.run("git stash apply", check=True, shell=True)


def _git_get_current_commit_tag() -> str | None:
    process = subprocess.run(
        "git tag --points-at HEAD", check=True, shell=True, capture_output=True, text=True
    )
    return process.stdout.strip()


def _git_get_current_branch() -> str:
    return subprocess.run(
        "git branch --show-current",
        check=True,
        shell=True,
        capture_output=True,
        text=True,
    ).stdout.strip()


def git_bump_tag() -> None:
    assert _git_get_current_branch() == "master"

    if _git_get_current_commit_tag():
        log.info("Skipping bumping tag")
        return

    version_tags = subprocess.run(
        'git tag -l "v1\\.*"', check=True, shell=True, capture_output=True, text=True
    ).stdout.strip()

    next_version = 0
    if version_tags:
        next_version = max(int(t.split(".")[-1]) for t in version_tags.split("\n")) + 1

    (
        SRC_DIR / "bf_version.cpp"
    ).write_text(f"""// automatically generated by cli.py, do not modify
#pragma once

#define BF_VERSION ("v1.{next_version}")
""")

    run_command("git reset")
    run_command("git add src/bf_version.cpp")
    run_command(f'git commit -m "Release v1.{next_version}"')
    run_command("git push --force")

    run_command(f"git tag v1.{next_version}")
    run_command(f"git push origin v1.{next_version}")
