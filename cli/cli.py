import os
import subprocess
import zipfile
from itertools import product
from pathlib import Path
from typing import Callable, ParamSpec

import typer
from bf_gamelib import do_generate
from bf_lib import (
    BUTLER_PATH,
    # CLANG_CL_PATH,
    CMAKE_TESTS_PATH,
    MSBUILD_PATH,
    PROJECT_DIR,
    TEMP_DIR,
    BuildPlatform,
    BuildTarget,
    BuildType,
    T,
    data_values,
    global_timing_manager_instance,
    hash32,
    run_command,
    timed_exit,
    timing,
)

P = ParamSpec("P")


sdl_platforms_mapping = {
    BuildPlatform.Win: "SDL_PLATFORM_WIN32",
    BuildPlatform.Web: "SDL_PLATFORM_EMSCRIPTEN",
}


def hook_exit():
    global exit
    exit = timed_exit  # noqa: A001


app = typer.Typer(
    callback=hook_exit, result_callback=timed_exit, pretty_exceptions_enable=False
)


@timing
def do_cmake(platform: BuildPlatform, build_type: BuildType) -> None:
    command = [
        "cmake",
        "-DBUILD_SHARED_LIBS=OFF",
        f"-DPLATFORM={platform}",
        f"-DCMAKE_CONFIGURATION_TYPES={build_type}",
    ]

    match platform:
        case BuildPlatform.Win:
            # TODO: ASAN
            # if build_type != BuildType.Release:
            #     command.append("-A x64")
            #     command.append("-T ClangCL")

            command.append('-G "Visual Studio 17 2022"')
            command.append(r"-B .cmake\vs17")

        case BuildPlatform.Web:
            assert build_type != BuildType.RelWithDebInfo, (
                "Don't use 'RelWithDebInfo' for 'Web' target"
            )

            command.insert(0, "emcmake")
            command.append(rf"-B .cmake\{platform}_{build_type}")

        case _:
            assert False, f"Not supported platform: {platform}"

    run_command(" ".join(command))


@timing
def do_build(target: BuildTarget, platform: BuildPlatform, build_type: BuildType):
    match platform:
        case BuildPlatform.Win:
            compiler = MSBUILD_PATH
            # if build_type != BuildType.Release:
            #     compiler = CLANG_CL_PATH

            run_command(
                rf"""
                "{compiler}" .cmake\vs17\game.sln
                -v:minimal
                -property:WarningLevel=3
                -t:{target}
                """
            )

        case BuildPlatform.Web:
            run_command(rf"cmake --build .cmake\Web_{build_type} -t {target}")

        case _:
            assert False, f"Not supported platform: {platform}"


@timing
def do_build_tests() -> None:
    run_command(
        rf"""
        "{MSBUILD_PATH}" .cmake\vs17\game.sln
        -v:minimal
        -property:WarningLevel=3
        -t:tests
        """
    )


@timing
def do_test() -> None:
    run_command(str(CMAKE_TESTS_PATH))


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


@timing
def do_run_in_debugger_ahk(target: BuildTarget, build_type: BuildType) -> None:
    exe_path = f".cmake/vs17/{build_type}/{target}.exe"
    run_command(rf".nvim-personal\cli.ahk run_in_debugger {exe_path}")


def command(f: Callable[P, T]) -> Callable[P, T]:
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
def build(target: BuildTarget, platform: BuildPlatform, build_type: BuildType):
    do_cmake(platform, build_type)
    do_generate()
    do_build(target, platform, build_type)


@command
def build_all_and_test():
    test()
    for platform, build_type in product(BuildPlatform, BuildType):
        if (platform == BuildPlatform.Web) and (build_type == BuildType.RelWithDebInfo):
            continue

        build(BuildTarget.game, platform, build_type)


@command
def run_in_debugger(target, build_type: BuildType):
    platform = BuildPlatform.Win

    do_stop_debugger_ahk()

    do_cmake(platform, build_type)
    do_generate()
    do_build(target, platform, build_type)

    do_run_in_debugger_ahk(target, build_type)


@command
def update_template():
    subprocess.run("git fetch template", check=True, shell=True)
    git_status_process = subprocess.run(
        "git status --porcelain", check=True, shell=True, capture_output=True
    )
    git_status_text = git_status_process.stdout.decode("utf-8").strip()
    should_stash = bool(git_status_text)

    if should_stash:
        stash_message = datetime.now().strftime("%Y%m%d-%H%M%S template-update autostash")
        subprocess.run(f'git stash push -u -m "{stash_message}"', check=True, shell=True)

    subprocess.run("git rebase template/template", check=True, shell=True)
    subprocess.run("poetry install", check=True, shell=True)

    if should_stash:
        subprocess.run("git stash apply", check=True, shell=True)


@command
def test():
    platform = BuildPlatform.Win
    build_type = BuildType.Debug

    do_cmake(platform, build_type)
    do_build(BuildTarget.tests, platform, build_type)
    do_test()


@command
def deploy_itch():
    zip_path = TEMP_DIR / "itch.zip"

    with zipfile.ZipFile(zip_path, "w") as archive:
        for filepath in ("index.data", "index.html", "index.js", "index.wasm"):
            archive.write(Path(".cmake/Web_Release") / filepath, filepath)

    target = "{}:html".format(data_values.itch_target)
    run_command([BUTLER_PATH, "push", zip_path, target])


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
