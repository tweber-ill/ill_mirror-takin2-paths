#!/bin/bash
#
# downloads external libraries
# @author Tobias Weber <tweber@ill.fr>
# @date 22-apr-2021
# @license GPLv3, see 'LICENSE' file
#


# -----------------------------------------------------------------------------
# tools
WGET=wget
UNZIP=unzip
TAR=tar
UZIP=unzip
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# URLs for external libraries
TLIBS2=https://code.ill.fr/scientific-software/takin/tlibs2/-/archive/master/tlibs2-master.zip
TLIBS2_LOCAL=${TLIBS2##*[/\\]}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# cleans externals
function clean_dirs()
{
	# remove old version, but not if it's a link
	if [ ! -L tlibs2 ]; then
		rm -rfv tlibs2
	fi
}

function clean_files()
{
	rm -fv ${TLIBS2_LOCAL}
}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
function setup_tlibs2()
{
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

	mv tlibs2-master tlibs2
}
# -----------------------------------------------------------------------------


echo -e "--------------------------------------------------------------------------------"
echo -e "Removing old files and directories...\n"
clean_dirs
clean_files
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Downloading and setting up tlibs2...\n"
setup_tlibs2
echo -e "--------------------------------------------------------------------------------\n"

echo -e "--------------------------------------------------------------------------------"
echo -e "Removing temporary files...\n"
clean_files
echo -e "--------------------------------------------------------------------------------\n"
