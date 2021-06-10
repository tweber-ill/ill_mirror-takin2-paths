#
# finds the openvoronoi libs
# @author Tobias Weber <tweber@ill.fr>
# @date 10-jun-2021
# @license GPLv3
#

find_path(OVD_INCLUDE_DIRS
	NAMES voronoidiagram.hpp
	PATH_SUFFIXES openvoronoi
	HINTS /usr/local/include/ /usr/include/ /opt/local/include
	DOC "OVD include directories"
)


find_library(OVD_LIBRARIES
	NAMES openvoronoi
	PATH_SUFFIXES openvoronoi
	HINTS /usr/local/lib64 /usr/local/lib /usr/lib64 /usr/lib /opt/local/lib
	DOC "OVD library"
)


if(OVD_INCLUDE_DIRS AND OVD_LIBRARIES)
	set(OVD_FOUND TRUE)

	message("OVD include directories: ${OVD_INCLUDE_DIRS}")
	message("OVD library: ${OVD_LIBRARIES}")
else()
	set(OVD_FOUND FALSE)

	message("Error: OVD could not be found!")
endif()
