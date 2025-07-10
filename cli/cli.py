import os
import subprocess
from collections import defaultdict
from enum import Enum
from typing import Callable, ParamSpec

import typer
from bf_lib import (
    MSBUILD_PATH,
    PROJECT_DIR,
    T,
    global_timing_manager_instance,
    hash32,
    recursive_mkdir,
    run_command,
    timed_exit,
    timing,
)

P = ParamSpec("P")

SHADERC_PATH = str(PROJECT_DIR / "vendor/bgfx/.build/win64_vs2022/bin/shadercRelease.exe")


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


sdl_platforms_mapping = {
    BuildPlatform.Win: "SDL_PLATFORM_WIN32",
    BuildPlatform.Web: "SDL_PLATFORM_EMSCRIPTEN",
}


class BuildTarget(StrEnum):
    game = "game"


# ASSETS_DIR,
# CLANG_TIDY_PATH,
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
                    recursive_mkdir(out_file)

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


# with open(PROJECT_DIR / "codegen" / "codegen.cpp", "w") as codegen_file:
#
#     def genline(line: str) -> None:
#         codegen_file.write(line)
#         codegen_file.write("\n")
#
#     shaders_per_platform = defaultdict(list)
#
#     for platform in BuildPlatform.values():
#         platform_mapping = {
#             # BGFX_RENDERER_TYPE
#             BuildPlatform.Win: [
#                 ("windows", "s_5_0", ["DIRECT3D11", "DIRECT3D12"]),
#             ],
#             BuildPlatform.Web: [
#                 ("asm.js", "100_es", ["OPENGLES"]),
#                 ("asm.js", "300_es", ["OPENGLES"]),
#             ],
#         }
#
#         assert platform in platform_mapping, f"Not supported platform: {platform}"
#
#         output_directory = PROJECT_DIR / "codegen" / "shaders"
#
#         base = PROJECT_DIR / "src" / "shaders"
#
#         for shader_type, shaders in (
#             ("vertex", list(base.glob("*_vs.sc"))),
#             ("fragment", list(base.glob("*_fs.sc"))),
#         ):
#             for shaderc_platform_name, profile, renderers in platform_mapping[
#                 platform
#             ]:
#                 for shader in shaders:
#                     varyingdef = str(shader).rsplit("_", 1)[0] + "_var.def.sc"
#
#                     out_file = output_directory / (
#                         shader.stem + f"_{platform}_{profile}.bin"
#                     )
#
#                     shaders_per_platform[platform].append(
#                         (out_file, shader_type, profile, renderers)
#                     )
#
#                     run_command(
#                         [
#                             SHADERC_PATH,
#                             f"-f {shader}",
#                             f"-o {out_file}",
#                             f"--type {shader_type}",
#                             f"--platform {shaderc_platform_name}",
#                             f"--profile {profile}",
#                             f'-i {PROJECT_DIR / "vendor" / "bgfx" / "src"}',
#                             f"--varyingdef {varyingdef}",
#                             "--bin2c",
#                             "-O3",
#                         ]
#                     )
#
#         for platform, d in shaders_per_platform.items():
#             genline("#ifdef {}".format(sdl_platforms_mapping[platform]))
#
#             genline("#endif")


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
            command.append('-G "Visual Studio 17 2022"')
            command.append(r"-B .cmake\vs17")

        case BuildPlatform.Web:
            command.insert(0, "emcmake")
            command.append(rf"-B .cmake\{platform}_{build_type}")

        case _:
            assert False, f"Not supported platform: {platform}"

    run_command(" ".join(command))


@timing
def do_build(target: BuildTarget, platform: BuildPlatform, build_type: BuildType):
    match platform:
        case BuildPlatform.Win:
            run_command(
                rf"""
                "{MSBUILD_PATH}" .cmake\vs17\game.sln
                -v:minimal
                -property:WarningLevel=3
                -t:{target}
                """
            )

        case BuildPlatform.Web:
            run_command(rf"cmake --build .cmake\Web_{build_type} -t {target}")

        case _:
            assert False, f"Not supported platform: {platform}"


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
    subprocess.run("git rebase template/template", check=True, shell=True)


# @command
# def test():
#     do_cmake_vs_files(Debug=True)
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
