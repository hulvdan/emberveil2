import os

import typer
from bf_lib import (
    CMAKE_RELEASE_BUILD_TYPE,
    MSBUILD_PATH,
    PROJECT_DIR,
    global_timing_manager_instance,
    hash32,
    run_command,
    timed_exit,
    timing,
)

# ASSETS_DIR,
# CLANG_TIDY_PATH,
# CMAKE_RELEASE_BUILD_TYPE,
# CMAKE_TESTS_PATH,
# CPPCHECK_PATH,
# PROJECT_DIR,
# SRC_DIR,
# TEMP_DIR,
# global_timing_manager_instance,
# hash32,
# hash32_file_utf8,
# recursive_mkdir,


def hook_exit():
    global exit
    exit = timed_exit  # noqa: A001


app = typer.Typer(
    callback=hook_exit, result_callback=timed_exit, pretty_exceptions_enable=False
)


@timing
def do_build_game() -> None:
    run_command(
        rf"""
        "{MSBUILD_PATH}" .cmake\vs17\game.sln
        -v:minimal
        -property:WarningLevel=3
        -t:game
        """
    )


@timing
def do_build_game_web() -> None:
    run_command(r"cmake --build .cmake\web -t game")


@timing
def do_cmake_vs_files(*, debug: bool) -> None:
    if debug:
        build_type = "Debug"
    else:
        build_type = CMAKE_RELEASE_BUILD_TYPE

    run_command(
        rf"""
            cmake
            -G "Visual Studio 17 2022"
            -B .cmake\vs17
            -DCMAKE_CONFIGURATION_TYPES={build_type}
            -DBUILD_SHARED_LIBS=OFF
            -DPLATFORM=Win
        """
    )


@timing
def do_cmake_web_files(*, debug: bool):
    if debug:
        build_type = "Debug"
    else:
        build_type = "Release"

    run_command(
        rf"""
            emcmake
            cmake
            -B .cmake\web
            -DCMAKE_BUILD_TYPE={build_type}
            -DPLATFORM=Web
        """
    )


# @timing
# def do_build_tests() -> None:
#     run_command(
#         rf"""
#         "{MSBUILD_PATH}" .cmake\vs17\game.sln
#         -v:minimal
#         -property:WarningLevel=3
#         -t:tests
#         """
#     )
#
#
# @timing
# def do_test() -> None:
#     run_command(str(CMAKE_TESTS_PATH))
#
#
# @timing
# def do_lint() -> None:
#     files_to_lint = [
#         *SRC_DIR.rglob("*.cpp"),
#         *SRC_DIR.rglob("*.h"),
#     ]
#     run_command(["poetry", "run", "cpplint", "--quiet", *files_to_lint])
#
#     (Path(".cmake") / "cppcheck").mkdir(exist_ok=True)
#     defines = (
#         "_MSC_VER",
#         "_WIN32",
#         "_WIN64",
#         "_M_X64",
#         "BF_DEBUG",
#         "BF_ENABLE_ASSERTS",
#         "BF_ENABLE_LOGGING",
#         "TESTS",
#     )
#     run_command(
#         [
#             CPPCHECK_PATH,
#             "-j 4",
#             "--cppcheck-build-dir=.cmake/cppcheck",
#             "--project=compile_commands.json",
#             "-ivendor",
#             "--enable=all",
#             "--inline-suppr",
#             "--platform=win64",
#             "--suppress=*:*codegen*",
#             "--suppress=*:*vendor*",
#             "--suppress=checkLevelNormal",
#             "--suppress=checkersReport",
#             "--suppress=constParameterCallback",
#             "--suppress=constParameterPointer",
#             "--suppress=constParameterReference",
#             "--suppress=constStatement",
#             "--suppress=constVariable",
#             "--suppress=constVariablePointer",
#             "--suppress=constVariableReference",
#             "--suppress=cstyleCast",
#             "--suppress=duplicateBreak",
#             "--suppress=invalidPointerCast",
#             "--suppress=knownConditionTrueFalse",
#             "--suppress=memsetClassFloat",
#             "--suppress=missingIncludeSystem",
#             "--suppress=moduloofone",
#             "--suppress=normalCheckLevelMaxBranches",
#             "--suppress=nullPointerArithmetic",
#             "--suppress=passedByValue",
#             "--suppress=selfAssignment",
#             "--suppress=unmatchedSuppression",
#             "--suppress=unreadVariable",
#             "--suppress=useStlAlgorithm",
#             "--suppress=uselessAssignmentArg",
#             "--suppress=variableScope",
#             "--std=c++20",
#             "-q",
#             *[f"-D{d}" for d in defines],
#         ]
#     )
#
#     run_command(
#         rf"""
#             "{CLANG_TIDY_PATH}"
#             src\bf_dll_game.cpp
#             src\bf_exe_game_win.cpp
#             src\bf_exe_launcher.cpp
#             src\bf_exe_tests.cpp
#         """
#         # Убираем абсолютный путь к проекту из выдачи линтинга.
#         # Тут куча экранирования происходит, поэтому нужно дублировать обратные слеши.
#         + r'| sed "s/{}//"'.format(
#             str(PROJECT_DIR).replace(os.path.sep, os.path.sep * 3) + os.path.sep * 3
#         )
#     )


# @timing
# def do_cmake_ninja_files() -> None:
#     run_command(
#         r"""
#             cmake
#             -G Ninja
#             -B .cmake\ninja
#             -D CMAKE_CXX_COMPILER=cl
#             -D CMAKE_C_COMPILER=cl
#             -DCMAKE_CONFIGURATION_TYPES=Debug
#         """
#     )
#
#
# @timing
# def do_compile_commands_json() -> None:
#     run_command(
#         r"""
#             ninja
#             -C .cmake\ninja
#             -f build.ninja
#             -t compdb
#             > compile_commands.json
#         """
#     )


@timing
def do_stop_debugger_ahk() -> None:
    run_command(r".nvim-personal\cli.ahk stop_debugger")


# exe_name: game.
@timing
def do_run_in_debugger_ahk(exe_name: str, *, debug: bool) -> None:
    build_type = "Debug"
    if not debug:
        build_type = CMAKE_RELEASE_BUILD_TYPE

    exe_path = f".cmake/vs17/{build_type}/{exe_name}.exe"
    run_command(r".nvim-personal\cli.ahk run_in_debugger {}".format(exe_path))


def command(f):
    return app.command(f.__name__)(f)


# @command
# def cog():
#     files_to_cog_and_format = [
#         *SRC_DIR.rglob("*.cpp"),
#         *SRC_DIR.rglob("*.h"),
#         *(Path("codegen") / "cog").rglob("*.cpp"),
#         *(Path("codegen") / "cog").rglob("*.h"),
#     ]
#
#     for filepath in files_to_cog_and_format:
#         print(f"Processing {filepath}...")
#         assert filepath in files_to_cog_and_format
#
#         with open(filepath, encoding="utf-8") as in_file:
#             data = in_file.read()
#
#         if ("[[" + "[cog") in data:  # NOTE: string is split for cog.
#             result = subprocess.run(
#                 ".venv/Scripts/cog.exe -n UTF-8 -U -",
#                 check=True,
#                 input=data,
#                 stdout=subprocess.PIPE,
#                 text=True,
#                 encoding="utf-8",
#             )
#
#             result = subprocess.run(
#                 "clang-format",
#                 check=True,
#                 input=result.stdout,
#                 encoding="utf-8",
#                 text=True,
#                 shell=True,
#                 capture_output=True,
#             )
#
#             with open(filepath, "w", encoding="utf-8", newline="\n") as out_file:
#                 out_file.write(result.stdout)
#
#         else:
#             subprocess.run(
#                 f"clang-format -i {filepath}",
#                 check=True,
#                 input=data,
#                 encoding="utf-8",
#                 text=True,
#                 shell=True,
#             )


@command
def build_game_debug():
    do_cmake_vs_files(debug=True)
    do_build_game()


# @command
# def build_game_release():
#     do_cmake_vs_files(debug=False)
#
#     do_build_game()


@command
def build_game_web():
    do_cmake_web_files(debug=True)
    do_build_game_web()


@command
def game_run_in_debugger_debug():
    do_stop_debugger_ahk()

    do_cmake_vs_files(debug=True)
    do_build_game()

    do_run_in_debugger_ahk("game", debug=True)


# @command
# def game_run_in_debugger_release():
#     do_stop_debugger_ahk()
#
#     do_cmake_vs_files(debug=False)
#     do_build_game()
#
#     do_run_in_debugger_ahk("game", debug=False)
#
#
# @command
# def test():
#     do_cmake_vs_files(debug=True)
#     do_build_tests()
#     do_test()
#
#
# @command
# def lint():
#     do_cmake_ninja_files()
#     do_compile_commands_json()
#     do_lint()


def main() -> None:
    test_value = hash32("test")
    assert test_value == 0xAFD071E5, test_value

    # Исполняем файл относительно корня проекта.
    os.chdir(PROJECT_DIR)

    caught_exc = None
    with global_timing_manager_instance:
        try:
            app()
        except Exception as e:
            caught_exc = e
    if caught_exc is not None:
        raise caught_exc


if __name__ == "__main__":
    main()
