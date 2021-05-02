/**
 * create an image of the instrument's configuration space
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @license see 'LICENSE' file
 *
 * g++-10 -O2 -DNDEBUG -std=c++20 -I.. -o configspace configspace.cpp ../src/core/Geometry.cpp ../src/core/Instrument.cpp -lpng -lboost_filesystem
 */

#include <iostream>
#include <cmath>
#include "src/core/Instrument.h"

#include <boost/gil/image.hpp>
#include <boost/gil/extension/io/png.hpp>
namespace gil = boost::gil;


int main(int argc, char** argv)
{
	if(argc < 2)
	{
		std::cerr << "Please give an instrument file name." << std::endl;
		return -1;
	}

	std::cout.precision(4);

	// load instrument definition
	std::string filename = argv[1];
	InstrumentSpace instrspace;

	if(auto [ok, msg] = load_instrumentspace(filename, instrspace); !ok)
	{
		std::cerr << "Error: " << msg << std::endl;
		return -1;
	}

	t_real a6 = 83.957 / 180. * tl2::pi<t_real>;

	t_real da2 = 0.5 / 180. * tl2::pi<t_real>;
	t_real starta2 = 0.;
	t_real enda2 = tl2::pi<t_real>;

	t_real da4 = -0.5 / 180. * tl2::pi<t_real>;
	t_real starta4 = 0.;
	t_real enda4 = -tl2::pi<t_real>;

	std::size_t img_w = (enda4-starta4) / da4;
	std::size_t img_h = (enda2-starta2) / da2;
	std::cout << "Image size: " << img_w << " x " << img_h << "." << std::endl;
	gil::gray8_image_t img(img_w, img_h);
	auto img_view = gil::view(img);

	for(std::size_t img_row=0; img_row<img_h; ++img_row)
	{
		std::cout << 100. * t_real(img_row+1) / t_real(img_h) << " %" << std::endl;

		t_real a2 = std::lerp(starta2, enda2, t_real(img_row)/t_real(img_h));
		auto img_iter = img_view.row_begin(img_row);

		for(std::size_t img_col=0; img_col<img_w; ++img_col)
		{
			t_real a4 = std::lerp(starta4, enda4, t_real(img_col)/t_real(img_w));
			t_real a3 = a4 * 0.5;

			// set scattering angles
			instrspace.GetInstrument().GetMonochromator().SetAxisAngleOut(a2);
			instrspace.GetInstrument().GetSample().SetAxisAngleOut(a4);
			instrspace.GetInstrument().GetAnalyser().SetAxisAngleOut(a6);

			// set crystal angles
			instrspace.GetInstrument().GetMonochromator().SetAxisAngleInternal(0.5 * a2);
			instrspace.GetInstrument().GetSample().SetAxisAngleInternal(a3);
			instrspace.GetInstrument().GetAnalyser().SetAxisAngleInternal(0.5 * a6);

			if(instrspace.CheckCollision2D())
				(*img_iter)[img_col] = 0x00;
			else
				(*img_iter)[img_col] = 0xff;
		}
	}

	gil::write_view("configspace.png", img_view, gil::png_tag{});
	return 0;
}
