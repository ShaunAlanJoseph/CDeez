#!/bin/sh
# Builds an installable package from the current checkout, for handing to
# testers before there is a tagged release to build from.
#
# Everything lands in dist/, which is not tracked: the source tarball, the
# package, and makepkg's own src/ and pkg/ scratch directories.
#
# The commit is baked into the binary so `cdeez --version` identifies which
# build a bug report came from; a real release leaves that to the tag.

set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DIST=$ROOT/dist
PKGVER=$(sed -n 's/^pkgver=//p' "$ROOT/packaging/PKGBUILD")

mkdir -p "$DIST"

git -C "$ROOT" archive --format=tar.gz --prefix="CDeez-$PKGVER/" HEAD \
  -o "$DIST/cdeez-$PKGVER.tar.gz"

# The real PKGBUILD fetches a tagged tarball from GitHub; this copy builds the
# one we just made from the working tree.
sed 's|^source=.*|source=("$pkgname-$pkgver.tar.gz")|' \
  "$ROOT/packaging/PKGBUILD" > "$DIST/PKGBUILD"

REVISION=$(git -C "$ROOT" describe --always --dirty --abbrev=8)

cd "$DIST"
CDEEZ_REVISION="$REVISION" makepkg -f

printf '\nBuilt from %s\n' "$REVISION"
printf 'Install with:  sudo pacman -U %s\n' \
  "$(ls -t "$DIST"/*.pkg.tar.zst | head -1)"
