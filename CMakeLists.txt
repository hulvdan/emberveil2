cmake_minimum_required(VERSION 3.12)
project(game CXX C)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# bgfx
add_library(bgfx STATIC IMPORTED)
target_include_directories(bgfx INTERFACE "${PROJECT_SOURCE_DIR}/vendor/bgfx/include")
add_library(bx STATIC IMPORTED)
add_library(bimg STATIC IMPORTED)
target_link_libraries(bgfx INTERFACE bx bimg)
# if ("${PLATFORM}" STREQUAL "Win")
#     if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
#         set_target_properties(bgfx PROPERTIES IMPORTED_LOCATION
#             "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bgfxDebug.lib")
#         set_target_properties(bx PROPERTIES IMPORTED_LOCATION
#             "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bxDebug.lib")
#         set_target_properties(bimg PROPERTIES IMPORTED_LOCATION
#             "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bimgDebug.lib")
#     else()
#         set_target_properties(bgfx PROPERTIES IMPORTED_LOCATION
#             "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bgfxRelease.lib")
#         set_target_properties(bx PROPERTIES IMPORTED_LOCATION
#             "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bxRelease.lib")
#         set_target_properties(bimg PROPERTIES IMPORTED_LOCATION
#             "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bimgRelease.lib")
#     endif()
# elif ("${PLATFORM}" STREQUAL "Web")
    if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
        set_target_properties(bgfx PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bgfxDebug.a")
        set_target_properties(bx PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bxDebug.a")
        set_target_properties(bimg PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bimgDebug.a")
    else()
        set_target_properties(bgfx PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bgfxRelease.a")
        set_target_properties(bx PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bxRelease.a")
        set_target_properties(bimg PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/wasm/bin/bimgRelease.a")
    endif()
# endif()

# sdl
add_subdirectory("${PROJECT_SOURCE_DIR}/vendor/sdl")

# game
add_executable(game "${PROJECT_SOURCE_DIR}/src/main.cpp")
target_link_libraries(game bgfx SDL3-static)

# if ("${PLATFORM}" STREQUAL "Web")
    set_target_properties(game PROPERTIES OUTPUT_NAME "index")
    target_compile_options(game PRIVATE
        "-o MIN_WEBGL_VERSION=1"
        "-o MAX_WEBGL_VERSION=2"
    )
    target_link_options(game PRIVATE
        "-sMIN_WEBGL_VERSION=1"
        "-sMAX_WEBGL_VERSION=2"
        "-sALLOW_MEMORY_GROWTH=1"
        "--shell-file=${PROJECT_SOURCE_DIR}/vendor/minshell.html"
    )
    set_target_properties(game PROPERTIES SUFFIX ".html")
# endif ()

if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
    target_link_options(game PRIVATE
        "-sASSERTIONS=1"
    )
    target_compile_options(game PRIVATE
        "-oASSERTIONS=1"
    )
else()
endif ()


# add_compile_definitions("$<$<CONFIG:Debug>:BF_DEBUG=1>")
# add_compile_definitions("$<$<NOT:$<CONFIG:Debug>>:BF_DEBUG=0>")
# add_compile_definitions("$<$<CONFIG:Debug>:BF_PROFILING=0>")
#
# # GameAnalytics.
# #-----------------------------------------------------------------------------------
# add_library(game_analytics STATIC IMPORTED)
# target_include_directories(game_analytics INTERFACE
#     "${PROJECT_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/include"
# )
# set_target_properties(game_analytics PROPERTIES
#     IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/build/Debug/GameAnalytics.lib"
# )
# target_link_libraries(game_analytics INTERFACE
#     "${PROJECT_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/curl/lib/win64/libcurl.lib"
#     "${PROJECT_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/openssl/1.1.1d/libs/win64/ssleay32.lib"
#     "${PROJECT_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/openssl/1.1.1d/libs/win64/libssl.lib"
#     "${PROJECT_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/openssl/1.1.1d/libs/win64/libeay32.lib"
#     "${PROJECT_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/externals/openssl/1.1.1d/libs/win64/libcrypto.lib"
# )
#
# # flatbuffers.
# #-----------------------------------------------------------------------------------
# add_library(flatbuffers INTERFACE)
# target_include_directories(flatbuffers INTERFACE
#     "${PROJECT_SOURCE_DIR}/vendor/flatbuffers-24.3.25/include"
#     "${PROJECT_SOURCE_DIR}/codegen/flatbuffers"
# )
#
# # toml-c.
# #-----------------------------------------------------------------------------------
# add_library(toml-c STATIC
#     "${PROJECT_SOURCE_DIR}/vendor/toml-c/toml.h"
#     "${PROJECT_SOURCE_DIR}/vendor/toml-c/toml.c"
# )
# target_include_directories(toml-c PUBLIC
#     "${PROJECT_SOURCE_DIR}/vendor/toml-c"
# )
#
# # vendor_and_codegen.
# #-----------------------------------------------------------------------------------
# add_library(vendor_and_codegen INTERFACE)
# target_include_directories(vendor_and_codegen INTERFACE
#     "${PROJECT_SOURCE_DIR}/vendor"
#     "${PROJECT_SOURCE_DIR}/codegen/hands"
#     "${PROJECT_SOURCE_DIR}/codegen/cog"
# )
#
# # box2d.
# #-----------------------------------------------------------------------------------
# add_subdirectory("${PROJECT_SOURCE_DIR}/vendor/box2d-3.0.0")
# set_property(TARGET box2d PROPERTY COMPILE_WARNING_AS_ERROR OFF)
#
# # raylib.
# #-----------------------------------------------------------------------------------
# add_subdirectory("${PROJECT_SOURCE_DIR}/vendor/raylib-5.5")
#
# # raytmx.
# #-----------------------------------------------------------------------------------
# add_library(raytmx STATIC
#     "${PROJECT_SOURCE_DIR}/vendor/raytmx/include/raytmx.h"
#     "${PROJECT_SOURCE_DIR}/vendor/raytmx/sources/raytmx.c"
# )
# target_include_directories(raytmx
#     PRIVATE
#         "${PROJECT_SOURCE_DIR}/vendor/raytmx/external"
#     PUBLIC
#         "${PROJECT_SOURCE_DIR}/vendor/raytmx/include"
# )
# target_compile_definitions(raytmx PRIVATE RAYTMX_IMPLEMENTATION)
# set_target_properties(raytmx PROPERTIES LINKER_LANGUAGE C)
# target_link_libraries(raytmx PRIVATE raylib)
#
# # # tracy.
# # #-----------------------------------------------------------------------------------
# # add_library(tracy)
# # target_include_directories(tracy PUBLIC "${PROJECT_SOURCE_DIR}/vendor/tracy-0.11.1/public")
# # target_sources(tracy PRIVATE
# #     "${PROJECT_SOURCE_DIR}/vendor/tracy-0.11.1/public/TracyClient.cpp"
# # )
# # target_compile_definitions(tracy PUBLIC
# #     TRACY_ENABLE
# #     TRACY_ONLY_LOCALHOST
# #     TRACY_NO_BROADCAST
# # )
# #
# # if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
# #     target_compile_definitions(tracy PUBLIC TRACY_IMPORTS)
# #     target_compile_definitions(tracy PRIVATE TRACY_EXPORTS)
# # endif()
#
# # Game.
# #-----------------------------------------------------------------------------------
# add_library(game src/bf_dll.h)
# target_compile_definitions(game PRIVATE BF_IMPLEMENTATION)
# target_sources(game PRIVATE
#     "${PROJECT_SOURCE_DIR}/src/bf_dll_game.cpp"
# )
# target_link_libraries(game box2d flatbuffers vendor_and_codegen raylib raytmx game_analytics)
#
# # if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
# #     target_link_libraries(game tracy)
# # endif()
#
# if (WIN32)
#     add_executable(game_win src/bf_exe_game_win.cpp)
#     add_dependencies(game_win game)
#     target_link_libraries(game_win toml-c)
#     target_compile_definitions(game_win PRIVATE NOMINMAX)
#
#     add_executable(launcher_win src/bf_exe_launcher_win.cpp)
#     target_link_libraries(launcher_win toml-c)
#     target_compile_definitions(launcher_win PRIVATE NOMINMAX)
#
#     if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
#         add_custom_command(
#             TARGET game_win POST_BUILD
#             COMMAND ${CMAKE_COMMAND}
#               -E copy $<TARGET_FILE_DIR:raylib>/raylib.dll $<TARGET_FILE_DIR:game_win>
#             COMMAND ${CMAKE_COMMAND}
#               -E copy $<TARGET_FILE_DIR:box2d>/box2d.dll $<TARGET_FILE_DIR:game_win>
#             DEPENDS game_win
#         )
#     else()
#         target_link_libraries(game_win game)
#     endif()
#
#     if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES STREQUAL "RelWithDebInfo")
#         add_custom_command(
#             TARGET game_win POST_BUILD
#             COMMAND ${CMAKE_COMMAND}
#               -E copy $<TARGET_FILE_DIR:raylib>/raylib.pdb $<TARGET_FILE_DIR:game_win>
#             COMMAND ${CMAKE_COMMAND}
#               -E copy $<TARGET_FILE_DIR:box2d>/box2d.pdb $<TARGET_FILE_DIR:game_win>
#             DEPENDS game_win
#         )
#     endif()
#
#     set_target_properties(game_win PROPERTIES
#         VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:game_win>"
#         VS_DEBUGGER_COMMAND           "$<TARGET_FILE:game_win>"
#         VS_DEBUGGER_ENVIRONMENT       "PATH=%PATH%;${CMAKE_PREFIX_PATH}/bin"
#     )
#
#     configure_file(${CMAKE_SOURCE_DIR}/src/assets/art/icon/icon.ico ${CMAKE_BINARY_DIR}/icon.ico COPYONLY)
#     file(WRITE "${CMAKE_BINARY_DIR}/icon.rc" "IDI_ICON1 ICON DISCARDABLE \"icon.ico\"\n")
#     target_sources(game_win PRIVATE "${CMAKE_BINARY_DIR}/icon.rc")
# endif()
#
# if ("${PLATFORM}" STREQUAL "Web")
#     add_executable(game_web src/bf_exe_game_web.cpp)
#     target_compile_definitions(game_web PRIVATE BF_IMPLEMENTATION)
#     target_link_libraries(game_web box2d flatbuffers vendor_and_codegen raylib raytmx)
#
#     set_target_properties(game_web PROPERTIES SUFFIX ".html")
#     target_link_options(game_web PRIVATE --preload-file resources)
#
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -sGL_ENABLE_GET_PROC_ADDRESS=1 -O3")
#     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -sGL_ENABLE_GET_PROC_ADDRESS=1 -O3")
#
#     set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wall -sUSE_GLFW=3 -sASSERTIONS=1 -sWASM=1 -sGL_ENABLE_GET_PROC_ADDRESS=1 -sALLOW_MEMORY_GROWTH -sASYNCIFY --shell-file ./../../vendor/raylib-5.5/src/minshell.html -sERROR_ON_UNDEFINED_SYMBOLS=0 -s EXPORTED_RUNTIME_METHODS=ccall,HEAPF32 -O3")
# endif()
#
# add_custom_command(
#     TARGET game PRE_BUILD
#     COMMAND touch ${CMAKE_BINARY_DIR}/.game_build_lock
#     COMMAND ${CMAKE_COMMAND}
#       -E copy_directory ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:game>/resources
#     DEPENDS game
# )
# add_custom_command(
#     TARGET game POST_BUILD
#     COMMAND rm ${CMAKE_BINARY_DIR}/.game_build_lock
#     DEPENDS game
# )
#
# # Boner.
# #-----------------------------------------------------------------------------------
# add_library(boner src/bf_dll.h)
# target_compile_definitions(boner PRIVATE BF_IMPLEMENTATION)
# target_sources(boner PRIVATE
#     "${PROJECT_SOURCE_DIR}/src/bf_dll_boner.cpp"
# )
# target_link_libraries(boner flatbuffers raylib vendor_and_codegen)
#
# if (WIN32)
#     add_executable(boner_win src/bf_exe_boner_win.cpp)
#     add_dependencies(boner_win boner)
#     target_link_libraries(boner_win toml-c)
#
#     target_compile_definitions(boner_win PRIVATE NOMINMAX)
#
#     if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
#         add_custom_command(
#             TARGET boner_win POST_BUILD
#             COMMAND ${CMAKE_COMMAND}
#               -E copy $<TARGET_FILE_DIR:raylib>/raylib.dll $<TARGET_FILE_DIR:boner_win>
#             DEPENDS boner_win
#         )
#     endif()
#
#     set_target_properties(boner_win PROPERTIES
#         VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:boner_win>"
#         VS_DEBUGGER_COMMAND           "$<TARGET_FILE:boner_win>"
#         VS_DEBUGGER_ENVIRONMENT       "PATH=%PATH%;${CMAKE_PREFIX_PATH}/bin"
#     )
# endif()
#
# add_custom_command(
#     TARGET boner PRE_BUILD
#     COMMAND touch ${CMAKE_BINARY_DIR}/.boner_build_lock
#     DEPENDS boner
# )
# add_custom_command(
#     TARGET boner POST_BUILD
#     COMMAND rm ${CMAKE_BINARY_DIR}/.boner_build_lock
#     DEPENDS boner
# )
#
# # Shaders.
# #-----------------------------------------------------------------------------------
# add_library(shaders src/bf_dll.h)
# target_compile_definitions(shaders PRIVATE BF_IMPLEMENTATION)
# target_sources(shaders PRIVATE
#     "${PROJECT_SOURCE_DIR}/src/bf_dll_shaders.cpp"
# )
# target_link_libraries(shaders flatbuffers raylib vendor_and_codegen)
#
# if (WIN32)
#     add_executable(shaders_win src/bf_exe_shaders_win.cpp)
#     add_dependencies(shaders_win shaders)
#     target_link_libraries(shaders_win toml-c)
#
#     target_compile_definitions(shaders_win PRIVATE NOMINMAX)
#
#     if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
#         add_custom_command(
#             TARGET shaders_win POST_BUILD
#             COMMAND ${CMAKE_COMMAND}
#               -E copy $<TARGET_FILE_DIR:raylib>/raylib.dll $<TARGET_FILE_DIR:shaders_win>
#             DEPENDS shaders_win
#         )
#     endif()
#
#     if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES STREQUAL "RelWithDebInfo")
#         add_custom_command(
#             TARGET shaders_win POST_BUILD
#             COMMAND ${CMAKE_COMMAND}
#               -E copy $<TARGET_FILE_DIR:raylib>/raylib.pdb $<TARGET_FILE_DIR:shaders_win>
#             DEPENDS shaders_win
#         )
#     endif()
#
#     set_target_properties(shaders_win PROPERTIES
#         VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:shaders_win>"
#         VS_DEBUGGER_COMMAND           "$<TARGET_FILE:shaders_win>"
#         VS_DEBUGGER_ENVIRONMENT       "PATH=%PATH%;${CMAKE_PREFIX_PATH}/bin"
#     )
# endif()
#
# add_custom_command(
#     TARGET shaders PRE_BUILD
#     COMMAND touch ${CMAKE_BINARY_DIR}/.shaders_build_lock
#     DEPENDS shaders
# )
# add_custom_command(
#     TARGET shaders POST_BUILD
#     COMMAND rm ${CMAKE_BINARY_DIR}/.shaders_build_lock
#     DEPENDS shaders
# )
#
# # Tests.
# #-----------------------------------------------------------------------------------
# add_executable(tests src/bf_exe_tests.cpp)
# target_compile_definitions(tests PRIVATE TESTS BF_IMPLEMENTATION)
# target_link_libraries(tests box2d flatbuffers vendor_and_codegen raylib raytmx)
#
# # Checks if OSX and links appropriate frameworks (Only required on MacOS)
# if (APPLE)
#     target_link_libraries(game "-framework IOKit")
#     target_link_libraries(game "-framework Cocoa")
#     target_link_libraries(game "-framework OpenGL")
#     target_link_libraries(boner "-framework IOKit")
#     target_link_libraries(boner "-framework Cocoa")
#     target_link_libraries(boner "-framework OpenGL")
#     target_link_libraries(shaders "-framework IOKit")
#     target_link_libraries(shaders "-framework Cocoa")
#     target_link_libraries(shaders "-framework OpenGL")
#     target_link_libraries(tests "-framework IOKit")
#     target_link_libraries(tests "-framework Cocoa")
#     target_link_libraries(tests "-framework OpenGL")
# endif()
#
# # Cleaning Up Stale Files and previously copied DLLs.
# #-----------------------------------------------------------------------------------
# # - Casey Muratori: MSVC PDBs Are Filled With Stale Debug Info:
# #   https://www.computerenhance.com/p/msvc-pdbs-are-filled-with-stale-debug
# IF ((WIN32) AND (NOT CMAKE_CONFIGURATION_TYPES STREQUAL "RelWithDebInfo"))
#     # NOTE: Visual Studio won't let us delete old bf_game.dll PDB files.
#     # This is why we are creating PDBs with timestamped names.
#     string(TIMESTAMP V_TIMESTAMP "%Y%m%d-%H%M%S" UTC)
#     set_target_properties(game PROPERTIES
#         PDB_NAME game_${V_TIMESTAMP}
#     )
#     set_target_properties(boner PROPERTIES
#         PDB_NAME boner_${V_TIMESTAMP}
#     )
#     set_target_properties(shaders PROPERTIES
#         PDB_NAME shaders_${V_TIMESTAMP}
#     )
#     file(GLOB files_to_delete
#         ${CMAKE_BINARY_DIR}/Debug/game_*.pdb
#         ${CMAKE_BINARY_DIR}/Debug/game_*.rdi
#         ${CMAKE_BINARY_DIR}/Debug/game_*.dll
#         ${CMAKE_BINARY_DIR}/Debug/boner_*.pdb
#         ${CMAKE_BINARY_DIR}/Debug/boner_*.rdi
#         ${CMAKE_BINARY_DIR}/Debug/boner_*.dll
#         ${CMAKE_BINARY_DIR}/Debug/shaders_*.pdb
#         ${CMAKE_BINARY_DIR}/Debug/shaders_*.rdi
#         ${CMAKE_BINARY_DIR}/Debug/shaders_*.dll
#     )
#     list(LENGTH files_to_delete files_to_delete_length)
#     IF (files_to_delete_length GREATER 0)
#         file(REMOVE ${files_to_delete})
#     ENDIF()
# ENDIF()
#
# # Enabling Linting On Windows.
# #-----------------------------------------------------------------------------------
# if(WIN32 AND CMAKE_GENERATOR STREQUAL Ninja)
#     add_compile_definitions(
#         BF_DEBUG
#         BF_LINT
#         BF_ENABLE_ASSERTS
#     )
#
#     # NOTE(2025-01-06): Херню строкой ниже поставил,
#     # т.к. в какой-то момент перестала отрабатывать Ninja.
#     # ref: https://cmake.org/pipermail/cmake/2016-June/063848.html
#     set(CMAKE_C_COMPILE_OBJECT
#         "cmake\
#         -DWINE=${WINE}\
#         -DCOMPILER=<CMAKE_C_COMPILER>\
#         -DSOURCE=<SOURCE>\
#         -DINCLUDES=\"<INCLUDES>\"\
#         -DFLAGS=\"<DEFINES> <FLAGS>\"\
#         -DOBJECT=<OBJECT>"
#     )
#
#     include_directories(
#         "${PROJECT_SOURCE_DIR}/vendor"
#         "${PROJECT_SOURCE_DIR}/vendor/box2d-3.0.0/include"
#         "${PROJECT_SOURCE_DIR}/vendor/gameanalytics-sdk-cpp/include"
#         "${PROJECT_SOURCE_DIR}/vendor/raytmx/include"
#         # "${PROJECT_SOURCE_DIR}/vendor/tracy-0.11.1/public"
#         "c:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/um"
#     )
# endif()
#
# # Windows. Startup project in Visual Studio.
# #-----------------------------------------------------------------------------------
# set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT game_win)
