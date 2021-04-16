#
# @author Tobias Weber <tweber@ill.fr>
# @date apr-2021
# @license GPLv3, see 'LICENSE' file
#

rm -rfv build
mkdir build
cd build

if ! cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2" -DCMAKE_VERBOSE_MAKEFILE=False ..; then
	echo -e "cmake failed."
	exit -1
fi

if ! make -j4; then
	echo -e "make failed."
	exit -1
fi

strip -v taspaths
