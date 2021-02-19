#!/bin/bash

if [[ ! -z "$1" ]]
then
	shift
	if command -v ninja &> /dev/null
	then
		GEN=" -G Ninja "
	else
		GEN=""
	fi
	cmake $GEN -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "-DCMAKE_BUILD_TYPE=$1" "$@" -S . -B build
fi

cmake --build build
