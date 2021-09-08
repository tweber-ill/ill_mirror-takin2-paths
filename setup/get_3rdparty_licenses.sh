#!/bin/bash
#
# gets license files for 3rd party libraries
# @author Tobias Weber <tweber@ill.fr>
# @date jan-2021
# @license GPLv3, see 'LICENSE' file
#

LICDIR=3rdparty_licenses


# -----------------------------------------------------------------------------
# cleans old directory
# -----------------------------------------------------------------------------
function clean_dir()
{
	# remove old versions, but not if they're links

	if [ ! -L ${LICDIR} ]; then
		rm -rfv ${LICDIR}
	fi
}


echo -e "Preparing license directory...\n"
clean_dir
mkdir ${LICDIR}
echo -e "Downloading license texts...\n"


# boost
if ! wget http://www.boost.org/LICENSE_1_0.txt -O ${LICDIR}/boost_license.txt; then
	echo -e "Error: Cannot download Boost license.";
fi

# lapack(e)
if ! wget http://www.netlib.org/lapack/LICENSE.txt -O ${LICDIR}/lapack_license.txt; then
	echo -e "Error: Cannot download Lapack(e) license.";
fi

# qhull
if ! wget https://raw.githubusercontent.com/qhull/qhull/master/COPYING.txt -O ${LICDIR}/qhull_license.txt; then
	echo -e "Error: Cannot download Qhull license.";
fi

# qcustomplot
if ! wget https://gitlab.com/DerManu/QCustomPlot/-/raw/master/GPL.txt -O ${LICDIR}/qcustomplot_license.txt; then
	echo -e "Error: Cannot download QCustomPlot license.";
fi

# python
if ! wget https://raw.githubusercontent.com/python/cpython/master/Doc/license.rst -O ${LICDIR}/python_license.txt; then
	echo -e "Error: Cannot download Python license.";
fi

# numpy
if ! wget https://raw.githubusercontent.com/numpy/numpy/master/LICENSE.txt -O ${LICDIR}/numpy_license.txt; then
	echo -e "Error: Cannot download Numpy license.";
fi

# scipy
if ! wget https://raw.githubusercontent.com/scipy/scipy/master/LICENSE.txt -O ${LICDIR}/scipy_license.txt; then
	echo -e "Error: Cannot download Scipy license.";
fi

# matplotlib
if ! wget https://raw.githubusercontent.com/matplotlib/matplotlib/master/LICENSE/LICENSE -O ${LICDIR}/matplotlib_license.txt; then
	echo -e "Error: Cannot download Matplotlib license.";
fi

# dejavu
if ! wget https://raw.githubusercontent.com/dejavu-fonts/dejavu-fonts/master/LICENSE -O ${LICDIR}/dejavu_license.txt; then
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
