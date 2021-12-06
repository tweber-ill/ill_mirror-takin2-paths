#
# finds the qcustomplot libs
# @author Tobias Weber <tweber@ill.fr>
# @date 16-sep-2020
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

find_path(QCP_INCLUDE_DIRS
	NAMES qcustomplot.h
	HINTS ./externals/qcustomplot /usr/include/ /usr/local/include/
		/opt/local/include
	DOC "QCP include directories"
)


find_library(QCP_LIBRARIES
	NAMES qcustomplot_local qcustomplot
	HINTS ./externals/qcustomplot /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/lib
		 /usr/local/lib64 /usr/local/lib /opt/local/lib
	DOC "QCP library"
)


if(QCP_INCLUDE_DIRS AND QCP_LIBRARIES)
	set(QCP_FOUND TRUE)

	message("QCP include directories: ${QCP_INCLUDE_DIRS}")
	message("QCP libraries: ${QCP_LIBRARIES}")
else()
	set(QCP_FOUND FALSE)

	message("Error: QCP could not be found!")
endif()
