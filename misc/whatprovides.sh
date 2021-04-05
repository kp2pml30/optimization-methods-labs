#!/bin/bash

phelp () {
	echo "whatprovides.sh <arch|deb> <pathtoexe>"
	exit 1
}

if [ $# -ne 2 ]
then
	phelp
fi

ARCH=$1
shift
EXE=$1
shift

case $ARCH in
arch)
	PROVIDER="pacman"
	PROVIDERARG="-Qoq"
	;;
deb)
	PROVIDER="dpkg"
	PROVIDERARG="-S"
	;;
*)
	phelp
esac

RESULT=$(ldd "$EXE" | awk '/=> [^ ]+ \(/{ print $3; }')
ARR=""
NL=$'\n'
for i in "$RESULT"
do
	ARR="$ARR$NL$($PROVIDER $PROVIDERARG $i)"
done
echo "$ARR" | sort | uniq -u

