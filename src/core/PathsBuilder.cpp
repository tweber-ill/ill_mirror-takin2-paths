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


/**
 * returns the full or the simplified wall contours
 */
const std::vector<std::vector<PathsBuilder::t_contourvec>>& 
PathsBuilder::GetWallContours(bool full) const
{
	if(full)
		return m_fullwallcontours;

	return m_wallcontours;
}


/**
 * calculate the obstacle regions in the angular configuration space
 */
void PathsBuilder::CalculateConfigSpace(t_real da2, t_real da4)
{
	if(!m_instrspace)
		return;

	std::string message{"Calculating configuration space..."};
	(*m_sigProgress)(true, false, 0, message);

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

	// get results
	for(std::size_t taskidx=0; taskidx<tasks.size(); ++taskidx)
	{
		if(!(*m_sigProgress)(false, false, t_real(taskidx) / t_real(tasks.size()), message))
		{
			pool.stop();
			break;
		}

		tasks[taskidx]->get_future().get();
	}

	pool.join();
	(*m_sigProgress)(false, true, 1, message);
}


/**
 * calculate the contour lines of the obstacle regions
 */
void PathsBuilder::CalculateWallContours()
{
	std::string message{"Calculating obstacle contours..."};
	(*m_sigProgress)(true, false, 0, message);

	m_wallcontours = geo::trace_boundary<t_contourvec, decltype(m_img)>(m_img);
	m_fullwallcontours = m_wallcontours;

	(*m_sigProgress)(false, false, 0.5, message);


	// simplify wall contours
	for(auto& contour : m_wallcontours)
		geo::simplify_contour<t_contourvec, t_real>(contour, 1./180.*tl2::pi<t_real>);

	(*m_sigProgress)(false, true, 1, message);
}


/**
 * calculate lines segments and groups
 */
void PathsBuilder::CalculateLineSegments()
{
	std::string message{"Calculating obstacle line segments..."};
	(*m_sigProgress)(true, false, 0, message);

	m_lines.clear();
	m_linegroups.clear();

	std::size_t totalverts = 0;
	for(const auto& contour : m_wallcontours)
		totalverts += contour.size();

	m_lines.reserve(totalverts/2 + 1);
	m_linegroups.reserve(m_wallcontours.size());

	// contour vertices
	std::size_t linectr = 0;
	for(std::size_t contouridx = 0; contouridx < m_wallcontours.size(); ++contouridx)
	{
		const auto& contour = m_wallcontours[contouridx];
		std::size_t groupstart = linectr;

		for(std::size_t vert1 = 0; vert1 < contour.size(); ++vert1)
		{
			std::size_t vert2 = (vert1 + 1) % contour.size();

			const t_contourvec& vec1 = contour[vert1];
			const t_contourvec& vec2 = contour[vert2];

			t_vec linevec1 = vec1;
			t_vec linevec2 = vec2;
			m_lines.emplace_back(std::make_pair(std::move(linevec1), std::move(linevec2)));

			++linectr;
		}

		// mark line group start and end index
		std::size_t groupend = linectr;
		m_linegroups.emplace_back(std::make_pair(groupstart, groupend));
	}

	(*m_sigProgress)(false, true, 1, message);
}


/**
 * calculate the voronoi diagram
 */
void PathsBuilder::CalculateVoronoi()
{
	std::string message{"Calculating voronoi diagram..."};
	(*m_sigProgress)(true, false, 0, message);

	std::tie(m_vertices, m_linear_edges, m_parabolic_edges, m_vorograph)
		= geo::calc_voro<t_vec, t_line, t_graph>(m_lines, m_linegroups, true, m_voroedge_eps);

	(*m_sigProgress)(false, true, 1, message);
}


/**
 * save the contour line segments to the lines tool
 */
bool PathsBuilder::SaveToLinesTool(std::ostream& ostr)
{
	//std::ofstream ostr("contour.xml");
	ostr << "<lines2d>\n";

	// contour vertices
	std::size_t vertctr = 0;
	ostr << "<vertices>\n";
	for(std::size_t contouridx = 0; contouridx < m_linegroups.size(); ++contouridx)
	{
		const auto& contour = m_linegroups[contouridx];
		ostr << "\t<!-- contour " << contouridx << " -->\n";

		for(std::size_t lineidx=std::get<0>(contour); lineidx<std::get<1>(contour); ++lineidx)
		{
			const t_line& line = m_lines[lineidx];

			ostr << "\t<" << vertctr;
			ostr << " x=\"" << std::get<0>(line)[0] << "\"";
			ostr << " y=\"" << std::get<0>(line)[1] << "\"";
			ostr << "/>\n";
			++vertctr;

			ostr << "\t<" << vertctr;
			ostr << " x=\"" << std::get<1>(line)[0] << "\"";
			ostr << " y=\"" << std::get<1>(line)[1] << "\"";
			ostr << "/>\n\n";
			++vertctr;
		}
	}
	ostr << "</vertices>\n";

	// contour groups
	ostr << "\n<groups>\n";
	for(std::size_t groupidx = 0; groupidx < m_linegroups.size(); ++groupidx)
	{
		const auto& group = m_linegroups[groupidx];
		ostr << "\t<!-- contour " << groupidx << " -->\n";
		ostr << "\t<" << groupidx << ">\n";

		ostr << "\t\t<begin>" << std::get<0>(group)*2 << "</begin>\n";
		ostr << "\t\t<end>" << std::get<1>(group)*2 << "</end>\n";

		ostr << "\t</" << groupidx << ">\n\n";
	}
	ostr << "</groups>\n";

	// contour regions
	/*ostr << "\n<regions>\n";
	for(std::size_t contouridx = 0; contouridx<m_wallcontours.size(); ++contouridx)
	{
		const auto& contour = m_wallcontours[contouridx];
		ostr << "\t<!-- contour " << contouridx << " -->\n";
		ostr << "\t<" << contouridx << ">\n";

		for(std::size_t vertidx=0; vertidx<contour.size(); ++vertidx)
		{
			const t_contourvec& vec = contour[vertidx];

			ostr << "\t\t<" << vertidx;
			ostr << " x=\"" << vec[0] << "\"";
			ostr << " y=\"" << vec[1] << "\"";
			ostr << "/>\n";
		}

		ostr << "\t</" << contouridx << ">\n\n";
	}
	ostr << "</regions>\n";*/

	ostr << "</lines2d>" << std::endl;
	return true;
}
