cmake_minimum_required(VERSION 3.29)
project(game CXX C)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /Zc:preprocessor)
endif()

# Adding BF_DEBUG
if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
    add_compile_definitions("BF_DEBUG=1")
else()
    add_compile_definitions("BF_DEBUG=0")
endif()

# Adding:
# BF_PLATFORM_Web
# BF_PLATFORM_WebYandex
# BF_PLATFORM_Win
add_compile_definitions("BF_PLATFORM_${PLATFORM}=1")

# vendor and codegen.
#-----------------------------------------------------------------------------------
add_library(vendor_and_codegen INTERFACE)
target_include_directories(vendor_and_codegen
    INTERFACE
        "${CMAKE_SOURCE_DIR}/codegen"
        "${CMAKE_SOURCE_DIR}/vendor"
)

# GameAnalytics.
#-----------------------------------------------------------------------------------
if (("${PLATFORM}" STREQUAL "Web") OR ("${PLATFORM}" STREQUAL "WebYandex"))
    add_library(GameAnalytics INTERFACE)
else()
    add_subdirectory(vendor/gameanalytics-sdk-cpp)
endif()
target_include_directories(GameAnalytics
    INTERFACE
        "${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/include"
)

# bgfx.
#-----------------------------------------------------------------------------------
add_library(bgfx STATIC IMPORTED)
target_include_directories(bgfx
    INTERFACE
        "${CMAKE_SOURCE_DIR}/vendor/bgfx/include"
)
add_library(bx STATIC IMPORTED)
target_include_directories(bx
    INTERFACE
        "${CMAKE_SOURCE_DIR}/vendor/bx/include"
)
add_library(bimg STATIC IMPORTED)
target_link_libraries(bgfx
    INTERFACE
        bx
        bimg
)
if ("${PLATFORM}" STREQUAL "Win")
    if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
        set_target_properties(bgfx
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bgfxDebug.lib"
        )
        set_target_properties(bx
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bxDebug.lib"
        )
        set_target_properties(bimg
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bimgDebug.lib"
        )
    else()
        set_target_properties(bgfx
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bgfxRelease.lib"
        )
        set_target_properties(bx
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bxRelease.lib"
        )
        set_target_properties(bimg
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bimgRelease.lib"
        )
    endif()
elseif (("${PLATFORM}" STREQUAL "Web") OR ("${PLATFORM}" STREQUAL "WebYandex"))
    if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
        set_target_properties(bgfx
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bgfxDebug.a"
        )
        set_target_properties(bx
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bxDebug.a"
        )
        set_target_properties(bimg
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bimgDebug.a"
        )
    else()
        set_target_properties(bgfx
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bgfxRelease.a"
        )
        set_target_properties(bx
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bxRelease.a"
        )
        set_target_properties(bimg
            PROPERTIES
                IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bimgRelease.a"
        )
    endif()
else()
    message(FATAL_ERROR "bgfx: Not supported platform: ${PLATFORM}")
endif()

# sdl.
#-----------------------------------------------------------------------------------
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/SDL3")

# box2d.
#-----------------------------------------------------------------------------------
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/box2d")
set_target_properties(box2d
    PROPERTIES
        COMPILE_WARNING_AS_ERROR OFF
)

# basisu_transcoder.
#-----------------------------------------------------------------------------------
add_library(zstd STATIC
    "${CMAKE_SOURCE_DIR}/vendor/basis_universal/zstd/zstd.h"
    "${CMAKE_SOURCE_DIR}/vendor/basis_universal/zstd/zstd.c"
)
target_include_directories(zstd PUBLIC "${CMAKE_SOURCE_DIR}/vendor/basis_universal/zstd")
add_library(basisu_transcoder STATIC
    "${CMAKE_SOURCE_DIR}/vendor/basis_universal/transcoder/basisu_transcoder.h"
    "${CMAKE_SOURCE_DIR}/vendor/basis_universal/transcoder/basisu_transcoder.cpp"
)
target_include_directories(basisu_transcoder
    PUBLIC
        "${CMAKE_SOURCE_DIR}/vendor/basis_universal/transcoder"
)
target_link_libraries(basisu_transcoder PRIVATE zstd)

if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
    target_compile_definitions(basisu_transcoder
        PRIVATE
            BASISU_FORCE_DEVEL_MESSAGES=1
    )
endif()

# tracy.
#-----------------------------------------------------------------------------------
add_subdirectory(vendor/tracy)

# stb_vorbis.
#-----------------------------------------------------------------------------------
add_library(stb_vorbis STATIC
    "${CMAKE_SOURCE_DIR}/vendor/stb_vorbis.c"
)
target_compile_definitions(stb_vorbis
    INTERFACE
        STB_VORBIS_HEADER_ONLY
)

# miniaudio.
#-----------------------------------------------------------------------------------
add_library(miniaudio STATIC
    "${CMAKE_SOURCE_DIR}/vendor/miniaudio/include/miniaudio.h"
    "${CMAKE_SOURCE_DIR}/vendor/miniaudio/src/miniaudio.c"
)
target_include_directories(miniaudio
    PUBLIC
        "${CMAKE_SOURCE_DIR}/vendor/miniaudio/include"
)
target_compile_definitions(miniaudio
    PRIVATE
        MA_IMPLEMENTATION
)
target_link_libraries(miniaudio
    PRIVATE
        vendor_and_codegen
        stb_vorbis
)

# game.
#-----------------------------------------------------------------------------------
add_executable(game "${CMAKE_SOURCE_DIR}/src/engine/bf_game_exe.cpp")
target_link_libraries(game
    PRIVATE
        SDL3-static
        bgfx
        box2d
        vendor_and_codegen
        basisu_transcoder
        GameAnalytics
        miniaudio
)
if ("${PLATFORM}" STREQUAL "Win")
    target_link_libraries(game
        PRIVATE
            TracyClient
    )
endif()
target_include_directories(game
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
)
target_compile_definitions(game
    PRIVATE
        DOCTEST_CONFIG_DISABLE
)

if ("${PLATFORM}" STREQUAL "Win")
elseif (("${PLATFORM}" STREQUAL "Web") OR ("${PLATFORM}" STREQUAL "WebYandex"))
    target_compile_options(game
        PRIVATE
            "-o MIN_WEBGL_VERSION=1"
            "-o MAX_WEBGL_VERSION=2"
    )
    target_link_options(game
        PRIVATE
            "-sUSE_WEBGL2=1"
            "-sMIN_WEBGL_VERSION=1"
            "-sMAX_WEBGL_VERSION=2"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sINITIAL_MEMORY=56426496" # 63MB heap.
            "-sSTACK_SIZE=1048576" # 1MB stack.
            "-sEXPORTED_RUNTIME_METHODS=['ccall']"
            --preload-file resources
    )
    set_target_properties(game
        PROPERTIES
            OUTPUT_NAME "index"
            SUFFIX ".html"
    )

    if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
        target_compile_options(game
            PRIVATE
                "-O0"
                "-oASSERTIONS=2"
        )
        target_link_options(game
            PRIVATE
                "-sASSERTIONS=2"
                "-g"
                "-O0"
                "-sFORCE_FILESYSTEM=1"
                "--shell-file=${CMAKE_SOURCE_DIR}/vendor/shell_debug.html"
        )
    else()
        target_compile_options(game
            PRIVATE
                "-O3"
                "-oASSERTIONS=0"
        )
        target_link_options(game
            PRIVATE
                "-O3"
                "-sASSERTIONS=0"
                "-sEXIT_RUNTIME=0"
                "--closure 1"
        )
        if ("${PLATFORM}" STREQUAL "WebYandex")
            target_link_options(game PRIVATE
                "--shell-file=${CMAKE_SOURCE_DIR}/.temp/shell_release_yandex.html")
        else()
            target_link_options(game PRIVATE
                "--shell-file=${CMAKE_SOURCE_DIR}/.temp/shell_release.html")
        endif()
    endif()
else()
    message(FATAL_ERROR "Not supported platform: ${PLATFORM}")
endif()

if (0)
    # TODO: ASAN
    if (NOT CMAKE_CONFIGURATION_TYPES STREQUAL "Release")
        # Enabling Address sanitizer
        if(MSVC)
            target_compile_options(game PRIVATE /fsanitize=address)
            # target_link_options(game PRIVATE /INFERASANLIBS)
        else()
            target_compile_options(game PRIVATE "-fsanitize=address" "-fno-omit-frame-pointer")
            target_link_options(game PRIVATE "-fsanitize=address")
        endif()

        # Enabling Undefined behaviour sanitizer
        if (NOT MSVC)
            # General UBSan flags — tune as needed
            set(UBSAN_FLAGS "-fsanitize=undefined -fno-sanitize-recover=all -fno-omit-frame-pointer")
            target_compile_options(game PRIVATE ${UBSAN_FLAGS})
            target_link_options(game PRIVATE ${UBSAN_FLAGS})
        endif()
    endif()
endif()

# Tests.
#-----------------------------------------------------------------------------------
add_executable(tests "${CMAKE_SOURCE_DIR}/src/engine/bf_game_exe.cpp")
target_link_libraries(tests
    PRIVATE
        SDL3-static
        bgfx
        box2d
        vendor_and_codegen
        basisu_transcoder
        GameAnalytics
        miniaudio
)
target_link_libraries(tests
    PRIVATE
        GameAnalytics
)
target_include_directories(tests
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
)
target_compile_definitions(tests
    PRIVATE
        TESTS
        SDL_MAIN_HANDLED
        DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
)

# GameAnalytics post setup.
#-----------------------------------------------------------------------------------
if ("${PLATFORM}" STREQUAL "Win")
    add_custom_command(
        TARGET game PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/curl/lib/win64
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/openssl/1.1.1d/libs/win64
            ${CMAKE_BINARY_DIR}
        DEPENDS GameAnalytics
    )
    add_custom_command(
        TARGET tests PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/curl/lib/win64
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/openssl/1.1.1d/libs/win64
            ${CMAKE_BINARY_DIR}
        DEPENDS GameAnalytics
    )
endif()

# Copying resources.
#-----------------------------------------------------------------------------------
add_custom_command(
    TARGET game PRE_BUILD
    COMMAND ${CMAKE_COMMAND}
      -E copy_directory ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:game>/resources
    DEPENDS GameAnalytics
)
add_custom_command(
    TARGET tests PRE_BUILD
    COMMAND ${CMAKE_COMMAND}
      -E copy_directory ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:tests>/resources
    DEPENDS GameAnalytics
)

# Enabling Linting On Windows.
#-----------------------------------------------------------------------------------
if(WIN32 AND CMAKE_GENERATOR STREQUAL Ninja)
    # add_compile_definitions(
    #     BF_DEBUG
    #     BF_LINT
    #     BF_ENABLE_ASSERTS
    # )

    # # NOTE(2025-01-06): Херню строкой ниже поставил,
    # # т.к. в какой-то момент перестала отрабатывать Ninja.
    # # ref: https://cmake.org/pipermail/cmake/2016-June/063848.html
    # set(CMAKE_C_COMPILE_OBJECT
    #     "cmake\
    #     -DWINE=${WINE}\
    #     -DCOMPILER=<CMAKE_C_COMPILER>\
    #     -DSOURCE=<SOURCE>\
    #     -DINCLUDES=\"<INCLUDES>\"\
    #     -DFLAGS=\"<DEFINES> <FLAGS>\"\
    #     -DOBJECT=<OBJECT>"
    # )

    include_directories(
        "${CMAKE_SOURCE_DIR}/vendor/"
        "${CMAKE_SOURCE_DIR}/vendor/basis_universal/transcoder"
        "${CMAKE_SOURCE_DIR}/vendor/bgfx/include"
        "${CMAKE_SOURCE_DIR}/vendor/bx/include"
        "${CMAKE_SOURCE_DIR}/vendor/bimg/include"
        "${CMAKE_SOURCE_DIR}/vendor/box2d/include"
        "${CMAKE_SOURCE_DIR}/vendor/bx/include"
        "${CMAKE_SOURCE_DIR}/vendor/flatbuffers"
        "${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/include"
        "${CMAKE_SOURCE_DIR}/vendor/glm"
        "${CMAKE_SOURCE_DIR}/vendor/miniaudio/include"
        "${CMAKE_SOURCE_DIR}/vendor/SDL3/include"
        # "${CMAKE_SOURCE_DIR}/vendor/tracy"
        "c:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/um"
    )
endif()

# Windows. Startup project in Visual Studio.
#-----------------------------------------------------------------------------------
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT game)

set_target_properties(game PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:game>"
    VS_DEBUGGER_COMMAND           "$<TARGET_FILE:game>"
    VS_DEBUGGER_ENVIRONMENT       "PATH=%PATH%;${CMAKE_PREFIX_PATH}/bin"
)

# ///
