#!/bin/bash
#
# create an app bundle and dmg file
# @author Tobias Weber <tweber@ill.fr>
# @date jan-2019, apr-2021
# @license GPLv3, see 'LICENSE' file
#

create_appdir=1
create_dmg=1

APPNAME="TASPaths"
APPDIRNAME="${APPNAME}.app"
APPDMGNAME="${APPNAME}.dmg"
TMPFILE="${APPNAME}_tmp.dmg"


if [ $create_appdir -ne 0 ]; then
	echo -e "\nCleaning and (re)creating directories..."
	rm -rfv "${APPDIRNAME}"
	mkdir -pv "${APPDIRNAME}/Contents/MacOS"
	mkdir -pv "${APPDIRNAME}/Contents/Resources"
	mkdir -pv "${APPDIRNAME}/Contents/Libraries"
	echo -e "--------------------------------------------------------------------------------"

	echo -e "\nCopying files to ${APPDIRNAME}..."
	cp -v setup/osx/Info.plist "${APPDIRNAME}/Contents/"
	cp -v build/taspaths "${APPDIRNAME}/Contents/MacOS/"
	cp -v res/* "${APPDIRNAME}/Contents/Resources/"
	cp -v /usr/local/lib/libboost_filesystem.dylib "${APPDIRNAME}/Contents/Libraries/"
	echo -e "--------------------------------------------------------------------------------"

	echo -e "\nChanging linked names..."
	install_name_tool -change \
		@rpath/libboost_filesystem.dylib \
		@executable_path/../Libraries/libboost_filesystem.dylib \
		"${APPDIRNAME}/Contents/MacOS/taspaths"
	echo -e "--------------------------------------------------------------------------------"
fi


if [ $create_dmg -ne 0 ]; then
	echo -e "\nCreating ${APPDMGNAME} from ${APPDIRNAME}..."
	rm -fv "${APPDMGNAME}"
	rm -fv "${TMPFILE}"
	if ! hdiutil create "${APPDMGNAME}" -srcfolder "${APPDIRNAME}" \
		-fs UDF -format "UDRW" -volname "${APPNAME}"
	then
		echo -e "Error: Cannot create ${APPDMGNAME}."
		exit -1
	fi
	echo -e "--------------------------------------------------------------------------------"

	echo -e "\nMounting ${APPDMGNAME}..."
	if ! hdiutil attach "${APPDMGNAME}" -readwrite; then
		echo -e "Error: Cannot mount ${APPDMGNAME}."
		exit -1
	fi

	echo -e "\nAdding files to ${APPDMGNAME}..."
	ln -sf /Applications \
		"/Volumes/${APPNAME}/Install by dragging ${APPDIRNAME} here."

	echo -e "\nUnmounting ${APPDMGNAME}..."
	if ! hdiutil detach "/Volumes/${APPNAME}"; then
		echo -e "Error: Cannot detach ${APPDMGNAME}."
		exit -1
	fi
	echo -e "--------------------------------------------------------------------------------"

	echo -e "\nCompressing ${APPDMGNAME} into ${TMPFILE}..."
	if ! hdiutil convert "${APPDMGNAME}" -o "${TMPFILE}" -format "UDBZ"
	then
		echo -e "Error: Cannot compress ${APPDMGNAME}."
		exit -1
	fi
	echo -e "--------------------------------------------------------------------------------"

	echo -e "\nCopying ${APPDMGNAME}..."
	mv -v "${TMPFILE}" "${APPDMGNAME}"

	echo -e "\nSuccessfully created "${APPDMGNAME}"."
	echo -e "--------------------------------------------------------------------------------"
fi
