#!/bin/bash
# Build SoftProjector from source/ and run it on a Mac for testing.
# Works on a copy of spData.sqlite, so testing never changes the church database in the repo.
#
#   tools/run-on-mac.sh             build (only what changed) and run
#   tools/run-on-mac.sh --reset-db  same, but start over from a fresh copy of the database
set -euo pipefail

QT_DIR="${QT_DIR:-$HOME/Qt/5.15.2/clang_64}"
REPO="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$REPO/source"
APP="$SRC/mac_build/bin/SoftProjector.app"
BIN_DIR="$APP/Contents/MacOS"

if [ ! -x "$QT_DIR/bin/qmake" ]; then
  echo "Qt 5.15.2 not found in $QT_DIR. Install it with:"
  echo "  python3 -m pip install aqtinstall && python3 -m aqt install-qt mac desktop 5.15.2 clang_64 -O ~/Qt"
  exit 1
fi

cd "$SRC"
[ -f Makefile ] || "$QT_DIR/bin/qmake" CONFIG+=sdk_no_version_check softProjector.pro
make -j"$(sysctl -n hw.ncpu)"

mkdir -p "$BIN_DIR/translations"
for ts in translations/softpro_*.ts; do
  "$QT_DIR/bin/lrelease" -silent "$ts" -qm "$BIN_DIR/translations/$(basename "${ts%.ts}").qm"
done
cp translations/flag_*.png "$BIN_DIR/translations/"
rm -rf "$BIN_DIR/DataFixes"
cp -R "$REPO/DataFixes" "$BIN_DIR/"

if [ "${1:-}" = "--reset-db" ] || [ ! -f "$BIN_DIR/spData.sqlite" ]; then
  cp "$REPO/spData.sqlite" "$BIN_DIR/spData.sqlite"
  # The church PC sends the projector to its 3rd monitor; on a Mac use the 2nd one and no extra output
  sqlite3 "$BIN_DIR/spData.sqlite" "UPDATE Settings SET sets = replace(replace(sets,
    'displayScreen = 2', 'displayScreen = 1'), 'displayScreen2 = 1', 'displayScreen2 = -1') WHERE type = 'general'"
  echo "Using a fresh copy of spData.sqlite"
fi

open "$APP"
