#!/bin/bash
#
# downloads external libraries
# @author Tobias Weber <tweber@ill.fr>
# @date 22-apr-2021
# @license GPLv3, see 'LICENSE' file
#


# -----------------------------------------------------------------------------
# tools
# -----------------------------------------------------------------------------
WGET=wget
UNZIP=unzip
TAR=tar
UZIP=unzip
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# helper functions
# -----------------------------------------------------------------------------
get_filename_from_url() {
	local url="$1"

	local filename=${url##*[/\\]}
	echo "${filename}"
}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# URLs for external libraries
# -----------------------------------------------------------------------------
TLIBS2=https://code.ill.fr/scientific-software/takin/tlibs2/-/archive/master/tlibs2-master.zip
PLOTTER=https://www.qcustomplot.com/release/2.1.0fixed/QCustomPlot-source.tar.gz
BOOST_POLY_UTIL=https://raw.githubusercontent.com/boostorg/polygon/develop/example/voronoi_visual_utils.hpp
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# cleans externals
# -----------------------------------------------------------------------------
function clean_dirs()
{
	# remove old versions, but not if they're links

	if [ ! -L tlibs2 ]; then
		rm -rfv tlibs2
	fi

	if [ ! -L externals ]; then
		rm -rfv externals
	fi
}

function clean_files()
{
	local TLIBS2_LOCAL=$(get_filename_from_url ${TLIBS2})
	rm -fv ${TLIBS2_LOCAL}

	local PLOTTER_LOCAL=$(get_filename_from_url ${PLOTTER})
	rm -fv ${PLOTTER_LOCAL}
}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
function setup_tlibs2()
{
	local TLIBS2_LOCAL=$(get_filename_from_url ${TLIBS2})

	if [ -L tlibs2 ]; then
		echo -e "A link to tlibs2 already exists, skipping action."
		return
	fi

	if ! ${WGET} ${TLIBS2}
	then
		echo -e "Error downloading tlibs2.";
		exit -1;
	fi

	if ! ${UNZIP} ${TLIBS2_LOCAL}
	then
		echo -e "Error extracting tlibs2.";
		exit -1;
	fi

	mv -v tlibs2-master tlibs2
}


function setup_plotter()
{
	local PLOTTER_LOCAL=$(get_filename_from_url ${PLOTTER})

	if [ -L qcustomplot ]; then
		echo -e "A link to QCustomPlot already exists, skipping action."
		return
	fi

	if ! ${WGET} ${PLOTTER}
	then
		echo -e "Error downloading QCustomPlot.";
		exit -1;
	fi

	if ! ${TAR} xzvf ${PLOTTER_LOCAL}
	then
		echo -e "Error extracting QCustomPlot.";
		exit -1;
	fi

	mv -v qcustomplot-source externals/qcustomplot
}


function setup_boostpoly()
{
	local BOOST_POLY_UTIL_LOCAL=$(get_filename_from_url ${BOOST_POLY_UTIL})

	if ! ${WGET} ${BOOST_POLY_UTIL}
	then
		echo -e "Error downloading Boost.Polygon utils.";
		exit -1;
	fi

	mv -v ${BOOST_POLY_UTIL_LOCAL} externals/
}
# -----------------------------------------------------------------------------


echo -e "--------------------------------------------------------------------------------"
echo -e "Removing old files and directories...\n"
clean_dirs
clean_files
mkdir externals
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Downloading and setting up tlibs2...\n"
setup_tlibs2
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Downloading and setting up QCustomPlot...\n"
setup_plotter
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Downloading and setting up Boost.Polygon utilities...\n"
setup_boostpoly
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Removing temporary files...\n"
clean_files
echo -e "--------------------------------------------------------------------------------\n"
