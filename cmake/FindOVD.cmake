#
# finds the openvoronoi libs
# @author Tobias Weber <tweber@ill.fr>
# @date 10-jun-2021
# @license GPLv3
#
# -----------------------------------------------------------------------------
# TAS-Paths (part of the Takin software suite)
# Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL), 
#                     Grenoble, France).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# -----------------------------------------------------------------------------
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
