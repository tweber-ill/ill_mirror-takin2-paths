#!/bin/bash
#
# @author Tobias Weber <tweber@ill.fr>
# @date apr-2021
# @license GPLv3, see 'LICENSE' file
#

echo -e "--------------------------------------------------------------------------------"
echo -e "Building main binary..."
echo -e "--------------------------------------------------------------------------------"

rm -rfv build
mkdir build
pushd build

if ! cmake -DUSE_LAPACK=False -DUSE_QT6=False -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2" -DCMAKE_VERBOSE_MAKEFILE=False ..; then
	echo -e "cmake failed."
	exit -1
fi

if ! make -j4; then
	echo -e "make failed."
	exit -1
fi

strip -v taspaths
popd
echo -e "--------------------------------------------------------------------------------"



echo -e "--------------------------------------------------------------------------------"
echo -e "Building test binaries..."
echo -e "--------------------------------------------------------------------------------"

rm -rfv tests/build
mkdir tests/build
pushd tests/build

if ! cmake -DUSE_LAPACK=False -DUSE_QT6=False -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2" -DCMAKE_VERBOSE_MAKEFILE=False ..; then
	echo -e "cmake failed."
	exit -1
fi

if ! make -j4; then
	echo -e "make failed."
	exit -1
fi

strip -v lines
strip -v hull
popd
echo -e "--------------------------------------------------------------------------------"
