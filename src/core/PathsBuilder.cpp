/**
 * calculate obstacles' voronoi edge paths
 * @author Tobias Weber <tweber@ill.fr>
 * @date jun-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "PathsBuilder.h"

#include <iostream>
#include <thread>
#include <future>
#include <cmath>
#include <cstdint>

#include "mingw_hacks.h"
#include <boost/asio.hpp>
namespace asio = boost::asio;

#include "src/libs/hull.h"
#include "tlibs2/libs/maths.h"

using t_task = std::packaged_task<void()>;
using t_taskptr = std::shared_ptr<t_task>;


PathsBuilder::PathsBuilder()
	: m_sigProgress{std::make_shared<t_sig_progress>()}
{

}


PathsBuilder::~PathsBuilder()
{

}


void PathsBuilder::CalculateConfigSpace(t_real da2, t_real da4)
{
	if(!m_instrspace)
		return;

	// angles and ranges
	t_real a6 = m_instrspace->GetInstrument().GetAnalyser().GetAxisAngleOut();

	da4 = da4 / 180. * tl2::pi<t_real>;
	t_real starta4 = 0.;
	t_real enda4 = tl2::pi<t_real>;

	da2 = da2 / 180. * tl2::pi<t_real>;
	t_real starta2 = 0.;
	t_real enda2 = tl2::pi<t_real>;

	// include scattering senses
	if(m_sensesCCW)
	{
		da4 *= m_sensesCCW[1];
		starta4 *= m_sensesCCW[1];
		enda4 *= m_sensesCCW[1];

		da2 *= m_sensesCCW[0];
		starta2 *= m_sensesCCW[0];
		enda2 *= m_sensesCCW[0];
	}

	// create colour map and image
	std::size_t img_w = (enda4-starta4) / da4;
	std::size_t img_h = (enda2-starta2) / da2;
	//std::cout << "Image size: " << img_w << " x " << img_h << "." << std::endl;
	m_img.Init(img_w, img_h);

	// create thread pool
	unsigned int num_threads = std::max<unsigned int>(
		1, std::thread::hardware_concurrency()/2);
	asio::thread_pool pool(num_threads);

	std::vector<t_taskptr> tasks;
	tasks.reserve(img_h);

	// set image pixels
	for(std::size_t img_row=0; img_row<img_h; ++img_row)
	{
		t_real a2 = std::lerp(starta2, enda2, t_real(img_row)/t_real(img_h));

		auto task = [this, img_w, img_row, starta4, enda4, a2, a6]()
		{
			InstrumentSpace instrspace_cpy = *this->m_instrspace;

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

				// set image value
				bool colliding = instrspace_cpy.CheckCollision2D();
				m_img.SetPixel(img_col, img_row, colliding ? 255 : 0);
			}
		};

		t_taskptr taskptr = std::make_shared<t_task>(task);
		tasks.push_back(taskptr);
		asio::post(pool, [taskptr]() { (*taskptr)(); });
	}

	(*m_sigProgress)(true, false, 0);

	// get results
	for(std::size_t taskidx=0; taskidx<tasks.size(); ++taskidx)
	{
		if(!(*m_sigProgress)(false, false, t_real(taskidx) / t_real(tasks.size())))
		{
			pool.stop();
			break;
		}

		tasks[taskidx]->get_future().get();
	}

	pool.join();
	(*m_sigProgress)(false, true, 1);
}


/**
 * calculate contour lines
 */
void PathsBuilder::CalculateWallContours()
{
	m_wallcontours = geo::trace_boundary<t_contourvec, decltype(m_img)>(m_img);
}


void PathsBuilder::SimplifyWallContours()
{
	for(auto& contour : m_wallcontours)
		geo::simplify_contour<t_contourvec, t_real>(contour, 2.5/180.*tl2::pi<t_real>);
}


void PathsBuilder::CalculateVoronoi()
{

}


void PathsBuilder::SimplifyVoronoi()
{

}


/**
 * save the contour data to the lines tool
 */
bool PathsBuilder::SaveToLinesTool(std::ostream& ostr)
{
	//std::ofstream ostr("contour.xml");
	ostr << "<lines2d>\n<vertices>\n";

	std::size_t vertctr = 0;
	for(const auto& contour : m_wallcontours)
	{
		for(std::size_t vert1=0; vert1<contour.size(); ++vert1)
		{
			std::size_t vert2 = (vert1 + 1) % contour.size();

			const t_contourvec& vec1 = contour[vert1];
			const t_contourvec& vec2 = contour[vert2];

			ostr << "\t<" << vertctr;
			ostr << " x=\"" << vec1[0] << "\"";
			ostr << " y=\"" << vec1[1] << "\"";
			ostr << "/>\n";
			++vertctr;

			ostr << "\t<" << vertctr;
			ostr << " x=\"" << vec2[0] << "\"";
			ostr << " y=\"" << vec2[1] << "\"";
			ostr << "/>\n\n";
			++vertctr;
		}
	}
	ostr << "\n</vertices>\n</lines2d>" << std::endl;
	return true;
}
