#
# create an app bundle
# @author Tobias Weber <tweber@ill.fr>
# @date apr-2021
# @license GPLv3, see 'LICENSE' file
#

# remove any old version
rm -rfv taspaths.app

# create directories
mkdir -pv taspaths.app/Contents/MacOS
mkdir -pv taspaths.app/Contents/Resources

# copy files
cp -v setup/osx/Info.plist taspaths.app/Contents/
cp -v build/taspaths taspaths.app/Contents/MacOS/
cp -v res/* taspaths.app/Contents/Resources/
