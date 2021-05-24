#!/bin/bash
#
# creates a mingw distribution
# @author Tobias Weber <tweber@ill.fr>
# @date may-2021
# @license GPLv2
#

# defines
APPNAME="TASPaths"
APPDIRNAME="${APPNAME}"

# create directories
mkdir -p ${APPDIRNAME}
mkdir -p ${APPDIRNAME}/res
mkdir -p ${APPDIRNAME}/qt_plugins/platforms/
mkdir -p ${APPDIRNAME}/qt_plugins/styles/
mkdir -p ${APPDIRNAME}/qt_plugins/iconengines/
mkdir -p ${APPDIRNAME}/qt_plugins/imageformats

# copy program files
cp -v bin/*.exe		${APPDIRNAME}/
cp -rv res/* 		${APPDIRNAME}/res/
cp -v AUTHORS		${APPDIRNAME}/
cp -v LICENSE		${APPDIRNAME}/
cp -rv 3rdparty_licenses/	${APPDIRNAME}/

# create qt config file
echo -e "[Paths]\nPlugins = qt_plugins\n" > ${APPDIRNAME}/qt.conf

# copy third-party libraries
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/Qt5Core.dll			${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/Qt5Gui.dll				${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/Qt5Widgets.dll			${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/Qt5OpenGL.dll			${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/Qt5DBus.dll			${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/Qt5PrintSupport.dll	${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/Qt5Svg.dll				${APPDIRNAME}/

cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libboost_system-x64.dll		${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libboost_filesystem-x64.dll	${APPDIRNAME}/

cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libstdc++-6.dll		${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll	${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libglib-2.0-0.dll		${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_sjlj-1.dll	${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_seh-1.dll		${APPDIRNAME}/

cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libbz2-1.dll			${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/zlib1.dll				${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libpng16-16.dll		${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/iconv.dll				${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libpcre2-16-0.dll		${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libharfbuzz-0.dll		${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libintl-8.dll			${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libpcre-1.dll			${APPDIRNAME}/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libssp-0.dll			${APPDIRNAME}/

#cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/liblapack.dll		${APPDIRNAME}/
#cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/liblapacke.dll	${APPDIRNAME}/
#cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libblas.dll		${APPDIRNAME}/
#cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libquadmath-0.dll	${APPDIRNAME}/
#cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgfortran-5.dll	${APPDIRNAME}/

# copy qt plugins
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/lib/qt5/plugins/platforms/*.dll			${APPDIRNAME}/qt_plugins/platforms/
cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/lib/qt5/plugins/styles/*.dll				${APPDIRNAME}/qt_plugins/styles/
#cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/lib/qt5/plugins/iconengines/qsvgicon.dll	${APPDIRNAME}/qt_plugins/iconengines/
#cp -v /usr/x86_64-w64-mingw32/sys-root/mingw/lib/qt5/plugins/imageformats/qsvg.dll		${APPDIRNAME}/qt_plugins/imageformats/

# stripping
find ${APPDIRNAME} -type f \( -name "*.exe" -o -name "*.dll" \) -exec strip -v {} \;
