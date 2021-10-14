/**
 * boundary tracing
 * @author Tobias Weber
 * @date may-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * g++-10 -I.. -std=c++20 -o contour contour.cpp -lpng
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                     Grenoble, France).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#include <iostream>
#include <vector>
#include <boost/gil/image.hpp>
#include <boost/gil/extension/io/png.hpp>
namespace gil = boost::gil;

#define USE_GIL
#include "../src/libs/img.h"

using t_int = int;
using t_vec = tl2::vec<t_int, std::vector>;


int main(int argc, char** argv)
{
	if(argc <= 1)
	{
		std::cerr << "Please specify a grayscale png file." << std::endl;
		return -1;
	}

	gil::gray8_image_t img;
	gil::read_image(argv[1], img, gil::png_tag{});
	gil::gray8_image_t boundary(img.width(), img.height());

	auto boundaryview = gil::view(boundary);
	geo::trace_boundary<t_vec>(gil::view(img), &boundaryview);
	gil::write_view("contour.png", gil::view(boundary), gil::png_tag{});

	return 0;
}
