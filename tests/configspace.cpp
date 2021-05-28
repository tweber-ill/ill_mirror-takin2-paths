/**
 * create an image of the instrument's configuration space
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @license see 'LICENSE' file
 *
 * references:
 *   - https://www.boost.org/doc/libs/1_76_0/doc/html/boost_asio/reference/thread_pool.html
 *   - https://github.com/boostorg/gil/tree/develop/example
 *
 * g++-10 -O2 -DNDEBUG -std=c++20 -I.. -o configspace configspace.cpp ../src/core/Geometry.cpp ../src/core/Axis.cpp ../src/core/Instrument.cpp ../src/core/InstrumentSpace.cpp -lboost_filesystem-mt -lboost_system-mt -lpng -lpthread
 *
 * profile:
 *   g++-10 -DDEBUG -ggdb -pg -std=c++20 -I.. -o configspace configspace.cpp ../src/core/Geometry.cpp ../src/core/Axis.cpp ../src/core/Instrument.cpp ../src/core/InstrumentSpace.cpp -lboost_filesystem -lboost_system -lpng -lpthread
 *
 *   valgrind -v --tool=callgrind ./configspace ../res/instrument.taspaths
 *   kcachegrind ...
 *
 *   ./configspace ../res/instrument.taspaths
 *   gprof configspace > configspace.prof  &&  gprof2dot configspace.prof > configspace.dot  &&  dot -Tsvg configspace.dot > configspace.svg
 */

#include <iostream>
#include <thread>
#include <future>
#include <cmath>
#include <cstdint>

#include <boost/asio.hpp>
#include <boost/gil/image.hpp>
#include <boost/gil/extension/io/png.hpp>
namespace asio = boost::asio;
namespace gil = boost::gil;

#include "src/core/Instrument.h"

using t_task = std::packaged_task<void()>;
using t_taskptr = std::shared_ptr<t_task>;


int main(int argc, char** argv)
{
	try
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

		if(auto [ok, msg] = InstrumentSpace::load(filename, instrspace); !ok)
		{
			std::cerr << "Error: " << msg << std::endl;
			return -1;
		}

		// angles and ranges
		t_real a6 = 83.957 / 180. * tl2::pi<t_real>;

		t_real da2 = 0.25 / 180. * tl2::pi<t_real>;
		t_real starta2 = 0.;
		t_real enda2 = tl2::pi<t_real>;

		t_real da4 = -0.25 / 180. * tl2::pi<t_real>;
		t_real starta4 = 0.;
		t_real enda4 = -tl2::pi<t_real>;

		// create image
		std::size_t img_w = (enda4-starta4) / da4;
		std::size_t img_h = (enda2-starta2) / da2;
		std::uint8_t *img_buf = new std::uint8_t[img_w * img_h];
		std::cout << "Image size: " << img_w << " x " << img_h << "." << std::endl;

		// create thread pool
		unsigned int num_threads = std::max<unsigned int>(
			1, std::thread::hardware_concurrency()/2);
		asio::thread_pool pool(num_threads);
		std::cout << "Using " << num_threads << " threads." << std::endl;

		std::vector<t_taskptr> tasks;
		tasks.reserve(img_h);

		// set image pixels
		for(std::size_t img_row=0; img_row<img_h; ++img_row)
		{
			t_real a2 = std::lerp(starta2, enda2, t_real(img_row)/t_real(img_h));
			std::uint8_t* img_buf_row = img_buf + img_row*img_w;

			auto task = [&instrspace, img_buf_row, img_w, img_row, starta4, enda4, a2, a6]()
			{
				InstrumentSpace instrspace_cpy = instrspace;

				for(std::size_t img_col=0; img_col<img_w; ++img_col)
				{
					t_real a4 = std::lerp(starta4, enda4, t_real(img_col)/t_real(img_w));
					t_real a3 = a4 * 0.5;

					// set scattering angles
					instrspace_cpy.GetInstrument().GetMonochromator().SetAxisAngleOut(a2);
					instrspace_cpy.GetInstrument().GetSample().SetAxisAngleOut(a4);
					instrspace_cpy.GetInstrument().GetAnalyser().SetAxisAngleOut(a6);

					// set crystal angles
					instrspace_cpy.GetInstrument().GetMonochromator().SetAxisAngleInternal(0.5 * a2);
					instrspace_cpy.GetInstrument().GetSample().SetAxisAngleInternal(a3);
					instrspace_cpy.GetInstrument().GetAnalyser().SetAxisAngleInternal(0.5 * a6);

					if(instrspace_cpy.CheckCollision2D())
						img_buf_row[img_col] = 0x00;
					else
						img_buf_row[img_col] = 0xff;
				}
			};

			t_taskptr taskptr = std::make_shared<t_task>(task);
			tasks.push_back(taskptr);
			asio::post(pool, [taskptr]() { (*taskptr)(); });
		}

		for(std::size_t taskidx=0; taskidx<tasks.size(); ++taskidx)
		{
			tasks[taskidx]->get_future().get();
			std::cout << "Task " << taskidx+1 << " of " 
				<< tasks.size() << " finished." << std::endl;
		}

		pool.join();

		// save image
		gil::gray8_image_t img(img_w, img_h);
		auto img_view = gil::view(img);

		for(std::size_t img_row=0; img_row<img_h; ++img_row)
		{
			auto img_iter = img_view.row_begin(img_row);
			for(std::size_t img_col=0; img_col<img_w; ++img_col)
				*(img_iter + img_col) = img_buf[img_row*img_w + img_col];
		}

		delete[] img_buf;
		gil::write_view("configspace.png", img_view, gil::png_tag{});
	}
	catch(const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
		return -1;
	}

	return 0;
}
