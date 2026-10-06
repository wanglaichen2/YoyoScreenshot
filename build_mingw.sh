#!/usr/bin/env bash
# 悠悠截图 — MinGW / CMake（静态库）
# 用法: ./build_mingw.sh [Debug|Release] [x64] [--rebuild]

set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
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

BUILD_DIR="$ROOT/temp/YoyoScreenshot/mingw/${BUILD_TYPE}/${ARCH}"

find_tool() {
	local name="$1"
	shift
	local c
	for c in "$@"; do
		if [[ -x "$c" ]]; then
			echo "$c"
			return 0
		fi
	done
	if command -v "$name" >/dev/null 2>&1; then
		command -v "$name"
		return 0
	fi
	return 1
}

CMAKE="$(find_tool cmake \
	"/c/Program Files/CMake/bin/cmake.exe" \
	"/c/Program Files/JetBrains/CLion 2024.3/bin/cmake/win/x64/bin/cmake.exe" \
	"/d/Program Files/JetBrains/CLion 2024.3/bin/cmake/win/x64/bin/cmake.exe" \
	|| true)"
CMAKE="${CMAKE:-}"

GXX="$(find_tool g++ \
	"/mingw64/bin/g++.exe" \
	"/c/mingw64/bin/g++.exe" \
	"/d/mingw64/bin/g++.exe" \
	"/c/msys64/mingw64/bin/g++.exe" \
	"/d/msys64/mingw64/bin/g++.exe" \
	|| true)"
[[ -n "${GXX:-}" ]] || { echo "[ERROR] g++ not found"; exit 1; }
if [[ "$GXX" != *.exe && -f "${GXX}.exe" ]]; then
	GXX="${GXX}.exe"
fi

GXX_DIR="$(dirname "$GXX")"
MAKE=""
for c in "$GXX_DIR/mingw32-make.exe" "$GXX_DIR/make.exe"; do
	[[ -x "$c" ]] && MAKE="$c" && break
done
if [[ -z "$MAKE" ]]; then
	MAKE="$(find_tool mingw32-make || true)"
fi
[[ -n "${MAKE:-}" ]] || { echo "[ERROR] mingw32-make not found"; exit 1; }

RC=""
[[ -x "$GXX_DIR/windres.exe" ]] && RC="$GXX_DIR/windres.exe"

to_cmake_path() {
	local p="$1"
	if command -v cygpath >/dev/null 2>&1; then
		cygpath -m "$p"
	else
		echo "$p"
	fi
}

export PATH="$GXX_DIR:${PATH:-}"

if [[ "$REBUILD" -eq 1 ]]; then
	rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

echo "[INFO] CMake : ${CMAKE:-none}"
echo "[INFO] g++   : $GXX"
echo "[INFO] make  : $MAKE"
echo "[INFO] Config: $BUILD_TYPE|$ARCH"

cfg_args=(
	-S "$(to_cmake_path "$ROOT")"
	-B "$(to_cmake_path "$BUILD_DIR")"
	-G "MinGW Makefiles"
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE"
	"-DCMAKE_MAKE_PROGRAM:FILEPATH=$(to_cmake_path "$MAKE")"
	"-DCMAKE_CXX_COMPILER:FILEPATH=$(to_cmake_path "$GXX")"
	-DCMAKE_POLICY_VERSION_MINIMUM=3.5
)
if [[ -n "$RC" ]]; then
	cfg_args+=("-DCMAKE_RC_COMPILER:FILEPATH=$(to_cmake_path "$RC")")
fi

if [[ -n "${CMAKE:-}" ]]; then
	echo "[1/2] Configuring CMake..."
	"$CMAKE" "${cfg_args[@]}"
	echo "[2/2] Building..."
	"$CMAKE" --build "$(to_cmake_path "$BUILD_DIR")" --target YoyoScreenshot -j
else
	echo "[INFO] cmake 未找到，改用 Makefile + mingw32-make"
	MAKE_FLAGS=("CFG=$BUILD_TYPE" "-j")
	[[ "$REBUILD" -eq 1 ]] && "$MAKE" -C "$ROOT" -f Makefile CFG="$BUILD_TYPE" clean
	"$MAKE" -C "$ROOT" -f Makefile "${MAKE_FLAGS[@]}"
fi

OUT="$ROOT/Exec/${BUILD_TYPE}/${ARCH}/YoyoScreenshot/YoyoScreenshot.exe"
echo "[OK] $OUT"
[[ -f "$OUT" ]] || { echo "[ERROR] exe missing"; exit 1; }
