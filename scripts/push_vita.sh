#!/bin/bash
# Build the VPK in the vitasdk Docker image, then (optionally) install and launch it on a
# Vita running VitaCompanion (FTP on 1337, command server on 1338).
#
#   scripts/push_vita.sh              # build only -> build/swbf2render.vpk
#   scripts/push_vita.sh 192.168.0.7  # build, install, launch
#
# LibSWBF2 is built as part of the app (CMakeLists.txt add_subdirectory), so library
# changes always relink - no separate LibSWBF2 build step.
set -e
REPO="$(cd "$(dirname "$0")/.." && pwd)"
VITA_IP="$1"
TITLEID="SWBFVITA2"
IMAGE="${VITASDK_IMAGE:-vitasdk/vitasdk:2026.08}"

echo "== Building via $IMAGE =="
docker run --rm -u "$(id -u)":"$(id -g)" -v "$REPO:/workspace" -w /workspace "$IMAGE" \
    sh -c 'cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null && cmake --build build -j"$(nproc)"'

# Crash symbolication (vita-parse-core) needs the matching ELF; keep a copy if the folder exists.
[ -d "$HOME/vitacrash" ] && cp "$REPO/build/swbf2render" "$HOME/vitacrash/swbf2render.elf"

[ -z "$VITA_IP" ] && { echo "== Built $REPO/build/swbf2render.vpk (no IP given, not deployed) =="; exit 0; }

echo "== Pushing VPK to ux0:/data/ (FTP 1337) =="
curl --ftp-method nocwd -T "$REPO/build/swbf2render.vpk" "ftp://$VITA_IP:1337/ux0:/data/swbf2render.vpk"

# Game data is NOT pushed: the user copies the game to ux0:data/swbf2/GameData/ once;
# the app reads it in place.

echo "== Closing the running app, installing, launching $TITLEID =="
echo "destroy" | nc -w 5 "$VITA_IP" 1338; sleep 2
echo "vpk ux0:/data/swbf2render.vpk" | nc -w 8 "$VITA_IP" 1338; sleep 2
echo "launch $TITLEID" | nc -w 5 "$VITA_IP" 1338
echo "== Done. (scripts/netlog_listener.py shows the live log.) =="
