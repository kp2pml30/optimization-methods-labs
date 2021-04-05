#!/bin/bash
set -e
BUILDDIR="/tmp/build/methopts"
SRCDIR=$(readlink -f $(dirname "${BASH_SOURCE[0]}"))

mkdir -p "$BUILDDIR"
cd "$BUILDDIR"

if [[ -n "$1" ]]
then
	BTYPE=$1
	shift
	if command -v ninja &> /dev/null
	then
		GEN=" -G Ninja "
	fi
	cmake $GEN -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "-DCMAKE_BUILD_TYPE=$BTYPE" "$@" "$SRCDIR"
	if [ ! -L "$SRCDIR/compile_commands.json" ]
	then
		ln -s "$BUILDDIR/compile_commands.json" "$SRCDIR/compile_commands.json"
	fi
fi

cmake --build .
