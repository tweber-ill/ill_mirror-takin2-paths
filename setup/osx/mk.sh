#!/bin/bash
#
# creates an app bundle and a dmg file
# @author Tobias Weber <tweber@ill.fr>
# @date jan-2019, apr-2021
# @license GPLv3, see 'LICENSE' file
#

create_appdir=1
create_dmg=1
strip_binaries=1
clean_frameworks=1

APPNAME="TASPaths"
APPDIRNAME="${APPNAME}.app"
APPDMGNAME="${APPNAME}.dmg"
TMPFILE="${APPNAME}_tmp.dmg"


declare -a QT_LIBS=(QtCore QtGui QtWidgets QtOpenGL QtDBus QtPrintSupport QtSvg)


#
# test if the given file is a binary image
#
is_binary() {
	local binary="$1"

	# does the file exist?
	if [ ! -f $binary ]; then
		return 0
	fi

	# is it a binary?
	if [[ "$(file ${binary})" != *"Mach-O"* ]]; then
		return 0
	fi

	return 1
}


#
# change a /usr/local linker path to an @rpath
#
change_to_rpath() {
	local binary="$1"
	local old_paths=$(otool -L ${binary} | grep /usr/local | sed -e "s/(.*)//p" -n | sed -e "s/\t//p" -n)

	is_binary ${binary}
	if [[ $? == 0 ]]; then
		return
	fi

	for old_path in $old_paths; do
		case "${old_path}" in
			*".framework"*)
				local new_base=$(basename ${old_path})
				local cur_path=$(dirname ${old_path})
				local new_path=${new_base}

				while true; do
					local next_base=$(basename ${cur_path})
					local cur_path=$(dirname ${cur_path})
					local new_path="${next_base}/${new_path}"

					if [[ "${next_base}" == *".framework"* ]]; then
						break
					fi
				done

				local new_path=@rpath/${new_path}
				;;

			*)
				local new_path=@rpath/$(basename $old_path)
				;;
		esac

		echo -e "Changing linker path: $old_path -> $new_path"
		install_name_tool -change ${old_path} ${new_path} ${binary}
	done
}


#
# create the application directory
#
if [ $create_appdir -ne 0 ]; then
	echo -e "\nCleaning and (re)creating directories..."
	rm -rfv "${APPDIRNAME}"

	mkdir -pv "${APPDIRNAME}/Contents/MacOS"
	mkdir -pv "${APPDIRNAME}/Contents/Resources"
	mkdir -pv "${APPDIRNAME}/Contents/Libraries"
	mkdir -pv "${APPDIRNAME}/Contents/Libraries/Qt_Plugins"
	mkdir -pv "${APPDIRNAME}/Contents/Frameworks"

	ln -sf "Libraries/Qt_Plugins" "${APPDIRNAME}/Contents/PlugIns"
	echo -e "--------------------------------------------------------------------------------"


	echo -e "\nCopying files to ${APPDIRNAME}..."

	# program files
	cp -v setup/osx/Info.plist "${APPDIRNAME}/Contents/"
	cp -v build/taspaths "${APPDIRNAME}/Contents/MacOS/"
	cp -v build/lines "${APPDIRNAME}/Contents/MacOS/"
	cp -v build/hull "${APPDIRNAME}/Contents/MacOS/"
	cp -v build/poly "${APPDIRNAME}/Contents/MacOS/"
	cp -v res/* "${APPDIRNAME}/Contents/Resources/"

	# libraries
	cp -v /usr/local/lib/libboost_filesystem-mt.dylib "${APPDIRNAME}/Contents/Libraries/"
	cp -v /usr/local/lib/libqhull_r.8.0.dylib "${APPDIRNAME}/Contents/Libraries/"
	#cp -v /usr/local/opt/lapack/lib/liblapacke.3.dylib "${APPDIRNAME}/Contents/Libraries/"
	#cp -v /usr/local/opt/lapack/lib/liblapack.3.dylib "${APPDIRNAME}/Contents/Libraries/"
	cp -v build/libqcustomplot_local.dylib "${APPDIRNAME}/Contents/Libraries/"

	# frameworks
	for (( libidx=0; libidx<${#QT_LIBS[@]}; ++libidx )); do
		QT_LIB=${QT_LIBS[$libidx]}

		cp -rv /usr/local/Frameworks/${QT_LIB}.framework/ \
			"${APPDIRNAME}/Contents/Frameworks/"
	done

	# remove unnecessary files from frameworks
	if [ $clean_frameworks -ne 0 ]; then
		echo -e "\nCleaning frameworks..."
		find ${APPDIRNAME}/Contents/Frameworks/ -type d -name "Headers" -exec rm -rfv {} \;
		find ${APPDIRNAME}/Contents/Frameworks/ -type l -name "Headers" -exec rm -rv {} \;
		echo -e "--------------------------------------------------------------------------------"
	fi

	# qt plugins
	cp -rv /usr/local/opt/qt@5/plugins/platforms "${APPDIRNAME}/Contents/Libraries/Qt_Plugins/"
	cp -rv /usr/local/opt/qt@5/plugins/styles "${APPDIRNAME}/Contents/Libraries/Qt_Plugins/"
	#cp -rv /usr/local/opt/qt@5/plugins/renderers "${APPDIRNAME}/Contents/Libraries/Qt_Plugins/"
	#cp -rv /usr/local/opt/qt@5/plugins/imageformats "${APPDIRNAME}/Contents/Libraries/Qt_Plugins/"
	#cp -rv /usr/local/opt/qt@5/plugins/iconengines "${APPDIRNAME}/Contents/Libraries/Qt_Plugins/"
	#cp -rv /usr/local/opt/qt@5/plugins/printsupport "${APPDIRNAME}/Contents/Libraries/Qt_Plugins/"
	#cp -rv /usr/local/opt/qt@5/plugins/platformthemes "${APPDIRNAME}/Contents/Libraries/Qt_Plugins/"

	rm -fv ${APPDIRNAME}/Contents/Libraries/Qt_Plugins/platforms/libqoffscreen.dylib
	rm -fv ${APPDIRNAME}/Contents/Libraries/Qt_Plugins/platforms/libqwebgl.dylib
	echo -e "--------------------------------------------------------------------------------"


	echo -e "\nChanging linked names..."

	# binaries
	for binary in $(ls "${APPDIRNAME}/Contents/MacOS/"); do
		echo -e "\nProcessing ${binary}..."

		install_name_tool \
			-add_rpath @executable_path/../Libraries \
			-add_rpath @executable_path/../Frameworks \
			"${APPDIRNAME}/Contents/MacOS/${binary}"

		change_to_rpath "${APPDIRNAME}/Contents/MacOS/${binary}"

		if [ $strip_binaries -ne 0 ]; then
			llvm-strip "${APPDIRNAME}/Contents/MacOS/${binary}"
		fi
	done

	# libraries and frameworks
	for library in $(find "${APPDIRNAME}/Contents/Libraries/" -type f && \
		find "${APPDIRNAME}/Contents/Frameworks/" -type f)
	do
		is_binary ${library}
		if [[ $? == 0 ]]; then
			continue
		fi

		echo -e "\nProcessing ${library}..."

		#install_name_tool \
		#	-add_rpath @executable_path/../Libraries \
		#	-add_rpath @executable_path/../Frameworks \
		#	"${library}"

		change_to_rpath "${library}"

		#for (( libidx=0; libidx<${#QT_LIBS[@]}; ++libidx )); do
		#	otherlibrary=${QT_LIBS[$libidx]}
		#
		#	install_name_tool -change \
		#		@rpath/${otherlibrary}.framework/Versions/A/${otherlibrary} \
		#		@executable_path/../Libraries/${otherlibrary} \
		#		"${library}"
		#done

		if [ $strip_binaries -ne 0 ]; then
			llvm-strip "${library}"
		fi
	done

	#install_name_tool -id "/usr/local/lib/libqcustomplot_local.dylib" "${APPDIRNAME}/Contents/Libraries/libqcustomplot_local.dylib"
	echo -e "--------------------------------------------------------------------------------"
fi


#
# create a dmg image
#
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
