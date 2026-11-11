#!/usr/bin/env bash
# Tracker firmware build/flash on the NUC (no sudo, self-contained toolchain).
# Usage: ./build_flash.sh build | flash <ip> | clean
set -euo pipefail
cd "$(dirname "$0")"
export PATH="$HOME/toolchain-env/bin:$HOME/pio-venv/bin:$HOME/.platformio/penv/bin:$HOME/.local/bin:$PATH"
export CC="$HOME/toolchain-env/bin/gcc" CXX="$HOME/toolchain-env/bin/g++" CMAKE_GENERATOR=Ninja
ENV_NAME="${TRACKER_ENV:-tracked_s3_ota}"
case "${1:-}" in
  build) pio run -e "$ENV_NAME" ;;
  flash) [ -n "${2:-}" ] || { echo "usage: $0 flash <ip>" >&2; exit 2; }
         pio run -e "$ENV_NAME" -t upload --upload-port "$2" ;;
  clean) pio run -e "$ENV_NAME" -t clean ;;
  *) echo "usage: $0 build | flash <ip> | clean   (env: TRACKER_ENV=$ENV_NAME)" >&2; exit 2 ;;
esac
