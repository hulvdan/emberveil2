# Imports.  {  ###
import asyncio
import datetime
import os
import zipfile
from collections import Counter
from pathlib import Path

import bf_lib
import websockets
from bf_game import *  # noqa
from bf_gamelib import (
    do_generate,
    get_sounds_that_reaper_would_export,
    regenerate_shaders,
)
from bf_lib import (
    ALLOWED_BUILDS,
    BUTLER_PATH,
    CLANG_TIDY_PATH,
    CMAKE_TESTS_PATH,
    CPPCHECK_PATH,
    MSBUILD_PATH,
    PROJECT_DIR,
    SRC_DIR,
    TEMP_DIR,
    BuildPlatform,
    BuildTarget,
    BuildType,
    bannerify,
    game_settings,
    git_bump_tag,
    git_check_no_unstashed,
    git_stash,
    hash32,
    run_command,
)
from bf_typer import app, command, global_timing_manager_instance, timing

# }


@timing
def make_web_build_archive(zip_path: Path, cmake_build_out_path: Path) -> None:
    # {  ###
    with zipfile.ZipFile(zip_path, "w") as archive:
        for filepath in (
            "index.data",
            "index.html",
            "index.js",
            "index.wasm",
        ):
            archive.write(cmake_build_out_path / filepath, filepath)
        RESOURCES_POSTLOAD_DIR = PROJECT_DIR / "resp"
        for f in RESOURCES_POSTLOAD_DIR.glob("*"):
            archive.write(f, "{}/{}".format(RESOURCES_POSTLOAD_DIR.name, f.name))
    # }


@timing
def do_cmake(platform: BuildPlatform, build_type: BuildType) -> None:
    # {  ###
    command = [
        "cmake",
        "-DBUILD_SHARED_LIBS=OFF",
        # "-T ClangCL",
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
            command.append(r"-B .cmake/vs17")

        case _:
            if platform.lower().startswith("web"):
                command.insert(0, "emcmake")
                command.append(f"-B .cmake/{platform}_{build_type}")
            else:
                assert False, f"Not supported platform: {platform}"

    run_command(" ".join(command))
    # }


@timing
def do_build(target: BuildTarget, platform: BuildPlatform, build_type: BuildType):
    # {  ###
    build_id = (target, platform, build_type)
    assert build_id in ALLOWED_BUILDS, "{} is not allowed!".format(build_id)

    match platform:
        case BuildPlatform.Win:
            compiler = MSBUILD_PATH
            # if build_type != BuildType.Release:
            #     compiler = CLANG_CL_PATH

            run_command(
                rf"""
                "{compiler}" .cmake/vs17/game.sln
                -v:minimal
                -property:WarningLevel=3
                -t:{target}
                """
            )

        case _:
            if platform.lower().startswith("web"):
                run_command(rf"cmake --build .cmake/{platform}_{build_type} -t {target}")

                if platform == BuildPlatform.WebYandex:
                    make_web_build_archive(
                        TEMP_DIR / "yandex.zip", Path(f".cmake/{platform}_{build_type}")
                    )

            else:
                assert False, f"Not supported platform: {platform}"
    # }


@timing
def do_test() -> None:
    run_command(str(CMAKE_TESTS_PATH), timeout_seconds=5)


@timing
def do_lint() -> None:
    # {  ###
    files_to_lint = [
        *SRC_DIR.rglob("*.cpp"),
        *SRC_DIR.rglob("*.h"),
    ]
    run_command(["poetry", "run", "cpplint", "--quiet", *files_to_lint])

    (Path(".cmake") / "cppcheck").mkdir(exist_ok=True)
    defines = (
        "BF_DEBUG=1",
        "BF_PLATFORM_Win=1",
        "TESTS=1",
    )
    run_command(
        [
            CPPCHECK_PATH,
            "-j 4",
            "--cppcheck-build-dir=.cmake/cppcheck",
            "--project=compile_commands.json",
            "-ivendor",
            "--enable=all",
            "--inline-suppr",
            "--platform=win64",
            "--suppress=*:*codegen*",
            "--suppress=*:*vendor*",
            "--suppress=checkLevelNormal",
            "--suppress=checkersReport",
            "--suppress=constParameterCallback",
            "--suppress=constParameterPointer",
            "--suppress=constParameterReference",
            "--suppress=constStatement",
            "--suppress=constVariable",
            "--suppress=constVariablePointer",
            "--suppress=constVariableReference",
            "--suppress=cstyleCast",
            "--suppress=duplicateBreak",
            "--suppress=invalidPointerCast",
            "--suppress=knownConditionTrueFalse",
            "--suppress=memsetClassFloat",
            "--suppress=missingIncludeSystem",
            "--suppress=moduloofone",
            "--suppress=normalCheckLevelMaxBranches",
            "--suppress=nullPointerArithmetic",
            "--suppress=passedByValue",
            "--suppress=selfAssignment",
            "--suppress=unmatchedSuppression",
            "--suppress=unreadVariable",
            "--suppress=useStlAlgorithm",
            "--suppress=uselessAssignmentArg",
            "--suppress=variableScope",
            "--std=c++20",
            "-q",
            *[f"-D{d}" for d in defines],
        ]
    )

    run_command(
        rf"""
            "{CLANG_TIDY_PATH}"
            src/engine/bf_engine.cpp
        """
        # Убираем абсолютный путь к проекту из выдачи линтинга.
        # Тут куча экранирования происходит, поэтому нужно дублировать обратные слеши.
        + r'| sed "s/{}//"'.format(
            str(PROJECT_DIR).replace(os.path.sep, os.path.sep * 3) + os.path.sep * 3
        )
    )
    # }


@timing
def do_cmake_ninja_files() -> None:
    # {  ###
    run_command(
        rf"""
            cmake
            -G Ninja
            -B .cmake/ninja
            -D CMAKE_CXX_COMPILER=cl
            -D CMAKE_C_COMPILER=cl
            -D PLATFORM={BuildPlatform.Win}
            -DCMAKE_CONFIGURATION_TYPES={BuildType.Debug}
        """
    )
    # }


@timing
def do_compile_commands_json() -> None:
    # {  ###
    run_command(
        r"""
            ninja
            -C .cmake/ninja
            -f build.ninja
            -t compdb
            > compile_commands.json
        """
    )
    # }


@timing
def do_stop_debugger_ahk() -> None:
    run_command(r"autohotkey .nvim-personal\cli.ahk stop_debugger")


@timing
def do_run_in_debugger_ahk(target: BuildTarget, build_type: BuildType) -> None:
    # {  ###
    exe_path = f".cmake/vs17/{build_type}/{target}.exe"
    run_command(rf"autohotkey .nvim-personal\cli.ahk run_in_debugger {exe_path}")
    # }


# @command
# def cog():
#     # {  ###
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
#   # }


@timing
def do_activate_game_ahk() -> None:
    run_command(r"autohotkey .nvim-personal\cli.ahk activate_game")


@command
def codegen(platform: BuildPlatform, build_type: BuildType):
    # {  ###
    do_cmake(platform, build_type)
    do_generate(platform, build_type)
    do_activate_game_ahk()
    # }


@command
def build(target: BuildTarget, platform: BuildPlatform, build_type: BuildType):
    # {  ###
    do_cmake(platform, build_type)
    do_generate(platform, build_type)
    do_build(target, platform, build_type)
    # }


@command
def build_all_and_test():
    # {  ###

    test()
    for target, platform, build_type in ALLOWED_BUILDS:
        if target != BuildTarget.game:
            continue
        do_generate(platform, build_type)
        build(BuildTarget.game, platform, build_type)
        bf_lib._gamelib = None  # noqa: SLF001
    # }


@command
def run_in_debugger(target: BuildTarget, build_type: BuildType):
    # {  ###
    platform = BuildPlatform.Win

    do_stop_debugger_ahk()

    do_cmake(platform, build_type)
    do_generate(platform, build_type)
    do_build(target, platform, build_type)

    do_run_in_debugger_ahk(target, build_type)
    # }


@command
def update_template():
    # {  ###
    run_command("git fetch template")

    with git_stash():
        run_command("git merge template/template")
        run_command("poetry install")
    # }


@command
def test():
    # {  ###
    platform = BuildPlatform.Win
    build_type = BuildType.Debug

    do_cmake(platform, build_type)
    do_generate(platform, build_type)
    do_build(BuildTarget.tests, platform, build_type)
    do_test()
    # }


@command
def deploy_itch():
    # {  ###
    git_check_no_unstashed()

    git_bump_tag()

    with git_stash():
        build(BuildTarget.game, BuildPlatform.WebItch, BuildType.Release)

    zip_path = TEMP_DIR / "itch.zip"
    make_web_build_archive(zip_path, Path(".cmake/WebItch_Release"))

    target = "{}:html".format(game_settings.itch_target)
    run_command([BUTLER_PATH, "push", zip_path, target])
    # }


@command
def deploy_yandex():
    # {  ###
    git_check_no_unstashed()

    git_bump_tag()

    with git_stash():
        build(BuildTarget.game, BuildPlatform.WebYandex, BuildType.Release)
    # }


@command
def receive_ws_logs(port: int):
    # {  ###
    get_time = lambda: datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]

    async def handler(ws):
        print(f"{get_time()} I: CONNECTED")
        try:
            async for msg in ws:
                print(f"{get_time()} {msg}")
        except websockets.exceptions.ConnectionClosedError:
            print(f"{get_time()} I: DISCONNECTED")

    async def main():
        async with websockets.serve(handler, "0.0.0.0", port):
            print(f"Listening on ws://0.0.0.0:{port}")
            await asyncio.Future()

    asyncio.run(main())
    # }


@command
def lint():
    # {  ###
    do_cmake_ninja_files()
    do_compile_commands_json()
    do_lint()
    # }


# CREDITING SFX {  ###
def _credit_sfx(_folder: Path, _credit: str = "") -> None:
    pass
    # assert isinstance(credit, str)
    #
    # credits_file = folder / "_credits.txt"
    # if credits_file.exists():
    #     credit = credits_file.read_text("utf-8")
    #
    # for file in folder.iterdir():
    #     is_audio = file.is_file() and any(file.name.endswith(x) for x in AUDIO_EXTENSIONS)
    #     if credit:
    #         run_command(
    #             [
    #                 "ffmpeg",
    #                 "-i",
    #                 "-i",
    #             ]
    #         )
    #     else:
    #         log.warning()


# @command
# @timing
# def credit_sfx() -> None:
#     stack = [Path("e:/Media/SFX CREDIT REQUIRED")]
#     while stack:
#         p = stack.pop(0)
#         stack = stack[1:]


@command
def shaders() -> None:
    regenerate_shaders(True)


@command
def banner(filepath: Path) -> None:
    # {  ###
    filepath.write_text(
        bannerify([x.rstrip() for x in filepath.read_text("utf-8").splitlines()]),
        "utf-8",
    )
    # }


@command
def list_sounds() -> None:
    a = Counter()  # type: ignore[var-annotated]
    a.update(x.split("__", 1)[0] for x in get_sounds_that_reaper_would_export())
    print(a)


def main() -> None:
    # {  ###
    test_value = hash32("test")
    assert test_value == 0xAFD071E5, test_value
    test_value = hash32("test")  # Checking that it's stable.
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
    # }


if __name__ == "__main__":
    main()

###
