#!/bin/bash
#
# gets license files for 3rd party libraries
# @author Tobias Weber <tweber@ill.fr>
# @date jan-2021
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

LICDIR=3rdparty_licenses


# -----------------------------------------------------------------------------
# cleans old directory
# -----------------------------------------------------------------------------
function clean_dir()
{
	# remove old directory, but not if it's a link

	if [ ! -L ${LICDIR} ]; then
		rm -rfv ${LICDIR}
	fi
}


echo -e "Preparing license directory...\n"
clean_dir
mkdir ${LICDIR}
echo -e "Downloading license texts...\n"


# boost
if ! wget http://www.boost.org/LICENSE_1_0.txt -O ${LICDIR}/Boost_license.txt; then
	echo -e "Error: Cannot download Boost license.";
fi

# cgal
if ! wget https://raw.githubusercontent.com/CGAL/cgal/master/Installation/LICENSE.GPL -O ${LICDIR}/CGAL_license.txt; then
	echo -e "Error: Cannot download CGAL license.";
fi

# lapack(e)
if ! wget http://www.netlib.org/lapack/LICENSE.txt -O ${LICDIR}/Lapack_license.txt; then
	echo -e "Error: Cannot download Lapack(e) license.";
fi

# qhull
if ! wget https://raw.githubusercontent.com/qhull/qhull/master/COPYING.txt -O ${LICDIR}/QHull_license.txt; then
	echo -e "Error: Cannot download Qhull license.";
fi

# qcustomplot
if ! wget https://gitlab.com/DerManu/QCustomPlot/-/raw/master/GPL.txt -O ${LICDIR}/QCustomPlot_license.txt; then
	echo -e "Error: Cannot download QCustomPlot license.";
fi

# python
if ! wget https://raw.githubusercontent.com/python/cpython/master/Doc/license.rst -O ${LICDIR}/Python_license.txt; then
	echo -e "Error: Cannot download Python license.";
fi

# numpy
if ! wget https://raw.githubusercontent.com/numpy/numpy/master/LICENSE.txt -O ${LICDIR}/Numpy_license.txt; then
	echo -e "Error: Cannot download Numpy license.";
fi

# scipy
if ! wget https://raw.githubusercontent.com/scipy/scipy/master/LICENSE.txt -O ${LICDIR}/Scipy_license.txt; then
	echo -e "Error: Cannot download Scipy license.";
fi

# matplotlib
if ! wget https://raw.githubusercontent.com/matplotlib/matplotlib/master/LICENSE/LICENSE -O ${LICDIR}/Matplotlib_license.txt; then
	echo -e "Error: Cannot download Matplotlib license.";
fi

# dejavu
if ! wget https://raw.githubusercontent.com/dejavu-fonts/dejavu-fonts/master/LICENSE -O ${LICDIR}/DejaVu_license.txt; then
	echo -e "Error: Cannot download DejaVu license.";
fi

# libjpeg
if ! wget https://raw.githubusercontent.com/freedesktop/libjpeg/master/README -O ${LICDIR}/libjpeg_license.txt; then
	echo -e "Error: Cannot download libjpg license.";
fi

# libtiff
if ! wget https://raw.githubusercontent.com/vadz/libtiff/master/COPYRIGHT -O ${LICDIR}/libtiff_license.txt; then
	echo -e "Error: Cannot download libtiff license.";
fi

# libpng
if ! wget http://www.libpng.org/pub/png/src/libpng-LICENSE.txt -O ${LICDIR}/libpng_license.txt; then
	echo -e "Error: Cannot download libpng license.";
fi

# libgmp
if ! wget https://raw.githubusercontent.com/sethtroisi/libgmp/master/COPYINGv3 -O ${LICDIR}/libgmp_license.txt; then
	echo -e "Error: Cannot download libgmp license.";
fi
