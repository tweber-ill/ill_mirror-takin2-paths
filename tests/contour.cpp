/**
 * boundary tracing
 * @author Tobias Weber
 * @date may-2021
 * @license see 'LICENSE' file
 * 
 * g++-10 -std=c++20 -o 0 contour.cpp -lpng
 */

#include <iostream>
#include <boost/gil/image.hpp>
#include <boost/gil/extension/io/png.hpp>
namespace gil = boost::gil;

#include "../src/libs/img.h"


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

	geo::trace_boundary(gil::view(img), gil::view(boundary));
	gil::write_view("contour.png", gil::view(boundary), gil::png_tag{});

	return 0;
}
