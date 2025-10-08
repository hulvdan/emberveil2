import os
import subprocess
import zipfile
from pathlib import Path
from typing import Callable, ParamSpec

import bf_swatch
import bf_swatch_aco
import typer
from bf_gamelib import do_generate
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
    T,
    data_values,
    git_bump_tag,
    git_stash,
    global_timing_manager_instance,
    hash32,
    hex_to_rgb,
    hex_to_rgb_floats,
    rgb_floats_to_hex,
    run_command,
    timed_exit,
    timing,
    transform_color,
)

P = ParamSpec("P")


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
            command.append(r"-B .cmake/vs17")

        case BuildPlatform.Web | BuildPlatform.WebYandex:
            command.insert(0, "emcmake")
            command.append(f"-B .cmake/{platform}_{build_type}")

        case _:
            assert False, f"Not supported platform: {platform}"

    run_command(" ".join(command))


@timing
def do_build(target: BuildTarget, platform: BuildPlatform, build_type: BuildType):
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

        case BuildPlatform.Web | BuildPlatform.WebYandex:
            run_command(rf"cmake --build .cmake\{platform}_{build_type} -t {target}")

        case _:
            assert False, f"Not supported platform: {platform}"


@timing
def do_test() -> None:
    run_command(str(CMAKE_TESTS_PATH), timeout_seconds=5)


@timing
def do_lint() -> None:
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
            src/engine/bf_game_exe.cpp
        """
        # Убираем абсолютный путь к проекту из выдачи линтинга.
        # Тут куча экранирования происходит, поэтому нужно дублировать обратные слеши.
        + r'| sed "s/{}//"'.format(
            str(PROJECT_DIR).replace(os.path.sep, os.path.sep * 3) + os.path.sep * 3
        )
    )


@timing
def do_cmake_ninja_files() -> None:
    run_command(
        rf"""
            cmake
            -G Ninja
            -B .cmake\ninja
            -D CMAKE_CXX_COMPILER=cl
            -D CMAKE_C_COMPILER=cl
            -D PLATFORM={BuildPlatform.Win}
            -DCMAKE_CONFIGURATION_TYPES={BuildType.Debug}
        """
    )


@timing
def do_compile_commands_json() -> None:
    run_command(
        r"""
            ninja
            -C .cmake\ninja
            -f build.ninja
            -t compdb
            > compile_commands.json
        """
    )


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
def codegen(platform: BuildPlatform, build_type: BuildType):
    do_cmake(platform, build_type)
    do_generate(platform, build_type)


@command
def build(target: BuildTarget, platform: BuildPlatform, build_type: BuildType):
    do_cmake(platform, build_type)
    do_generate(platform, build_type)
    do_build(target, platform, build_type)


@command
def build_all_and_test():
    test()
    for target, platform, build_type in ALLOWED_BUILDS:
        if target != BuildTarget.game:
            continue
        build(BuildTarget.game, platform, build_type)


@command
def run_in_debugger(target: BuildTarget, build_type: BuildType):
    platform = BuildPlatform.Win

    do_stop_debugger_ahk()

    do_cmake(platform, build_type)
    do_generate(platform, build_type)
    do_build(target, platform, build_type)

    do_run_in_debugger_ahk(target, build_type)


@command
def update_template():
    subprocess.run("git fetch template", check=True, shell=True)

    with git_stash():
        subprocess.run("git rebase template/template", check=True, shell=True)
        subprocess.run("poetry install", check=True, shell=True)


@command
def test():
    platform = BuildPlatform.Win
    build_type = BuildType.Debug

    do_cmake(platform, build_type)
    do_generate(platform, build_type)
    do_build(BuildTarget.tests, platform, build_type)
    do_test()


@command
def deploy_itch():
    git_bump_tag()

    with git_stash():
        build(BuildTarget.game, BuildPlatform.Web, BuildType.Release)

    zip_path = TEMP_DIR / "itch.zip"

    with zipfile.ZipFile(zip_path, "w") as archive:
        for filepath in ("index.data", "index.html", "index.js", "index.wasm"):
            archive.write(Path(".cmake/Web_Release") / filepath, filepath)

    target = "{}:html".format(data_values.itch_target)
    run_command([BUTLER_PATH, "push", zip_path, target])


@command
def deploy_yandex():
    git_bump_tag()

    with git_stash():
        build(BuildTarget.game, BuildPlatform.WebYandex, BuildType.Release)

    zip_path = TEMP_DIR / "yandex.zip"

    with zipfile.ZipFile(zip_path, "w") as archive:
        for filepath in ("index.data", "index.html", "index.js", "index.wasm"):
            archive.write(Path(".cmake/WebYandex_Release") / filepath, filepath)


@command
def make_swatch():
    # result = bf_swatch.parse("c:/Users/user/Downloads/woodspark.ase")
    # print(result)
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
        c = rgb_floats_to_hex(
            transform_color(
                hex_to_rgb_floats(color),
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
                "values": hex_to_rgb_floats(color),
            },
        }

    swatch_data = [process_color(c) for c in colors]
    bf_swatch.write(swatch_data, "aboba.ase")

    def process_color2(color: str) -> bf_swatch_aco.RawColor:
        r, g, b = hex_to_rgb(color)
        r = int(r * 65535 / 255)
        g = int(g * 65535 / 255)
        b = int(b * 65535 / 255)
        assert r < 65536, r
        assert g < 65536, g
        assert b < 65536, b
        assert r >= 0, r
        assert g >= 0, g
        assert b >= 0, b

        return bf_swatch_aco.RawColor(
            name=color,
            color_space=bf_swatch_aco.ColorSpace.RGB,
            component_1=r,
            component_2=g,
            component_3=b,
            component_4=65535,
        )

    with open("aboba.aco", "wb") as out_file:
        bf_swatch_aco.save_aco_file([process_color2(c) for c in new_colors], out_file)

    with open("aboba.pal", "w") as out_file:
        out_file.write(
            "JASC-PAL\n0100\n{}\n".format(
                len(new_colors),
            )
        )
        color_lines = []
        for color in new_colors:
            r, g, b = hex_to_rgb(color)
            color_lines.append(f"{r} {g} {b}")
        color_lines = color_lines[2:] + color_lines[:2]
        out_file.write("\n".join(color_lines))


@command
def lint():
    do_cmake_ninja_files()
    do_compile_commands_json()
    do_lint()


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
