#!/bin/bash
#
# creates a mingw distribution
# @author Tobias Weber <tweber@ill.fr>
# @date may-2021
# @license GPLv3, see 'LICENSE' file
#

use_lapacke=0

APPNAME="TASPaths"
APPDIRNAME="${APPNAME}"


# third-party libraries
EXT_LIBS=( \
	Qt5Core.dll Qt5Gui.dll Qt5Widgets.dll Qt5OpenGL.dll Qt5DBus.dll Qt5PrintSupport.dll Qt5Svg.dll \
	libboost_system-x64.dll libboost_filesystem-x64.dll \
	libstdc++-6.dll libwinpthread-1.dll libglib-2.0-0.dll libgcc_s_sjlj-1.dll libgcc_s_seh-1.dll \
	libbz2-1.dll zlib1.dll \
	libpng16-16.dll \
	libfreetype-6.dll \
	libpcre2-16-0.dll libpcre-1.dll libssp-0.dll \
	libharfbuzz-0.dll \
	iconv.dll libintl-8.dll \
)

# lapack(e) libraries
if [ $create_appdir -ne 0 ]; then
	EXT_LIBS+=( \
		liblapack.dll liblapacke.dll libblas.dll \
		libgfortran-5.dll libquadmath-0.dll \
	)
fi

# qt plugins
QT_PLUGINS=( \
	platforms/*.dll \
	styles/*.dll \
)

# create directories
mkdir -p ${APPDIRNAME}
mkdir -p ${APPDIRNAME}/res
mkdir -p ${APPDIRNAME}/qt_plugins/platforms/
mkdir -p ${APPDIRNAME}/qt_plugins/styles/

# copy program files
cp -v build/*.exe          ${APPDIRNAME}/
cp -rv res/*               ${APPDIRNAME}/res/
cp -v AUTHORS              ${APPDIRNAME}/
cp -v LICENSE              ${APPDIRNAME}/
cp -rv 3rdparty_licenses/  ${APPDIRNAME}/


# copy third-party libraries
for THELIB in ${EXT_LIBS[@]}; do
	cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/${THELIB} ${APPDIRNAME}/
done

# copy qt plugins
for THELIB in ${QT_PLUGINS[@]}; do
	LIBDIRNAME=$(dirname ${THELIB})
	cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/lib/qt5/plugins/${THELIB} ${APPDIRNAME}/qt_plugins/${LIBDIRNAME}
done

# create qt config file
echo -e "[Paths]\n\tPlugins = qt_plugins\n" > ${APPDIRNAME}/qt.conf

# stripping
find ${APPDIRNAME} -type f \( -name "*.exe" -o -name "*.dll" \) -exec strip -v {} \;
