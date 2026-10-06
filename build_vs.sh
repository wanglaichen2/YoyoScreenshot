#!/usr/bin/env bash
# 悠悠截图 — MSVC / MSBuild
# 用法: ./build_vs.sh [Debug|Release] [x64] [--rebuild]

set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
if command -v cygpath >/dev/null 2>&1; then
	ROOT_WIN="$(cygpath -w "$ROOT")"
else
	ROOT_WIN="$ROOT"
fi

BUILD_TYPE=Release
ARCH=x64
REBUILD=0

while [[ $# -gt 0 ]]; do
	case "$(echo "$1" | tr '[:upper:]' '[:lower:]')" in
		debug) BUILD_TYPE=Debug ;;
		release) BUILD_TYPE=Release ;;
		x64) ARCH=x64 ;;
		--rebuild|rebuild) REBUILD=1 ;;
		-h|--help) echo "Usage: $0 [Debug|Release] [x64] [--rebuild]"; exit 0 ;;
		*) echo "[ERROR] Unknown: $1"; exit 1 ;;
	esac
	shift
done

PROJ_WIN="${ROOT_WIN}\\YoyoScreenshot.vcxproj"
[[ -f "$ROOT/YoyoScreenshot.vcxproj" ]] || { echo "[ERROR] missing YoyoScreenshot.vcxproj"; exit 1; }

MSBUILD_WIN="C:\\Program Files\\Microsoft Visual Studio\\18\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe"
if [[ ! -f "/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe" ]]; then
	VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
	MSBUILD_WIN="$("$VSWHERE" -latest -requires Microsoft.Component.MSBuild -find "MSBuild/**/Bin/MSBuild.exe" | head -n1 | tr -d '\r')"
fi

echo "[INFO] MSBuild: $MSBUILD_WIN"
echo "[INFO] Config: $BUILD_TYPE|$ARCH"

TARGET=Build
[[ "$REBUILD" -eq 1 ]] && TARGET=Rebuild

powershell.exe -NoProfile -Command \
	"& '$MSBUILD_WIN' '$PROJ_WIN' -m -nologo -p:Configuration=$BUILD_TYPE -p:Platform=$ARCH -t:$TARGET"

OUT="$ROOT/Exec/${BUILD_TYPE}/${ARCH}/YoyoScreenshot/YoyoScreenshot.exe"
echo "[OK] $OUT"
[[ -f "$OUT" ]] || { echo "[ERROR] exe missing"; exit 1; }
