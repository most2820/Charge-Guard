#!/bin/bash
set -e

export DEVKITPRO=/opt/devkitpro
export DEVKITA64=$DEVKITPRO/devkitA64
export PATH=$DEVKITA64/bin:$DEVKITPRO/tools/bin:$PATH

PROJDIR="$(pwd)"
OUT="$PROJDIR/out"
rm -rf "$OUT"

echo "=== Building overlay ==="
rm -rf /tmp/_overlay
cp -r "$PROJDIR/overlay" /tmp/_overlay
rm -rf /tmp/_overlay/build
make -C /tmp/_overlay -j4
mkdir -p "$OUT/switch/.overlays"
cp /tmp/_overlay/ChargeGuard.ovl "$OUT/switch/.overlays/ChargeGuard.ovl"
rm -rf /tmp/_overlay

echo "=== Building sysmodule ==="
cp -r "$PROJDIR/sysmodule" /tmp/_sysmod
rm -rf /tmp/_sysmod/build /tmp/_sysmod/config
mv /tmp/_sysmod/sysmodule.json /tmp/_sysmod/_sysmod.json 2>/dev/null || true
make -C /tmp/_sysmod -j4
mkdir -p "$OUT/atmosphere/contents/010000000000BC01/flags"
cp /tmp/_sysmod/_sysmod.nsp "$OUT/atmosphere/contents/010000000000BC01/exefs.nsp"
touch "$OUT/atmosphere/contents/010000000000BC01/flags/boot2.flag"
rm -rf /tmp/_sysmod

mkdir -p "$OUT/config/charge-guard"

echo "=== Done ==="
echo ""
echo "Copy out/ to SD card root:"
echo ""
echo "  SD:/"
echo "  ├── atmosphere/contents/010000000000BC01/"
echo "  │   ├── exefs.nsp"
echo "  │   └── flags/boot2.flag"
echo "  ├── config/charge-guard/"
echo "  └── switch/.overlays/ChargeGuard.ovl"
echo ""
echo "Reboot your Switch after copying."
