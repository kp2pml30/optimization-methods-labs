#!/bin/bash
BUILDDIR="/tmp/build/methopts"

mkdir -p "$BUILDDIR"

if [[ ! -z "$1" ]]
then
	BTYPE=$1
	shift
	if command -v ninja &> /dev/null
	then
		GEN=" -G Ninja "
	else
		GEN=""
	fi
	cmake $GEN -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "-DCMAKE_BUILD_TYPE=$BTYPE" "$@" -S . -B "$BUILDDIR"
	if [ ! -L compile_commands.json ]
	then
		ln -s "$BUILDDIR/compile_commands.json"
	fi
fi

cmake --build "$BUILDDIR"
