#!/bin/bash
#
# creates a movie out of a sequence of screenshots
# @author Tobias Weber <tweber@ill.fr>
# @date oct-2021
# @license GPLv3, see 'LICENSE' file
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

convert_pdfs=1
trim_images=0
join_images=1
create_movie=1


# convert config space pdfs to pngs
if [ $convert_pdfs -ne 0 ]; then
	echo -e "\n-------------------------------------------------------------------------------"
	echo -e "Converting config space images..."
	echo -e "-------------------------------------------------------------------------------"

	for idx in {1..9999}; do
		printf -v filename_in "screenshot_%08d.pdf" ${idx}
		printf -v filename_out "screenshot_cfg_%08d.png" ${idx}

		if [ ! -f "${filename_in}" ]; then
			break
		fi

		echo -e "${filename_in} -> ${filename_out}"

		convert -density 80 ${filename_in} ${filename_out}
	done

	echo -e "-------------------------------------------------------------------------------"
fi


# trim images
if [ $trim_images -ne 0 ]; then
	echo -e "\n-------------------------------------------------------------------------------"
	echo -e "Trimming instrument space images..."
	echo -e "-------------------------------------------------------------------------------"

	for idx in {1..9999}; do
		printf -v filename_in "screenshot_%08d.png" ${idx}
		printf -v filename_out "screenshot_trimmed_%08d.png" ${idx}

		if [ ! -f "${filename_in}" ]; then
			break
		fi

		echo -e "${filename_in} -> ${filename_out}"

		convert -gravity north -chop 0x300 ${filename_in} ${filename_out}
	done

	echo -e "-------------------------------------------------------------------------------"
fi


# join images
if [ $join_images -ne 0 ]; then
	echo -e "\n-------------------------------------------------------------------------------"
	echo -e "Joining instrument and config space images..."
	echo -e "-------------------------------------------------------------------------------"

	for idx in {1..9999}; do
		if [ $trim_images -ne 0 ]; then
			printf -v filename_1 "screenshot_trimmed_%08d.png" ${idx}
		else
			printf -v filename_1 "screenshot_%08d.png" ${idx}
		fi
		printf -v filename_2 "screenshot_cfg_%08d.png" ${idx}
		printf -v filename_out "screenshot_joined_%08d.png" ${idx}

		if [ ! -f "${filename_1}" ]; then
			break
		fi

		echo -e "${filename_1} + ${filename_2} -> ${filename_out}"

		convert -background none +append ${filename_1} ${filename_2} ${filename_out}
	done

	echo -e "-------------------------------------------------------------------------------"
fi


# create movie
if [ $create_movie -ne 0 ]; then
	echo -e "\n-------------------------------------------------------------------------------"
	echo -e "Creating movie \"screenshot.mp4\"..."
	echo -e "-------------------------------------------------------------------------------"

	ffmpeg -i screenshot_joined_%08d.png -y screenshot.mp4

	echo -e "-------------------------------------------------------------------------------"
fi
