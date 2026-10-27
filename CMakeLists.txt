# swapforcerecomp - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(swapforcerecomp LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build the SDK from the pinned submodule when present; otherwise generated/rexglue.cmake falls back
# to an installed SDK found through find_package. Pass -DREXSDK_DIR=<path> to use another checkout.
if(NOT REXSDK_DIR AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rexglue-sdk/CMakeLists.txt")
    set(REXSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rexglue-sdk" CACHE PATH
        "Path to the rexglue-sdk source tree" FORCE)
endif()

# Vulkan next to D3D12 (the SDK leaves it off on Windows): gpu_backend = "vulkan" picks it; D3D12
# stays the default. Measured in the hub at 1440p, scale 2: 87 FPS on Vulkan, 86 on D3D12.
set(REXGLUE_USE_VULKAN ON CACHE BOOL "Enable Vulkan graphics backend")

# Builds keep no path of the machine they were made on: the source folder is mapped away in
# __FILE__ (the logs' source locations) and the debug info, for every target here and in the SDK and
# the library. The setups on GitHub Releases are passed on (checked: no user folder left in the exe
# and its DLLs).
add_compile_options("$<$<COMPILE_LANGUAGE:C,CXX>:-ffile-prefix-map=${CMAKE_SOURCE_DIR}/=>")

include(generated/rexglue.cmake)

# Portal of Power support (thirdparty/skylanders-portal, shared with other Skylanders recomps).
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/skylanders-portal/CMakeLists.txt")
    message(FATAL_ERROR "thirdparty/skylanders-portal is missing. Run: git submodule update --init --recursive")
endif()
add_subdirectory(thirdparty/skylanders-portal)

# Recompiled code is generated from the user's own default.xex and is not in the repository.
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated/default/sources.cmake")
    message(WARNING
        "No recompiled code yet (generated/default/sources.cmake is missing).\n"
        "  1. Put your extracted game disc in rom/ (rom/default.xex must exist).\n"
        "  2. Run:  cmake --build --preset <preset> --target swapforcerecomp_codegen\n"
        "  3. Re-run:  cmake --preset <preset>\n"
        "  4. Build:  cmake --build --preset <preset>\n"
        "See README.md.")
endif()

# Sources
set(SWAPFORCERECOMP_SOURCES
    src/main.cpp
    src/game/kernel_stubs.cpp
    src/game/magic_moment_watch.cpp
    src/game/game_art.cpp
    src/game/game_fonts.cpp
    src/game/toy_images.cpp
    src/game/ui_icon.cpp
    src/hooks/difficulty_hook.cpp
    src/hooks/intro_skip_hook.cpp
    src/hooks/figure_intro_hook.cpp
    src/hooks/frame_rate_hook.cpp
    src/hooks/fxaa_scale_hook.cpp
    src/hooks/newsworthy_hook.cpp
    src/hooks/post_effects_hook.cpp
    src/hooks/scene_size_hook.cpp
    src/hooks/stuck_falling_hook.cpp
    src/hooks/ultrawide_hook.cpp
    src/localization.cpp
    src/settings_pages.cpp
)

if(WIN32)
    add_executable(swapforcerecompiled WIN32 ${SWAPFORCERECOMP_SOURCES})
else()
    add_executable(swapforcerecompiled ${SWAPFORCERECOMP_SOURCES})
endif()

rexglue_setup_target(swapforcerecompiled GPU_PLUGINS xenos)

if(REXSDK_DIR)
    # An installed SDK ships imgui's headers next to its own; an in-tree build must point at them.
    target_include_directories(swapforcerecompiled PRIVATE "${REXSDK_DIR}/thirdparty/imgui")
endif()

# Seed a fresh build dir with the settings-file template. Never overwrites an existing one: the
# in-game F4 Settings overlay saves the user's own changes to this exact file.
if(NOT EXISTS "${CMAKE_BINARY_DIR}/swapforcerecomp.toml")
    file(COPY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/swapforcerecomp.toml.example" "${CMAKE_BINARY_DIR}/swapforcerecomp.toml")
endif()

target_link_libraries(swapforcerecompiled PRIVATE skylanders_portal_rex skylanders_alchemy)
# The exe's icon: the game's logo from the owner's disc (rom/nxeart), made at build time (#61).
skylanders_game_icon(swapforcerecompiled "${CMAKE_CURRENT_SOURCE_DIR}/rom")

# --- Pinned game executable -------------------------------------------------
# The disc check (skylanders-portal src/xex, cmake/game_xex.cmake): the supported discs' SHA-256
# from docs/game/default.xex.sha256 in the game and in swapforcerecomp_xexcheck, the installer's check,
# with their tests.
skylanders_game_xex(TARGET swapforcerecompiled PREFIX swapforcerecomp TITLE "Skylanders: Swap Force"
                    HASH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/docs/game/default.xex.sha256")
