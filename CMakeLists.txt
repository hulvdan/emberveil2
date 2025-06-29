cmake_minimum_required(VERSION 3.29)
project(game CXX C)

# include(FetchContent)
# set(FETCHCONTENT_QUIET FALSE)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
    add_compile_definitions("BF_DEBUG=1")
else()
    add_compile_definitions("BF_DEBUG=0")
endif()

# vendor
#-----------------------------------------------------------------------------------
add_library(vendor INTERFACE)
target_include_directories(vendor INTERFACE "${PROJECT_SOURCE_DIR}/vendor")

# bgfx
#-----------------------------------------------------------------------------------
add_library(bgfx STATIC IMPORTED)
target_include_directories(bgfx INTERFACE "${PROJECT_SOURCE_DIR}/vendor/bgfx/include")
add_library(bx STATIC IMPORTED)
add_library(bimg STATIC IMPORTED)
target_link_libraries(bgfx INTERFACE bx bimg)
if ("${PLATFORM}" STREQUAL "Win")
    if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
        set_target_properties(bgfx PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bgfxDebug.lib")
        set_target_properties(bx PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bxDebug.lib")
        set_target_properties(bimg PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bimgDebug.lib")
    else()
        set_target_properties(bgfx PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bgfxRelease.lib")
        set_target_properties(bx PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bxRelease.lib")
        set_target_properties(bimg PROPERTIES IMPORTED_LOCATION
            "${PROJECT_SOURCE_DIR}/vendor/bgfx/.build/win64_vs2022/bin/bimgRelease.lib")
    endif()
elseif ("${PLATFORM}" STREQUAL "Web")
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
else()
    message(FATAL_ERROR "bgfx: Not supported platform: ${PLATFORM}")
endif()

# sdl
#-----------------------------------------------------------------------------------
add_subdirectory("${PROJECT_SOURCE_DIR}/vendor/sdl")

# game
#-----------------------------------------------------------------------------------
add_executable(game "${PROJECT_SOURCE_DIR}/src/main.cpp")
target_link_libraries(game bgfx SDL3-static vendor)

set_target_properties(game PROPERTIES
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

if ("${PLATFORM}" STREQUAL "Win")
elseif ("${PLATFORM}" STREQUAL "Web")
    set_target_properties(game PROPERTIES OUTPUT_NAME "index")
    target_compile_options(game PRIVATE
        "-o MIN_WEBGL_VERSION=1"
        "-o MAX_WEBGL_VERSION=2"
    )
    target_link_options(game PRIVATE
        "-sMIN_WEBGL_VERSION=1"
        "-sMAX_WEBGL_VERSION=2"
        "-sALLOW_MEMORY_GROWTH=1"
    )
    set_target_properties(game PROPERTIES SUFFIX ".html")

    if (CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
        target_compile_options(game PRIVATE
            "-O0"
            "-oASSERTIONS=2"
        )
        target_link_options(game PRIVATE
            "-sASSERTIONS=2"
            "-g4"
            "-O0"
            "-sFORCE_FILESYSTEM=1"
            "--shell-file=${PROJECT_SOURCE_DIR}/vendor/shell_debug.html"
        )
    else()
        target_compile_options(game PRIVATE
            "-O3"
            "-oASSERTIONS=0"
        )
        target_link_options(game PRIVATE
            "-O3"
            "-sASSERTIONS=0"
            "-sEXIT_RUNTIME=0"
            "--closure 1"
            "--shell-file=${PROJECT_SOURCE_DIR}/vendor/shell_release.html"
        )
    endif()
else()
    message(FATAL_ERROR "game: Not supported platform: ${PLATFORM}")
endif()


# Windows. Startup project in Visual Studio.
#-----------------------------------------------------------------------------------
set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT game)

# ///
