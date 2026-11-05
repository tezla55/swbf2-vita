cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(swbf2render)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Star Wars Battlefront II")
set(VITA_TITLEID  "SWBFVITA2")
set(VITA_VERSION  "01.00")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")

# MILESTONE 110 (2026-08-30): request the Vita's EXTENDED memory budget.
#
# This project has treated ~15MB as a hard ceiling since M99 and designed
# around it (the AI weapon meshes were abandoned because of it). It was
# never a hardware limit - it was two defaults we never overrode:
#
#  1. vitaGL's own `vglInit(pool)` forwards ram_threshold = 0x1000000, and
#     vglInitWithCustomThreshold then hands vitaGL (free_user - 16MB). The
#     "15360 KB free" reading every log has shown IS that 16MB threshold.
#  2. A Vita app only gets the LARGER user-RAM budget if its SFO asks for
#     it. ATTRIBUTE2=12 is that request - the same one the large Vita
#     ports (OpenMW, the vitaQuake family) rely on. Without it we were
#     running in the default budget the whole time.
#
# Measured before this change: user=43MB free before vglInit (on top of
# the 200MB newlib heap), cdram=112MB, phycont=26MB.
set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(
  $ENV{VITASDK}/arm-vita-eabi/include/freetype2  # MILESTONE 139
)

# LibSWBF2 (our fork, a git submodule) is built as part of this project, so a
# library change can never link stale (the old separate build-vita did).
add_subdirectory(libs/LibSWBF2 ${CMAKE_BINARY_DIR}/LibSWBF2)
include_directories(libs/LibSWBF2/include)

set(LOCAL_VITA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_local/vita)
if(EXISTS ${LOCAL_VITA_DIR}/sce_sys/icon0.png)
  set(SCE_SYS_DIR ${LOCAL_VITA_DIR}/sce_sys)
else()
  set(SCE_SYS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vita/sce_sys)
endif()
set(LOCAL_VPK_FILES)
if(EXISTS ${LOCAL_VITA_DIR}/libshacccg.suprx)
  list(APPEND LOCAL_VPK_FILES FILE ${LOCAL_VITA_DIR}/libshacccg.suprx sce_module/libshacccg.suprx)
endif()
foreach(font ZeroesTwo.ttf ZeroesThree.ttf)
  if(EXISTS ${LOCAL_VITA_DIR}/${font})
    list(APPEND LOCAL_VPK_FILES FILE ${LOCAL_VITA_DIR}/${font} ${font})
  else()
    message(WARNING "_local/vita/${font} missing: the UI text will not render (supply it in _local/vita/)")
  endif()
endforeach()

# MILESTONE 15 (2026-08-25): vendored lua5.0-swbf2-x64 (github.com/
# Ben1138/lua5.0-swbf2-x64, GPLv3, pulled via SWBF2Phoenix's own
# submodule reference) - a portable, standard-layout Lua 5.0.3 C source
# tree specifically patched to load SWBF2's exact custom bytecode format
# (a fixed 32-bit lua_size_t regardless of host word size - see
# include/lua.h - rather than a generic Lua 5.0.3 build, which wouldn't
# necessarily agree on that format). Compiled clean against the vitasdk
# ARM cross-compiler with zero source changes needed (confirmed via a
# standalone compile-every-file test before wiring this in) - Lua's C
# source is famously portable, and Vita's own 32-bit ARM word size
# actually needs LESS special-casing here than the x64 port this fork
# was originally built for. Excludes loadlib.c (needs dlfcn/dlopen -
# real dynamic library loading, not available or wanted in this
# sandboxed context and not something SWBF2 script logic needs).
# ldump.c (the bytecode WRITER - this project only ever loads pre-
# compiled bytecode, never writes any back out) IS included despite
# being functionally unused - lapi.c's lua_dump() references luaU_dump
# unconditionally, so leaving it out is a link error, not a real
# savings; simpler to keep it than patch vendored source.
add_library(lua50 STATIC
  libs/lua50/src/ldump.c
  libs/lua50/src/lapi.c
  libs/lua50/src/lauxlib.c
  libs/lua50/src/lbaselib.c
  libs/lua50/src/lcode.c
  libs/lua50/src/ldblib.c
  libs/lua50/src/ldebug.c
  libs/lua50/src/ldo.c
  libs/lua50/src/lfunc.c
  libs/lua50/src/lgc.c
  libs/lua50/src/liolib.c
  libs/lua50/src/llex.c
  libs/lua50/src/lmathlib.c
  libs/lua50/src/lmem.c
  libs/lua50/src/lobject.c
  libs/lua50/src/lopcodes.c
  libs/lua50/src/lparser.c
  libs/lua50/src/lstate.c
  libs/lua50/src/lstring.c
  libs/lua50/src/lstrlib.c
  libs/lua50/src/ltable.c
  libs/lua50/src/ltablib.c
  libs/lua50/src/ltm.c
  libs/lua50/src/lundump.c
  libs/lua50/src/lvm.c
  libs/lua50/src/lzio.c
)
target_include_directories(lua50 PUBLIC
  libs/lua50/include
  libs/lua50/src
)

add_executable(${PROJECT_NAME}
  src/main.cpp
)

# MILESTONE 482: the shell's Bink flyover movies. FFmpeg 6.1.2 built with
# ONLY the bink demuxer + bink/binkaudio decoders (LGPL 2.1+, source and the
# configure line in third_party/ - see THIRD_PARTY_NOTICES.md). Its headers
# come first so the vitasdk's own libavcodec headers are never picked up.
set(FFBINK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ffmpeg-bink)
target_include_directories(${PROJECT_NAME} BEFORE PRIVATE ${FFBINK_DIR}/include)
target_link_libraries(${PROJECT_NAME}
  ${FFBINK_DIR}/lib/libavformat.a
  ${FFBINK_DIR}/lib/libavcodec.a
  ${FFBINK_DIR}/lib/libavutil.a
)

# MILESTONE 139 (2026-08-31): FreeType ships with the vitasdk, so the real
# TTF can be rasterised ON DEVICE at load. That retires both dead ends the
# font work hit: reverse-engineering the .fff glyph encoding, and decoding
# the core.lvl font atlases (which LibSWBF2 reports as 1 mipmap but returns
# no data for at any level).
target_link_libraries(${PROJECT_NAME}
  freetype
  png16   # freetype's sfnt module references libpng
  bz2     # ...and bzip2 for compressed font tables
  z
  lua50
  LibSWBF2
  fmt
  glm
  vitaGL
  vitashark
  SceShaccCgExt
  SceShaccCg_stub
  taihen_stub
  mathneon
  pthread
  SceGxm_stub
  SceDisplay_stub
  SceCommonDialog_stub
  SceSysmodule_stub
  SceAppMgr_stub
  SceKernelDmacMgr_stub
  SceCtrl_stub
  SceTouch_stub          # MILESTONE 199: offhand weapon on the front touch panel
  SceLibKernel_stub
  SceNet_stub
  SceNetCtl_stub
  SceAudio_stub          # MILESTONE 249: real sound - sceAudioOut
  ScePower_stub          # MILESTONE 484: the CPU/GPU clocks
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${SCE_SYS_DIR}/icon0.png sce_sys/icon0.png
  FILE ${SCE_SYS_DIR}/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE ${SCE_SYS_DIR}/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE ${SCE_SYS_DIR}/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
  # Bundling the shader compiler directly into the VPK instead of relying
  # on a shared copy at ur0:data/ - matches the pattern used by working
  # vitaGL ports like vitaQuakeII (app0:/sce_module/libshacccg.suprx).
  # Sidesteps every uncertainty around a shared system location (transfer
  # correctness, exact partition, permissions) since our own bundled copy
  # is guaranteed byte-for-byte correct (hash-verified against the
  # canonical release before this build).
  ${LOCAL_VPK_FILES}
  # M7: terrain's custom Cg shader pair, loaded at runtime from app0: -
  # matches the loading convention used by vitaGL's own official samples
  # (samples/illumination_models/main.cpp: fopen("app0:name.vert"/.frag)).
# MILESTONE 143: the REAL menu chrome. MILESTONE 452: no longer packaged -
# it is shipped SDK art (data/Common/interface), and the VPK carries code
# only. push_vita.sh copies the console originals to ux0:data/swbf2/console_ui/.
  FILE src/shaders/terrain.vert terrain.vert
  FILE src/shaders/terrain.frag terrain.frag
  # M7 round 2: multi-layer blend shader pair.
  FILE src/shaders/terrain_blend.vert terrain_blend.vert
  FILE src/shaders/terrain_blend.frag terrain_blend.frag
)
