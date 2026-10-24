#!/bin/bash -e
#
# MSYS2 no longer provides 32-bit (MINGW32) packages for allegro, enet and
# libmad. Build the latest release of each from its own source and install
# it into the MINGW32 prefix. Run in a MINGW32 shell with git, cmake,
# ninja and the dependencies installed (see windows-x86-build.yml).
#
# No version is pinned: each run uses the newest release tag, so a new
# release that no longer builds for 32-bit Windows fails the build.
#
# Usage: scripts/build-windows-x86-deps.sh versions
#            print the versions that would be built (for a cache key)
#        scripts/build-windows-x86-deps.sh <dir>
#            build into <dir> (kept if it already holds these versions),
#            then install

ALLEGRO_GIT=https://github.com/liballeg/allegro5
ENET_GIT=https://github.com/lsalzman/enet
# libmad's own development stopped at 0.15.1b in 2004; Tenacity's fork is
# where it is maintained now.
LIBMAD_GIT=https://codeberg.org/tenacityteam/libmad

latest() {
	local tag
	tag=$(git ls-remote --tags --refs --sort=-v:refname "$1.git" |
		sed 's|.*refs/tags/||' | grep -E '^v?[0-9]+(\.[0-9]+)+$' | head -n 1)
	[ -n "$tag" ] || { echo "No release tag found in $1" >&2; exit 1; }
	echo "$tag"
}

ALLEGRO_TAG=$(latest $ALLEGRO_GIT)
ENET_TAG=$(latest $ENET_GIT)
LIBMAD_TAG=$(latest $LIBMAD_GIT)
VERSIONS="allegro-$ALLEGRO_TAG enet-$ENET_TAG libmad-$LIBMAD_TAG"

if [ "$1" = versions ]; then
	echo "$VERSIONS" | tr ' ' '_'
	exit 0
fi

OUT=$(realpath -m "${1:-deps-x86}")
STAGE="$OUT/stage"

# Symbolic links (Allegro's README.md) are left out: tar cannot always
# create them on Windows.
fetch() {
	rm -rf "$2"
	mkdir -p "$2"
	curl -fsSL "$1" | tar -xz --strip-components=1 --exclude=README.md -C "$2"
}

build() {
	local src=$1
	shift
	MSYS2_ARG_CONV_EXCL="-DCMAKE_INSTALL_PREFIX=" \
		cmake -G Ninja -S "$src" -B "$src/build" \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" \
		"$@"
	cmake --build "$src/build"
	DESTDIR=$(cygpath -m "$STAGE") cmake --install "$src/build"
}

if [ "$(cat "$OUT/VERSIONS" 2>/dev/null)" != "$VERSIONS" ]; then
	echo "Building $VERSIONS"
	rm -rf "$OUT"
	mkdir -p "$OUT"
	SRC="$OUT/src"

	fetch "$LIBMAD_GIT/archive/$LIBMAD_TAG.tar.gz" "$SRC/libmad"
	# libmad picks its fixed-point maths from CMAKE_SYSTEM_PROCESSOR,
	# which is the 64-bit host's (AMD64) here, so it would fall back to
	# the less accurate generic version. Use the x86 one, as configure
	# chooses for MSYS2's libmad 0.15.1b.
	build "$SRC/libmad" -DEXAMPLE=OFF -DFPM_INTEL=ON -DFPM_DEFAULT=OFF
	grep -q '^#define FPM_INTEL' "$SRC/libmad/build/mad.h" ||
		{ echo "libmad was not built with FPM_INTEL" >&2; exit 1; }
	if grep -q -E '^#define FPM_(DEFAULT|64BIT)' "$SRC/libmad/build/mad.h"; then
		echo "libmad was not built with only FPM_INTEL" >&2
		exit 1
	fi

	fetch "$ENET_GIT/archive/refs/tags/$ENET_TAG.tar.gz" "$SRC/enet"
	build "$SRC/enet"
	# ENet installs its static library to lib/static.
	mv "$STAGE$MINGW_PREFIX"/lib/static/libenet.a "$STAGE$MINGW_PREFIX"/lib/
	rmdir "$STAGE$MINGW_PREFIX"/lib/static

	# Dune Dynasty does not use mod music (DUMB, OpenMPT), video (Theora),
	# OpenAL or PhysicsFS, whose libraries MINGW32 no longer has.
	fetch "$ALLEGRO_GIT/archive/refs/tags/$ALLEGRO_TAG.tar.gz" "$SRC/allegro"
	build "$SRC/allegro" \
		-DWANT_DOCS=OFF -DWANT_EXAMPLES=OFF -DWANT_TESTS=OFF -DWANT_DEMO=OFF \
		-DWANT_DUMB=OFF -DWANT_OPENMPT=OFF -DWANT_VIDEO=OFF \
		-DWANT_OPENAL=OFF -DWANT_PHYSFS=OFF \
		-DINSTALL_PKG_CONFIG_FILES=ON

	rm -rf "$SRC"
	echo "$VERSIONS" > "$OUT/VERSIONS"
fi

echo "Installing $VERSIONS"
cp -r "$STAGE$MINGW_PREFIX"/. "$MINGW_PREFIX"/
