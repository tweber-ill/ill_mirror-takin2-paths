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

using t_task = std::packaged_task<void()>;
using t_taskptr = std::shared_ptr<t_task>;


PathsBuilder::PathsBuilder()
	: m_sigProgress{std::make_shared<t_sig_progress>()}
{
}


PathsBuilder::~PathsBuilder()
{
}


void PathsBuilder::Clear()
{
	//m_img.Clear();
	m_wallcontours.clear();
	m_fullwallcontours.clear();

	m_lines.clear();
	m_linegroups.clear();

	m_voro_results.vertices.clear();
	m_voro_results.linear_edges.clear();
	m_voro_results.parabolic_edges.clear();
	m_voro_results.graph.Clear();
}


/**
 * convert a pixel of the plot image into the angular range of the plot 
 */
t_vec PathsBuilder::PixelToAngle(t_real img_x, t_real img_y, bool deg, bool inc_sense) const
{
	t_real x = std::lerp(m_sampleScatteringRange[0], m_sampleScatteringRange[1], 
		img_x / t_real(m_img.GetWidth()));
	t_real y = std::lerp(m_monoScatteringRange[0], m_monoScatteringRange[1], 
		img_y / t_real(m_img.GetHeight()));
	
	if(deg)
	{
		x *= t_real(180) / tl2::pi<t_real>;
		y *= t_real(180) / tl2::pi<t_real>;
	}

	if(inc_sense)
	{
		x *= m_sensesCCW[1];
		y *= m_sensesCCW[0];
	}

	return tl2::create<t_vec>({x, y});
}


/**
 * convert angular coordinates to a pixel in the plot image 
 */
t_vec PathsBuilder::AngleToPixel(t_real angle_x, t_real angle_y, bool deg, bool inc_sense) const
{
	if(deg)
	{
		angle_x *= tl2::pi<t_real> / t_real(180);
		angle_y *= tl2::pi<t_real> / t_real(180);
	}

	if(inc_sense)
	{
		angle_x *= m_sensesCCW[1];
		angle_y *= m_sensesCCW[0];
	}


	t_real x = std::lerp(t_real(0.), t_real(m_img.GetWidth()), 
		(angle_x - m_sampleScatteringRange[0]) / (m_sampleScatteringRange[1] - m_sampleScatteringRange[0]));
	t_real y = std::lerp(t_real(0.), t_real(m_img.GetHeight()), 
		(angle_y - m_monoScatteringRange[0]) / (m_monoScatteringRange[1] - m_monoScatteringRange[0]));
	
	return tl2::create<t_vec>({x, y});
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
bool PathsBuilder::CalculateConfigSpace(
	t_real da2, t_real da4, 
	t_real starta2, t_real enda2,
	t_real starta4, t_real enda4)
{
	if(!m_instrspace)
		return false;

	m_sampleScatteringRange[0] = starta4;
	m_sampleScatteringRange[1] = enda4;
	m_monoScatteringRange[0] = starta2;
	m_monoScatteringRange[1] = enda2;

	std::ostringstream ostrmsg;
	ostrmsg << "Calculating configuration space in " << m_maxnum_threads << " threads...";
	(*m_sigProgress)(true, false, 0, ostrmsg.str());

	// angles and ranges
	t_real a6 = m_instrspace->GetInstrument().GetAnalyser().GetAxisAngleOut();

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
	asio::thread_pool pool(m_maxnum_threads);

	std::vector<t_taskptr> tasks;
	tasks.reserve(img_h);


	// set image pixels
	std::atomic<std::size_t> num_pixels = 0;
	for(std::size_t img_row=0; img_row<img_h; ++img_row)
	{
		auto task = [this, img_w, img_row, a6, &num_pixels]()
		{
			InstrumentSpace instrspace_cpy = *this->m_instrspace;

			for(std::size_t img_col=0; img_col<img_w; ++img_col)
			{
				t_vec angle = PixelToAngle(img_col, img_row, false, true);
				t_real a4 = angle[0];
				t_real a2 = angle[1];
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

				++num_pixels;
			}

			//std::cout << a2/tl2::pi<t_real>*180. << " finished" << std::endl;
		};

		t_taskptr taskptr = std::make_shared<t_task>(task);
		tasks.push_back(taskptr);
		asio::post(pool, [taskptr]() { (*taskptr)(); });
	}


	// get results
	std::size_t num_tasks = tasks.size();
	// send no more than half-percentage update signals
	std::size_t signal_skip = num_tasks / 200;

	for(std::size_t taskidx=0; taskidx<num_tasks; ++taskidx)
	{
		// prevent sending too many progress signals
		if(signal_skip && taskidx % signal_skip == 0)
		{
			if(!(*m_sigProgress)(false, false, t_real(taskidx) / t_real(num_tasks), ostrmsg.str()))
			{
				pool.stop();
				break;
			}
		}

		tasks[taskidx]->get_future().get();
		//std::cout << taskidx << " / " << num_tasks << " finished" << std::endl;
	}

	pool.join();
	(*m_sigProgress)(false, true, 1, ostrmsg.str());

	//std::cout << "pixels total: " << img_h*img_w << ", calculated: " << num_pixels << std::endl;
	return num_pixels == img_h*img_w;
}


/**
 * calculate the contour lines of the obstacle regions
 */
bool PathsBuilder::CalculateWallContours(bool simplify, bool convex_split)
{
	std::string message{"Calculating obstacle contours..."};
	(*m_sigProgress)(true, false, 0, message);

	m_wallcontours = geo::trace_boundary<t_contourvec, decltype(m_img)>(m_img);
	m_fullwallcontours = m_wallcontours;

	(*m_sigProgress)(false, false, 0.33, message);

	if(simplify)
	{
		// iterate and simplify contour groups
		for(auto& contour : m_wallcontours)
		{
			// replace contour with its convex hull
			//std::vector<t_vec> contour_real = tl2::convert<t_vec, t_contourvec, std::vector>(contour);
			//auto [hull_verts, hull_lines, hull_indices]
			//	= geo::calc_delaunay<t_vec>(2, contour_real, true);
			//contour = tl2::convert<t_contourvec, t_vec, std::vector>(hull_verts);

			// simplify hull contour
			geo::simplify_contour<t_contourvec, t_real>(contour, 2., m_eps_angular);
		}
	}

	(*m_sigProgress)(false, false, 0.66, message);

	if(convex_split)
	{
		// convex split
		std::vector<std::vector<t_contourvec>> splitcontours;
		splitcontours.reserve(m_wallcontours.size()*2);

		for(auto& contour : m_wallcontours)
		{
			//std::reverse(contour.begin(), contour.end());
			auto slitcontour = geo::convex_split<t_contourvec, t_real>(contour, m_eps);
			if(slitcontour.size())
			{
				for(auto&& poly : slitcontour)
				{
					//std::reverse(poly.begin(), poly.end());
					splitcontours.emplace_back(std::move(poly));
				}
			}
			else
			{
				// no split, use original contour
				splitcontours.push_back(contour);
			}
		}

		m_wallcontours = std::move(splitcontours);
	}

	(*m_sigProgress)(false, true, 1, message);
	return true;
}


/**
 * calculate lines segments and groups
 */
bool PathsBuilder::CalculateLineSegments()
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
	return true;
}


/**
 * calculate the voronoi diagram
 */
bool PathsBuilder::CalculateVoronoi(bool group_lines)
{
	std::string message{"Calculating voronoi diagram..."};
	(*m_sigProgress)(true, false, 0, message);

	m_voro_results
		= geo::calc_voro<t_vec, t_line, t_graph>(
			m_lines, m_linegroups, group_lines, true, m_voroedge_eps);

	(*m_sigProgress)(false, true, 1, message);
	return true;
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


/**
 * find a path from an initial (a2, a4) to a final (a2, a4)
 */
InstrumentPath PathsBuilder::FindPath(
	t_real a2_i, t_real a4_i, 
	t_real a2_f, t_real a4_f)
{
	a2_i *= 180. / tl2::pi<t_real>;
	a4_i *= 180. / tl2::pi<t_real>;
	a2_f *= 180. / tl2::pi<t_real>;
	a4_f *= 180. / tl2::pi<t_real>;

#ifdef DEBUG
	std::cout << "a4_i = " << a4_i << ", a2_i = " << a2_i
		<< "; a4_f = " << a4_f << ", a2_f = " << a2_f 
		<< "." << std::endl;
#endif


	// vertices in configuration space
	t_vec vec_i = AngleToPixel(a4_i, a2_i, true);
	t_vec vec_f = AngleToPixel(a4_f, a2_f, true);

	std::cout << "start pixel: (" << vec_i[0] << ", " << vec_i[1] << std::endl; 
	std::cout << "target pixel: (" << vec_f[0] << ", " << vec_f[1] << std::endl; 

	// find closest voronoi vertices
	const auto& voro_vertices = m_voro_results.vertices;

	std::size_t idx_i = 0;
	std::size_t idx_f = 0;

	t_real mindist_i = std::numeric_limits<t_real>::max();
	t_real mindist_f = std::numeric_limits<t_real>::max();

	for(std::size_t idx_vert = 0; idx_vert < voro_vertices.size(); ++idx_vert)
	{
		const t_vec& cur_vert = voro_vertices[idx_vert];
		//std::cout << "cur_vert: " << cur_vert[0] << " " << cur_vert[1] << std::endl;

		t_vec diff_i = vec_i - cur_vert;
		t_vec diff_f = vec_f - cur_vert;

		t_real dist_i_sq = tl2::inner<t_vec>(diff_i, diff_i);
		t_real dist_f_sq = tl2::inner<t_vec>(diff_f, diff_f);

		if(dist_i_sq < mindist_i)
		{
			mindist_i = dist_i_sq;
			idx_i = idx_vert;
		}

		if(dist_f_sq < mindist_f)
		{
			mindist_f = dist_f_sq;
			idx_f = idx_vert;
		}
	}

#ifdef DEBUG
	std::cout << "Nearest voronoi vertices: " << idx_i << ", " << idx_f << "." << std::endl;
#endif


	// find the shortest path between the voronoi vertices
	const auto& voro_graph = m_voro_results.graph;

	const std::string& ident_i = voro_graph.GetVertexIdent(idx_i);
	const auto predecessors = geo::dijk(voro_graph, ident_i);

	InstrumentPath path{};
	std::size_t cur_vertidx = idx_f;

	while(true)
	{
		path.voronoi_indices.push_back(cur_vertidx);
		path.voronoi_vertices.push_back(voro_vertices[cur_vertidx]);

		if(cur_vertidx == idx_i)
		{
			path.ok = true;
			break;
		}

		auto next_vertidx = predecessors[cur_vertidx];
		if(!next_vertidx)
		{
			path.ok = false;
			break;
		}

		cur_vertidx = *next_vertidx;
	}

	std::reverse(path.voronoi_indices.begin(), path.voronoi_indices.end());
	std::reverse(path.voronoi_vertices.begin(), path.voronoi_vertices.end());

	std::cout << "Path ok: " << std::boolalpha << path.ok << std::endl;
	for(std::size_t idx=0; idx<path.voronoi_indices.size(); ++idx)
	{
		std::size_t voro_idx = path.voronoi_indices[idx];
		const t_vec& voro_vertex = path.voronoi_vertices[idx];
		const t_vec voro_angle = PixelToAngle(voro_vertex[0], voro_vertex[1], true);

		std::cout << "\tvertex index " << voro_idx << ": pixel (" 
			<< voro_vertex[0] << ", " << voro_vertex[1] << "), angle (" 
			<< voro_angle[0] << ", " << voro_angle[1] << ")" 
			<< std::endl;
	}

	return path;
}
