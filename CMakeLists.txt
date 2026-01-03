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

# Enabling address sanitizer.
if (0)
    #
    # After ENABLING run following commands:
    #
    #   cd vendor/bgfx
    #   del /f/s/q .build
    #   make
    #   msbuild .build/projects/vs2022/bgfx.sln /t:Build /p:Configuration=Debug /p:EnableAddressSanitizer=true
    #
    # After DISABLING run following commands:
    #
    #   cd vendor/bgfx
    #   del /f/s/q .build
    #   make
    #   msbuild .build/projects/vs2022/bgfx.sln /t:Build /p:Configuration=Debug
    #   msbuild .build/projects/vs2022/bgfx.sln /t:Build /p:Configuration=Release
    #
    if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
        if ("${PLATFORM}" STREQUAL "Win")
            add_compile_options(/fsanitize=address)
        endif()
    endif()
endif()

# Adding:
# BF_PLATFORM_Web
# BF_PLATFORM_WebItch
# BF_PLATFORM_WebYandex
# BF_PLATFORM_Win
add_compile_definitions("BF_PLATFORM_${PLATFORM}=1")

if ("${PLATFORM}" MATCHES "^Web")
    if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
    else()
        set(CMAKE_C_FLAGS_RELEASE "-O3 -flto -s WASM=1 -msimd128 -s ASSERTIONS=0 -s SAFE_HEAP=0 -fno-exceptions -fno-rtti")
        set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -s WASM=1 -msimd128 -s ASSERTIONS=0 -s SAFE_HEAP=0 -fno-exceptions -fno-rtti")
    endif()
endif()

add_library(optimization INTERFACE)
if ("${PLATFORM}" MATCHES "^Web")
    if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
        target_compile_options(optimization INTERFACE
            "-O1"
            "-oASSERTIONS=1"
            "-fsanitize=undefined"
        )
        target_link_options(optimization INTERFACE
            "-sASSERTIONS=1"
            "-g"
            "-O1"
            "-fsanitize=undefined"
            --profiling-funcs
        )
    else()
        target_compile_options(optimization INTERFACE
            -O3
            "-oASSERTIONS=0"
        )
        target_link_options(optimization INTERFACE
            -O3
            "--closure 1"
            "-sASSERTIONS=0"
            "-s WASM_ASYNC_COMPILATION=1"
        )
    endif()
endif()

# vendor and codegen.
#-----------------------------------------------------------------------------------
add_library(vendor_and_codegen INTERFACE)
target_include_directories(vendor_and_codegen
    INTERFACE
        "${CMAKE_SOURCE_DIR}/codegen"
        "${CMAKE_SOURCE_DIR}/vendor"
)

# # GameAnalytics.
# #-----------------------------------------------------------------------------------
# if ("${PLATFORM}" MATCHES "^Web")
#     add_library(GameAnalytics INTERFACE)
# else()
#     add_subdirectory(vendor/gameanalytics-sdk-cpp)
# endif()
# target_include_directories(GameAnalytics
#     INTERFACE
#         "${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/include"
# )

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
elseif ("${PLATFORM}" MATCHES "^Web")
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
# target_link_libraries(SDL3-static PRIVATE optimization)

# box2d.
#-----------------------------------------------------------------------------------
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/box2d")
set_target_properties(box2d
    PROPERTIES
        COMPILE_WARNING_AS_ERROR OFF
)
# target_link_libraries(box2d PRIVATE optimization)

# zstd.
#-----------------------------------------------------------------------------------
add_library(zstd STATIC
    "${CMAKE_SOURCE_DIR}/vendor/zstd/zstd.h"
    "${CMAKE_SOURCE_DIR}/vendor/zstd/zstd.c"
)
target_include_directories(zstd PUBLIC "${CMAKE_SOURCE_DIR}/vendor/zstd")
target_link_libraries(zstd PRIVATE optimization)

# basisu_transcoder.
#-----------------------------------------------------------------------------------
add_library(basisu_transcoder STATIC
    "${CMAKE_SOURCE_DIR}/vendor/basis_universal/transcoder/basisu_transcoder.h"
    "${CMAKE_SOURCE_DIR}/vendor/basis_universal/transcoder/basisu_transcoder.cpp"
)
target_include_directories(basisu_transcoder
    PUBLIC
        "${CMAKE_SOURCE_DIR}/vendor/basis_universal/transcoder"
)
target_link_libraries(basisu_transcoder PRIVATE zstd optimization)

if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
    target_compile_definitions(basisu_transcoder
        PRIVATE
            BASISU_FORCE_DEVEL_MESSAGES=1
    )
endif()

# tracy.
#-----------------------------------------------------------------------------------
add_subdirectory(vendor/tracy)
target_compile_definitions(TracyClient PRIVATE optimization)

# stb_vorbis.
#-----------------------------------------------------------------------------------
add_library(stb_vorbis STATIC
    "${CMAKE_SOURCE_DIR}/vendor/stb_vorbis.c"
)
target_compile_definitions(stb_vorbis
    INTERFACE
        STB_VORBIS_HEADER_ONLY
)
target_link_libraries(stb_vorbis PRIVATE optimization)
set_target_properties(stb_vorbis PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
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
target_link_libraries(miniaudio
    PRIVATE
        vendor_and_codegen
        stb_vorbis
        optimization
)
set_target_properties(miniaudio PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)
target_compile_definitions(miniaudio PUBLIC
    MA_NO_FLAC
    MA_NO_MP3
    MA_NO_GENERATION
    MA_NO_ENCODING
)
# target_compile_definitions(miniaudio PUBLIC MA_NO_DEVICE_IO)
# if ("${PLATFORM}" MATCHES "^Web")
#     target_compile_definitions(miniaudio PUBLIC MA_NO_DEVICE_IO)
# endif()

# readerwriterqueue.
#-----------------------------------------------------------------------------------
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/readerwriterqueue")
# target_link_libraries(readerwriterqueue PRIVATE optimization)

# # freetype.
# #-----------------------------------------------------------------------------------
# add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/freetype")

# imgui.
#-----------------------------------------------------------------------------------
add_library(imgui STATIC
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imgui.h"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imgui.cpp"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl3.cpp"
    "${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdlgpu3.cpp"
    # "${CMAKE_SOURCE_DIR}/vendor/imgui/misc/freetype/imgui_freetype.cpp"
)
target_include_directories(imgui
    PUBLIC
        "${CMAKE_SOURCE_DIR}/vendor/imgui"
        "${CMAKE_SOURCE_DIR}/vendor/imgui/backends"
        # "${CMAKE_SOURCE_DIR}/vendor/imgui/misc/freetype"
)
target_link_libraries(imgui
    PUBLIC
        SDL3-static
    PRIVATE
        optimization
    #     freetype
)
target_compile_definitions(imgui
    PUBLIC
        IMGUI_USE_WCHAR32
        # IMGUI_ENABLE_FREETYPE
)

# game.
#-----------------------------------------------------------------------------------
add_executable(game "${CMAKE_SOURCE_DIR}/src/engine/bf_engine.cpp")
target_link_libraries(game
    PRIVATE
        # GameAnalytics
        SDL3-static
        basisu_transcoder
        bgfx
        box2d
        bx
        imgui
        miniaudio
        readerwriterqueue
        vendor_and_codegen
        zstd
        optimization
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
elseif ("${PLATFORM}" MATCHES "^Web")
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
            "-sINITIAL_MEMORY=246022144" # X MB heap.
            "-sSTACK_SIZE=2097152" # 2MB stack.
            --preload-file resources
            -lembind
    )
    set_target_properties(game
        PROPERTIES
            OUTPUT_NAME "index"
            SUFFIX ".html"
    )

    if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
        target_link_options(game
            PRIVATE
                "-sFORCE_FILESYSTEM=1"
                "--shell-file=${CMAKE_SOURCE_DIR}/vendor/shell_debug.html"
                # "-sSAFE_HEAP=1"
        )
    else()
        if ("${PLATFORM}" STREQUAL "WebItch")
            target_link_options(game PRIVATE
                "--shell-file=${CMAKE_SOURCE_DIR}/.temp/shell_release_itch.html")
        elseif ("${PLATFORM}" STREQUAL "WebYandex")
            target_link_options(game PRIVATE
                "--shell-file=${CMAKE_SOURCE_DIR}/.temp/shell_release_yandex.html")
        elseif ("${PLATFORM}" STREQUAL "Web")
            target_compile_options(game
                PRIVATE
                    # "-oASSERTIONS=2"
                    # "-gsource-map"
                    # "-fsanitize=undefined"
                    # "-fsanitize-minimal-runtime"
                    # "-fsanitize=address"
            )
            target_link_options(game
                PRIVATE
                    # "-sSAFE_HEAP=1"
                    # "-sASSERTIONS=2"
                    # "-gsource-map"
                    # "-fsanitize=undefined"
                    # "-fsanitize-minimal-runtime"
                    # "-fsanitize=address"
            )
            target_link_options(game PRIVATE
                "--shell-file=${CMAKE_SOURCE_DIR}/.temp/shell_release.html")
        else()
            message(FATAL_ERROR "Not supported platform: ${PLATFORM}")
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
add_executable(tests "${CMAKE_SOURCE_DIR}/src/engine/bf_engine.cpp")
target_link_libraries(tests
    PRIVATE
        # GameAnalytics
        SDL3-static
        basisu_transcoder
        bgfx
        box2d
        bx
        imgui
        miniaudio
        readerwriterqueue
        vendor_and_codegen
        zstd
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

# # GameAnalytics post setup.
# #-----------------------------------------------------------------------------------
# if ("${PLATFORM}" STREQUAL "Win")
#     add_custom_command(
#         TARGET game PRE_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
#             ${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/curl/lib/win64
#             ${CMAKE_BINARY_DIR}
#         COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
#             ${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/openssl/1.1.1d/libs/win64
#             ${CMAKE_BINARY_DIR}
#         DEPENDS GameAnalytics
#     )
#     add_custom_command(
#         TARGET tests PRE_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
#             ${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/curl/lib/win64
#             ${CMAKE_BINARY_DIR}
#         COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
#             ${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/openssl/1.1.1d/libs/win64
#             ${CMAKE_BINARY_DIR}
#         DEPENDS GameAnalytics
#     )
# endif()

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
        # "${CMAKE_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/include"
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
