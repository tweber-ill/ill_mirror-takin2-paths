#!/bin/bash
#
# creates a debian package
# @author Tobias Weber <tweber@ill.fr>
# @date 2016, 23-may-2021
# @license GPLv3
#

create_appdir=1
create_deb=1


# defines
APPNAME="TASPaths"
APPDIRNAME="${APPNAME}"
APPDEBNAME="${APPNAME}.deb"


#
# create application directories
#
if [ $create_appdir -ne 0 ]; then
	# directories
	mkdir -p ${APPDIRNAME}/usr/local/bin
	mkdir -p ${APPDIRNAME}/usr/local/share/${APPNAME}/res
	mkdir -p ${APPDIRNAME}/usr/local/share/${APPNAME}/3rdparty_licenses
	mkdir -p ${APPDIRNAME}/usr/share/applications
	mkdir -p ${APPDIRNAME}/DEBIAN

	# debian package control file
	echo -e "Package: ${APPNAME}\nVersion: 0.3.0" > ${APPDIRNAME}/DEBIAN/control
	echo -e "Architecture: $(dpkg --print-architecture)" >> ${APPDIRNAME}/DEBIAN/control
	echo -e "Section: base\nPriority: optional" >> ${APPDIRNAME}/DEBIAN/control
	echo -e "Description: TAS path-finding tool" >> ${APPDIRNAME}/DEBIAN/control
	echo -e "Maintainer: tweber@ill.fr" >> ${APPDIRNAME}/DEBIAN/control
	echo -e "Depends:"\
		"libstdc++6 (>=10.0.0),"\
		"libboost-system1.71.0 (>=1.71.0),"\
		"libboost-filesystem1.71.0 (>=1.71.0),"\
		"libqhull-r7 (>=2015.2),"\
		"libqt5core5a (>=5.12.0),"\
		"libqt5gui5 (>=5.12.0),"\
		"libqt5widgets5 (>=5.12.0),"\
		"libqt5opengl5 (>=5.12.0),"\
		"libqt5svg5 (>=5.12.0),"\
		"libqt5printsupport5 (>=5.12.0),"\
		"libopengl0 (>=1.3.0)\n" \
			>> ${APPDIRNAME}/DEBIAN/control

	# binaries
	cp -v build/taspaths		${APPDIRNAME}/usr/local/bin
	cp -v build/lines		${APPDIRNAME}/usr/local/bin
	cp -v build/hull		${APPDIRNAME}/usr/local/bin

	# resources
	cp -rv res/*			${APPDIRNAME}/usr/local/share/${APPNAME}/res/
	cp -v AUTHORS			${APPDIRNAME}/usr/local/share/${APPNAME}/
	cp -v LICENSE			${APPDIRNAME}/usr/local/share/${APPNAME}/
	cp -rv 3rdparty_licenses/*	${APPDIRNAME}/usr/local/share/${APPNAME}/3rdparty_licenses/
	#cp -v setup_lin/taspaths.desktop	${APPDIRNAME}/usr/share/applications

	# binary permissions & stripping
	chmod a+x ${APPDIRNAME}/usr/local/bin/*
	strip -v ${APPDIRNAME}/usr/local/bin/*
fi


#
# create deb package
#
if [ $create_deb -ne 0 ]; then
	cd ${APPDIRNAME}/..
	chmod -R 775 ${APPDIRNAME}
	dpkg --build ${APPNAME}
fi
