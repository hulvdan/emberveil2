import json
import logging
import re
import subprocess
import sys
from contextlib import contextmanager
from functools import wraps
from pathlib import Path
from time import time
from typing import NoReturn, TypeAlias, TypeVar

import fnvhash

T: TypeAlias = TypeVar("T")

PIXEL_SCALE_MULTIPLIER = 5

_exiting = False

timings_stack = []
_root_timings_stack = timings_stack
_timing_marks = []
_timing_recursion_depth = 0


def timing(f):
    @wraps(f)
    def wrap(*args, **kw):
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
            log.debug("Running '{}' took: {:.2f} ms".format(f.__name__, elapsed * 1000))

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
CLI_DIR = Path("cli")
SRC_DIR = Path("src")
SHADERS_DIR = SRC_DIR / "shaders"
ASSETS_DIR = SRC_DIR / "assets"
ART_DIR = ASSETS_DIR / "art"
FLATBUFFERS_SRC_DIR = SRC_DIR / "flatbuffers"
RESOURCES_DIR = PROJECT_DIR / "resources"
HANDS_GENERATED_DIR = PROJECT_DIR / "codegen" / "hands"
FLATBUFFERS_GENERATED_DIR = PROJECT_DIR / "codegen" / "flatbuffers"
if 1:
    CMAKE_RELEASE_BUILD_TYPE = "RelWithDebInfo"
else:
    CMAKE_RELEASE_BUILD_TYPE = "Release"
CMAKE_DEBUG_BUILD_DIR = Path(".cmake") / "vs17" / "Debug"
CMAKE_RELEASE_BUILD_DIR = Path(".cmake") / "vs17" / CMAKE_RELEASE_BUILD_TYPE
CMAKE_TESTS_PATH = Path(".cmake") / "vs17" / "Debug" / "tests.exe"

CLANG_FORMAT_PATH = "C:/Program Files/LLVM/bin/clang-format.exe"
CLANG_TIDY_PATH = "C:/Program Files/LLVM/bin/clang-tidy.exe"
CPPCHECK_PATH = "C:/Program Files/Cppcheck/cppcheck.exe"
FLATC_PATH = CLI_DIR / "flatc.exe"

MSBUILD_PATH = r"c:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"


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


def run_command(cmd: list[str] | str, stdin_input: str | None = None, cwd=None) -> None:
    if isinstance(cmd, str):
        cmd = replace_double_spaces(cmd.replace("\n", " ").strip())

    c = cmd
    if not isinstance(c, str):
        c = " ".join(str(i) for i in cmd)

    log.debug(f"Executing command: {c}")

    p = subprocess.run(  # noqa: PLW1510
        cmd,
        shell=True,
        stdout=sys.stdout,
        stderr=sys.stderr,
        text=True,
        encoding="utf-8",
        input=stdin_input,
        cwd=cwd,
    )

    if p.returncode:
        log.critical(f'Failed to execute: "{c}"')
        exit(p.returncode)


def recursive_mkdir(path: Path) -> None:
    for parent in list(path.parents)[::-1][1:]:
        parent.mkdir(exist_ok=True)


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


def batched(list_: list[T], n: int) -> list[T]:
    for i in range(0, len(list_), n):
        yield list_[i : i + n]
