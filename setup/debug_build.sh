#!/bin/bash
#
# @author Tobias Weber <tweber@ill.fr>
# @date apr-2021
# @license GPLv3, see 'LICENSE' file
#
# -----------------------------------------------------------------------------
# TAS-Paths (part of the Takin software suite)
# Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL),
#                     Grenoble, France).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# -----------------------------------------------------------------------------
#

echo -e "--------------------------------------------------------------------------------"
echo -e "Building..."
echo -e "--------------------------------------------------------------------------------"

#export CXX=clang++-10
#export CXX=g++-10

rm -rfv build
mkdir build
pushd build

if ! cmake -DUSE_LAPACK=True -DUSE_QT6=False -DUSE_OVD=True -DUSE_CGAL=True \
	-DBUILD_TEST_TOOLS=False \
	-DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=True ..
then
	echo -e "cmake failed."
	exit -1
fi

if ! cmake --build . --parallel 4; then
	echo -e "make failed."
	exit -1
fi

popd
echo -e "--------------------------------------------------------------------------------"
