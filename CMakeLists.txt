cmake_minimum_required(VERSION 3.20)
project(ZeldaALttPSNESRecomp C CXX)

# Cross-platform (macOS/Linux) build for LegendofZeldaAlttpRecomp.
#
# The canonical Windows build is the MSVC solution (zelda.sln). This CMake build
# mirrors the same source list (see snesrecomp/runner/runner.cmake) so the game
# builds natively on macOS/Linux with clang/gcc + SDL2 + OpenGL. Modeled on the
# MegamanXRecomp CMakeLists; Zelda uses no Win32 Fiber API, so no fiber shim.
#
# Prerequisites: a snesrecomp checkout at ./snesrecomp (symlink/clone of the
# sibling repo), the generated C in src/gen/ (run `bash tools/regen.sh` with a
# verified zelda.sfc at the repo root), and SDL3 (SDL2 still works via
# -DSNESRECOMP_SDL_BACKEND=SDL2).

set(CMAKE_C_STANDARD 11)

# Reported by host_report_init() in crash breadcrumbs/minidumps, and the pin two
# netplay peers compare. Derived from VERSION rather than defaulting to "dev",
# which is what the scaffold does (tools/new_project/templates/CMakeLists.txt.in):
# a release built from the CMake path used to ship stamped "dev", so a crash
# report could not be tied to the build that produced it.
# NOTE: quote it when configuring - PowerShell rewrites an unquoted
# -DSNESRECOMP_BUILD_VERSION=0.6.0 into "0".
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" _GAME_VER_RAW)
string(STRIP "${_GAME_VER_RAW}" GAME_RELEASE_VERSION)
set(SNESRECOMP_BUILD_VERSION "" CACHE STRING
    "Version reported in crash diagnostics and network metadata")
if(SNESRECOMP_BUILD_VERSION STREQUAL "")
    get_property(_is_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(_is_multi)
        set(SNESRECOMP_BUILD_VERSION
            "$<IF:$<CONFIG:Release>,${GAME_RELEASE_VERSION},dev>")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(SNESRECOMP_BUILD_VERSION "${GAME_RELEASE_VERSION}")
    else()
        set(SNESRECOMP_BUILD_VERSION "dev")
    endif()
endif()

# Widescreen is a mod package, not a Display setting (same migration as Mega Man
# X / X2 / Super Mario World / Super Mario Kart). Must be set BEFORE
# runner.cmake is included: the engine forces RECOMP_UI_ENABLE_MODS on from this
# and compiles the package loader + trusted-plugin runtime, which is what makes
# the launcher's Mods view appear at all.
set(SNESRECOMP_ENABLE_MODS ON CACHE BOOL
    "Enable A Link to the Past's built-in mod catalog and SNES mod packages" FORCE)

set(SNESRECOMP_ROOT ${CMAKE_SOURCE_DIR}/snesrecomp)
include(${SNESRECOMP_ROOT}/runner/runner.cmake)

# Launcher UI backend: the SHARED recomp-ui launcher (F:\Projects\recomp-ui,
# consumed as the ./recomp-ui submodule/junction) so every recomp game drives
# ONE launcher repo. Mirrors MegamanXRecomp/StarFoxSNESRecomp.
include(${CMAKE_SOURCE_DIR}/recomp-ui/recomp_ui.cmake)

find_package(OpenGL REQUIRED)

add_executable(ZeldaALttPSNESRecomp
    ${SNESRECOMP_RUNNER_SOURCES}
    # game-specific runtime
    src/main.c
    src/opengl.c
    # Trusted static plugin backing the widescreen mod package's activation.
    src/mods/zelda_widescreen_plugin.c
    # Trusted static plugin backing the MSU-1 audio mod package's activation.
    src/mods/zelda_msu1_plugin.c
    src/zelda_00.c
    src/gen_stubs.c
    src/zelda_cpu_infra.c
    src/zelda_rtl.c
    src/zelda_spc_player.c

    # OpenGL function loader
    third_party/gl_core/gl_core_3_1.c
)

# Generated recompiler output (produced by tools/regen.sh; not committed --
# it is derived from ROM bytes). Framework-owned rather than a local
# file(GLOB) + FATAL_ERROR, because the local version aborted before it could
# consider SNESRECOMP_SETUP_HOST: the release workflow configures with
# -DSNESRECOMP_SETUP_HOST=ON against an empty src/gen to build the SETUP PACK,
# and every run of it failed at configure. With the option, an empty src/gen
# builds the setup host (no recompiled code; the launcher's Generate & rebuild
# is the only path forward in that binary); without it, an empty src/gen is
# still a hard error and the framework owns the message.
snesrecomp_target_generated_code(ZeldaALttPSNESRecomp ${CMAKE_SOURCE_DIR}/src/gen)
if(SNESRECOMP_SETUP_HOST)
    # The game's half of a setup-host link: entry points src/zelda_rtl.c calls
    # directly, which normally come from src/gen. The framework resolves its
    # own half (setup_host_dispatch.c) inside the call above. Every one of
    # these aborts if entered -- SnesInit() refuses to boot a guest in this
    # build, so none of them is reachable.
    target_sources(ZeldaALttPSNESRecomp PRIVATE src/zelda_setup_host_stubs.c)
endif()

# ROM identity. rom_identity.txt is the one place the digests live: this turns
# it into snesrecomp_rom_identity.h for src/main.c, and tools/regen.sh,
# scripts/package_release.sh and the release workflow read the same file, so a
# revision bump is one edit there rather than four in agreement.
snesrecomp_rom_identity(ZeldaALttPSNESRecomp ${CMAKE_SOURCE_DIR}/rom_identity.txt)

# Scoped to main.c, which is the only consumer (host_report_init).
set_property(SOURCE src/main.c APPEND PROPERTY COMPILE_DEFINITIONS
    SNESRECOMP_BUILD_VERSION="${SNESRECOMP_BUILD_VERSION}")

# What the framework's own code reads for the same two facts. Kept as separate
# names from SNESRECOMP_BUILD_VERSION above because the runner sources are
# compiled for every target and cannot use a per-source property.
target_compile_definitions(ZeldaALttPSNESRecomp PRIVATE
    SNES_GAME_VERSION="${SNESRECOMP_BUILD_VERSION}"
    SNES_GAME_TITLE="The Legend of Zelda: A Link to the Past")

# Dear ImGui pre-boot launcher (GUI) from the shared recomp-ui repo. One call
# wires sources/includes/RECOMP_LAUNCHER define/CXX/asset-staging (same Zelda
# boxart; recomp-ui's default pad.tga is already the SNES pad).
# CONSOLE is required by recomp-ui: it selects exactly one validated
# assets/consoles/<id>/ manifest, so an SNES game cannot silently stage N64
# cartridge or NES pad art next to the exe.
recomp_target_launcher_ui(ZeldaALttPSNESRecomp
    CONSOLE snes
    BOXART ${CMAKE_SOURCE_DIR}/recomp/launcher/boxart.tga)

# Generate & rebuild from inside the launcher: pick ROM -> recompile locally
# -> cmake --build -> relaunch. This is what makes a SETUP PACK usable: the
# release zip ships a binary with no recompiled code, and the wizard is the
# only path from there to a playable build on the player's own ROM. The
# implementation is the framework's and this project contributes no
# configuration (snesrecomp_codegen_host_autowire in src/main.c derives the
# paths from scaffolder convention and the running executable's own name).
# recomp-ui/src is on the include path because the host speaks
# RecompLauncherCGameInfo.
target_sources(ZeldaALttPSNESRecomp PRIVATE
    ${SNESRECOMP_ROOT}/host/snesrecomp_codegen_host.c)
target_include_directories(ZeldaALttPSNESRecomp PRIVATE
    ${SNESRECOMP_ROOT}/host
    ${RECOMP_UI_ROOT}/src)

target_include_directories(ZeldaALttPSNESRecomp PRIVATE
    # NOT ${CMAKE_SOURCE_DIR}. The repo root holds VERSION, and on a
    # case-insensitive filesystem (Windows, and macOS by default) an -I of the
    # root makes libc++'s `#include <version>` read that file instead of the
    # standard header -- the build then dies inside <cmath> on a line that says
    # "0.1.0", with nothing in the message naming the include path. The only
    # thing that needed the root was src/opengl.c's
    # "third_party/gl_core/gl_core_3_1.h"; that include is now relative to the
    # third_party directory named below.
    ${CMAKE_SOURCE_DIR}/third_party
    # common_rtl.c includes "config.h": the desktop key-map / Config contract
    # a host is expected to supply. The framework's shared version is used
    # here; copy it into src/ and drop this line once this game needs its own.
    ${SNESRECOMP_ROOT}/runner/src/desktop
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/recomp
    ${SNESRECOMP_RUNNER_INCLUDE_DIRS}
)
# SDL3 by default, SDL2 via -DSNESRECOMP_SDL_BACKEND=SDL2. Replaces the game's
# own find_package(SDL2)/SDL2_LIBRARIES wiring: recomp-ui now compiles its SDL3
# backend unconditionally, so hardcoding SDL2 here put two SDL majors in one
# binary. No SDLmain either way (Zelda provides a plain main()).
# The framework's config / [KeyMap] implementation, replacing the fork this
# port carried in src/config.c. That fork was a copy of this file, and a copy
# cannot inherit what is added to the original: the hotkeys the framework grew
# since -- ToggleWidescreen, SaveStateMenu, Rewind -- did not exist in this
# build at all, because kKeys_* is declared in the header the implementation
# comes with. It also restores SourceP1/SourceP2 (per-seat input source, which
# is what makes "Player 2: Keyboard" in the launcher do anything) and
# DisplayAspect.
snesrecomp_target_mmx_config(ZeldaALttPSNESRecomp)
snesrecomp_target_sdl(ZeldaALttPSNESRecomp)
snesrecomp_target_post_mortem(ZeldaALttPSNESRecomp)
snesrecomp_target_glsl_shader(ZeldaALttPSNESRecomp)

# prod-vs-debug via the single SNESRECOMP_ENABLE_TRACE switch (runner.cmake,
# default OFF). Prod: no debug_server.c / TCP server / rings. Debug:
# -DSNESRECOMP_ENABLE_TRACE=ON. tools/build-linux.sh --config {prod,debug} sets it.
if(SNESRECOMP_ENABLE_TRACE)
    set(_ZELDA_TRACE 1)
else()
    set(_ZELDA_TRACE 0)
endif()
target_compile_definitions(ZeldaALttPSNESRecomp PRIVATE
    SMW_DISABLE_LM=1
    SYSTEM_VOLUME_MIXER_AVAILABLE=0   # Win32 WASAPI volume control excluded
    SNESRECOMP_TRACE=${_ZELDA_TRACE}  # TCP debug server: 0 in prod, 1 in debug
    SNESRECOMP_REVERSE_DEBUG=0        # raw WRAM stores; matches non-debug generated C
)

# The recompiled C is machine-generated; quiet its warnings and tolerate the
# K&R-style implicit declarations clang rejects by default.
if(NOT MSVC)
    target_compile_options(ZeldaALttPSNESRecomp PRIVATE
        -w -Wno-implicit-function-declaration)
endif()

# SDL is linked by snesrecomp_target_sdl() above, for whichever major is chosen.
# SNESRECOMP_RUNNER_LIBRARIES is what runner.cmake decided the runner sources
# need (threads, dl, sockets); this target compiled those sources all along
# without asking for their libraries, which held only because something else
# happened to pull the same ones in.
target_link_libraries(ZeldaALttPSNESRecomp PRIVATE
    OpenGL::GL
    ${SNESRECOMP_RUNNER_LIBRARIES}
)

if(WIN32)
    # Trace/debug-server sockets and post-mortem symbolization.
    target_link_libraries(ZeldaALttPSNESRecomp PRIVATE ws2_32)
endif()

# The mod catalog. The framework owns the staged layout
# (mods/preloaded/packages beside the executable) and verifies it after
# every build, so a rename touches snesrecomp/runner/runner.cmake and
# nothing here. Pass NONE instead of a directory if this title ships
# no catalog.
snesrecomp_target_mod_catalog(ZeldaALttPSNESRecomp
    "${CMAKE_SOURCE_DIR}/mods/preloaded")
