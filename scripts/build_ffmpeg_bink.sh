#!/bin/bash
# Downloads FFmpeg 6.1.2 (LGPL-2.1+) and builds ONLY the Bink demuxer and decoders for the
# Vita, into third_party/ffmpeg-bink/ (used by the shell's background movies). Runs in the
# vitasdk Docker image. See third_party/THIRD_PARTY_NOTICES.md.
set -e
REPO="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE="${VITASDK_IMAGE:-vitasdk/vitasdk:2026.08}"
cd "$REPO/third_party"
if [ ! -d ffmpeg-6.1.2 ]; then
    curl -L -o ffmpeg-6.1.2.tar.xz https://ffmpeg.org/releases/ffmpeg-6.1.2.tar.xz
    tar xf ffmpeg-6.1.2.tar.xz && rm ffmpeg-6.1.2.tar.xz
fi
docker run --rm -u "$(id -u)":"$(id -g)" -v "$REPO:/workspace" -w /workspace/third_party/ffmpeg-6.1.2 "$IMAGE" sh -c '
./configure --prefix=/workspace/third_party/ffmpeg-bink \
  --enable-cross-compile --cross-prefix=arm-vita-eabi- --arch=arm --cpu=cortex-a9 --target-os=none \
  --disable-everything --enable-decoder=bink --enable-decoder=binkaudio_rdft --enable-decoder=binkaudio_dct --enable-demuxer=bink \
  --disable-programs --disable-doc --disable-network --disable-avdevice --disable-swresample --disable-swscale --disable-postproc --disable-avfilter \
  --enable-static --disable-shared --enable-neon --disable-debug --disable-runtime-cpudetect --disable-pthreads \
  --disable-iconv --disable-zlib --disable-bzlib --disable-lzma --disable-xlib --disable-sdl2 --disable-hwaccels --disable-vulkan \
  --extra-cflags="-O2 -mfpu=neon -mfloat-abi=hard -ffast-math -Wno-error=incompatible-pointer-types -Wno-incompatible-pointer-types -Wno-error=int-conversion -Wno-error=implicit-function-declaration" \
  && make -j"$(nproc)" && make install'
echo "== built third_party/ffmpeg-bink =="
