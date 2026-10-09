#!/usr/bin/env bash
# 悠悠截图 — 编译 Release 并打 MSIX
# 用法: ./build_msix.sh [--no-build] [--rebuild] [1.0.4.0]
# 产物: Pack/Output/YouYouJieTu_<ver>_x64.msix

set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
NO_BUILD=0
REBUILD=0
APP_VER="1.0.4.0"

while [[ $# -gt 0 ]]; do
	case "$(echo "$1" | tr '[:upper:]' '[:lower:]')" in
		--no-build|nobuild) NO_BUILD=1 ;;
		--rebuild|rebuild) REBUILD=1 ;;
		-h|--help)
			echo "Usage: $0 [--no-build] [--rebuild] [1.0.4.0]"
			echo "  --no-build   skip MSVC build, pack existing Release exe"
			echo "  --rebuild    force Rebuild before pack"
			echo "  version      MSIX four-part version (default 1.0.4.0)"
			exit 0
			;;
		[0-9]*.[0-9]*) APP_VER="$1" ;;
		*) echo "[ERROR] Unknown: $1"; exit 1 ;;
	esac
	shift
done

EXE="$ROOT/Exec/Release/x64/YoyoScreenshot/YoyoScreenshot.exe"
PACK_BAT="$ROOT/Pack/pack_msix.bat"

[[ -f "$PACK_BAT" ]] || { echo "[ERROR] missing $PACK_BAT"; exit 1; }

if [[ "$NO_BUILD" -eq 0 ]]; then
	ARGS=(Release x64)
	[[ "$REBUILD" -eq 1 ]] && ARGS+=(--rebuild)
	echo "[INFO] Build: ./build_vs.sh ${ARGS[*]}"
	"$ROOT/build_vs.sh" "${ARGS[@]}"
fi

[[ -f "$EXE" ]] || {
	echo "[ERROR] missing $EXE"
	echo "        Run: ./build_vs.sh Release x64"
	exit 1
}

if command -v cygpath >/dev/null 2>&1; then
	PACK_WIN="$(cygpath -w "$ROOT/Pack")"
else
	PACK_WIN="$ROOT/Pack"
fi

echo "[INFO] Pack MSIX version=$APP_VER ..."
powershell.exe -NoProfile -Command \
	"Set-Location -LiteralPath '$PACK_WIN'; & .\pack_msix.bat '$APP_VER'; exit \$LASTEXITCODE"

MSIX="$ROOT/Pack/Output/YouYouJieTu_${APP_VER}_x64.msix"
echo "[OK] $MSIX"
[[ -f "$MSIX" ]] || { echo "[ERROR] msix missing"; exit 1; }
