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

# vendor and codegen
#-----------------------------------------------------------------------------------
add_library(vendor_and_codegen INTERFACE)
target_include_directories(vendor_and_codegen
    INTERFACE
        "${CMAKE_SOURCE_DIR}/codegen"
        "${CMAKE_SOURCE_DIR}/vendor"
)

# bgfx
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
elseif ("${PLATFORM}" STREQUAL "Web")
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

# sdl
#-----------------------------------------------------------------------------------
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor/SDL3")

# box2d.
#-----------------------------------------------------------------------------------
add_subdirectory("${PROJECT_SOURCE_DIR}/vendor/box2d")
set_target_properties(box2d
    PROPERTIES
        COMPILE_WARNING_AS_ERROR OFF
)

# game
#-----------------------------------------------------------------------------------
add_executable(game "${CMAKE_SOURCE_DIR}/src/engine/bf_game_exe.cpp")
target_link_libraries(game
    PRIVATE
        SDL3-static
        bgfx
        box2d
        vendor_and_codegen
)
target_include_directories(game
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
)
target_compile_definitions(game
    PRIVATE
        DOCTEST_CONFIG_DISABLE
)

if ("${PLATFORM}" STREQUAL "Win")
elseif ("${PLATFORM}" STREQUAL "Web")
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
                "--shell-file=${CMAKE_SOURCE_DIR}/vendor/shell_release.html"
        )
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

add_custom_command(
    TARGET game PRE_BUILD
    COMMAND ${CMAKE_COMMAND}
      -E copy_directory ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:game>/resources
    DEPENDS game
)

# tests
#-----------------------------------------------------------------------------------
add_executable(tests "${CMAKE_SOURCE_DIR}/src/engine/bf_game_exe.cpp")
target_link_libraries(tests
    PRIVATE
        SDL3-static
        bgfx
        box2d
        vendor_and_codegen
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

# Windows. Startup project in Visual Studio.
#-----------------------------------------------------------------------------------
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT game)

# ///
