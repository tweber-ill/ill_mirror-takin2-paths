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

	m_voro_results.Clear();
}


/**
 * show progress messages on the console
 */
void PathsBuilder::AddConsoleProgressHandler()
{
	auto handler = []
		(bool start, bool end, t_real progress,
		const std::string& msg) -> bool
	{
		std::cout << std::fixed << "["
			<< std::setw(3) << (int)(progress * 100.)
			<< "%] " << msg << std::endl;
		return true;
	};

	AddProgressSlot(handler);
}


/**
 * convert a pixel of the plot image into the angular range of the plot
 */
t_vec2 PathsBuilder::PixelToAngle(t_real img_x, t_real img_y, bool deg, bool inc_sense) const
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

	const t_real *sensesCCW = nullptr;
	if(m_tascalc)
		sensesCCW = m_tascalc->GetScatteringSenses();

	if(inc_sense && sensesCCW)
	{
		x *= sensesCCW[1];
		y *= sensesCCW[0];
	}

	return tl2::create<t_vec2>({x, y});
}


/**
 * convert angular coordinates to a pixel in the plot image
 */
t_vec2 PathsBuilder::AngleToPixel(t_real angle_x, t_real angle_y, bool deg, bool inc_sense) const
{
	if(deg)
	{
		angle_x *= tl2::pi<t_real> / t_real(180);
		angle_y *= tl2::pi<t_real> / t_real(180);
	}

	const t_real *sensesCCW = nullptr;
	if(m_tascalc)
		sensesCCW = m_tascalc->GetScatteringSenses();

	if(inc_sense && sensesCCW)
	{
		angle_x *= sensesCCW[1];
		angle_y *= sensesCCW[0];
	}


	t_real x = std::lerp(t_real(0.), t_real(m_img.GetWidth()),
		(angle_x - m_sampleScatteringRange[0]) / (m_sampleScatteringRange[1] - m_sampleScatteringRange[0]));
	t_real y = std::lerp(t_real(0.), t_real(m_img.GetHeight()),
		(angle_y - m_monoScatteringRange[0]) / (m_monoScatteringRange[1] - m_monoScatteringRange[0]));

	return tl2::create<t_vec2>({x, y});
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

	const t_real *sensesCCW = nullptr;
	if(m_tascalc)
		sensesCCW = m_tascalc->GetScatteringSenses();

	// include scattering senses
	if(sensesCCW)
	{
		da4 *= sensesCCW[1];
		starta4 *= sensesCCW[1];
		enda4 *= sensesCCW[1];

		da2 *= sensesCCW[0];
		starta2 *= sensesCCW[0];
		enda2 *= sensesCCW[0];
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
				t_vec2 angle = PixelToAngle(img_col, img_row, false, true);
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
				bool angle_ok = instrspace_cpy.CheckAngularLimits();

				if(!angle_ok)
				{
					m_img.SetPixel(img_col, img_row, 0xf0);
				}
				else
				{
					bool colliding = instrspace_cpy.CheckCollision2D();
					m_img.SetPixel(img_col, img_row, colliding ? 0xff : 0x00);
				}

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
	// send no more than four-percent update signals
	std::size_t signal_skip = num_tasks / 25;

	for(std::size_t taskidx=0; taskidx<num_tasks; ++taskidx)
	{
		// prevent sending too many progress signals
		if(signal_skip && (taskidx % signal_skip == 0))
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
			//std::vector<t_vec2> contour_real = tl2::convert<t_vec2, t_contourvec, std::vector>(contour);
			//auto [hull_verts, hull_lines, hull_indices]
			//	= geo::calc_delaunay<t_vec2>(2, contour_real, true);
			//contour = tl2::convert<t_contourvec, t_vec2, std::vector>(hull_verts);

			// simplify hull contour
			geo::simplify_contour<t_contourvec, t_real>(contour, m_simplify_mindist, m_eps_angular, m_eps);
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
			auto splitcontour = geo::convex_split<t_contourvec, t_real>(contour, m_eps);
			if(splitcontour.size())
			{
				for(auto&& poly : splitcontour)
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
	m_points_outside_regions.clear();
	m_inverted_regions.clear();

	// find an arbitrary point outside all obstacles
	auto find_point_outside_regions = [this]
	(std::size_t x_start = 0, std::size_t y_start = 0,
		bool skip_search = false) -> t_vec2
	{
		t_vec2 point_outside_regions = tl2::create<t_vec2>({-50, -40});
		bool found_point = false;

		if(!skip_search)
		{
			for(std::size_t y=y_start; y<m_img.GetHeight(); ++y)
			{
				for(std::size_t x=x_start; x<m_img.GetWidth(); ++x)
				{
					if(m_img.GetPixel(x, y) == 0)
					{
						point_outside_regions =
							tl2::create<t_vec2>({
								static_cast<double>(x),
								static_cast<double>(y)
							});
						found_point = true;
						break;
					}
				}
				if(found_point)
					break;
			}
		}

		return point_outside_regions;
	};

	std::size_t totalverts = 0;
	for(const auto& contour : m_wallcontours)
		totalverts += contour.size();

	m_lines.reserve(totalverts/2 + 1);
	m_linegroups.reserve(m_wallcontours.size());
	m_points_outside_regions.reserve(m_wallcontours.size());
	m_inverted_regions.reserve(m_wallcontours.size());

	// contour vertices
	std::size_t linectr = 0;
	for(std::size_t contouridx = 0; contouridx < m_wallcontours.size(); ++contouridx)
	{
		const auto& contour = m_wallcontours[contouridx];
		std::size_t groupstart = linectr;
		t_contourvec contour_mean = tl2::zero<t_vec2>(2);

		for(std::size_t vert1 = 0; vert1 < contour.size(); ++vert1)
		{
			std::size_t vert2 = (vert1 + 1) % contour.size();

			const t_contourvec& vec1 = contour[vert1];
			const t_contourvec& vec2 = contour[vert2];
			contour_mean += vec1;

			t_vec2 linevec1 = vec1;
			t_vec2 linevec2 = vec2;

			m_lines.emplace_back(std::make_pair(std::move(linevec1), std::move(linevec2)));

			++linectr;
		}

		contour_mean /= contour.size();

		// move a point on the contour in the direction of the contour mean
		// to get a point inside the contour
		t_contourvec inside_contour = contour[0] + (contour_mean-contour[0]) / 8;

		// find a point outside the contour by moving a pixel away from the minimum vertex
		auto [contour_min, contour_max] = tl2::minmax(contour);
		t_contourvec outside_contour = contour_min;
		for(int i = 0; i < 2; ++i)
			outside_contour[i] -= 1;

		// mark line group start and end index
		std::size_t groupend = linectr;

		// don't include outer bounding region
		// TODO: test if such a region is there
		if(contouridx > 0)
		{
			m_linegroups.emplace_back(std::make_pair(groupstart, groupend));

			t_vec2 point_outside_regions =
				find_point_outside_regions(contour[0][0], contour[0][1], true);
			m_points_outside_regions.emplace_back(std::move(point_outside_regions));

			auto pix_incontour = m_img.GetPixel(inside_contour[0], inside_contour[1]);
			auto pix_outcontour = m_img.GetPixel(outside_contour[0], outside_contour[1]);

#ifdef DEBUG
			std::cout << "contour " << std::dec << contouridx
				<< ", pixel inside " << inside_contour[0] << ", " << inside_contour[1]
				<< ": " << std::hex << int(pix_incontour) << std::dec
				<< "; pixel outside " << outside_contour[0] << ", " << outside_contour[1]
				<< ": " << std::hex << int(pix_outcontour) << std::endl;
#endif

			// normal regions encircle forbidden coordinate points
			// inverted regions encircle allowed coordinate points
			//m_inverted_regions.push_back(pix_incontour == 0);
			m_inverted_regions.push_back(pix_outcontour != 0);
		}
	}

	(*m_sigProgress)(false, true, 1, message);
	return true;
}


/**
 * calculate the voronoi diagram
 */
bool PathsBuilder::CalculateVoronoi(bool group_lines)
{
	std::string message{"Calculating Voronoi diagram..."};
	(*m_sigProgress)(true, false, 0, message);

	m_voro_results
		= geo::calc_voro<t_vec2, t_line, t_graph>(
			m_lines, m_linegroups, group_lines, true,
			m_voroedge_eps, &m_points_outside_regions,
			&m_inverted_regions);

	(*m_sigProgress)(false, true, 1, message);
	return true;
}


/**
 * save the contour line segments to the lines tool
 */
bool PathsBuilder::SaveToLinesTool(std::ostream& ostr)
{
	ostr << "<lines2d>\n";
	std::vector<std::pair<std::size_t, std::size_t>> group_indices;
	group_indices.reserve(m_linegroups.size());

	// contour vertices
	std::size_t vertctr = 0;
	ostr << "<vertices>\n";
	for(std::size_t contouridx = 0; contouridx < m_linegroups.size(); ++contouridx)
	{
		const auto& contour = m_linegroups[contouridx];
		ostr << "\t<!-- contour " << contouridx << " -->\n";

		std::size_t group_begin = vertctr;

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

		std::size_t group_end = vertctr;
		group_indices.emplace_back(std::make_pair(group_begin, group_end));
	}
	ostr << "</vertices>\n";

	// contour groups
	ostr << "\n<groups>\n";
	for(std::size_t groupidx = 0; groupidx < group_indices.size(); ++groupidx)
	{
		const auto& group = group_indices[groupidx];
		ostr << "\t<!-- contour " << groupidx << " -->\n";
		ostr << "\t<" << groupidx << ">\n";

		ostr << "\t\t<begin>" << std::get<0>(group) << "</begin>\n";
		ostr << "\t\t<end>" << std::get<1>(group) << "</end>\n";

		ostr << "\t</" << groupidx << ">\n\n";
	}
	ostr << "</groups>\n";

	// alternatively: contour regions (obsolete)
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
 * save the contour line segments to the lines tool
 */
bool PathsBuilder::SaveToLinesTool(const std::string& filename)
{
	std::ofstream ofstr(filename);
	if(!ofstr)
		return false;

	return SaveToLinesTool(ofstr);
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

	InstrumentPath path{};
	path.ok = false;

	// vertices in configuration space
	path.vec_i = AngleToPixel(a4_i, a2_i, true);
	path.vec_f = AngleToPixel(a4_f, a2_f, true);

#ifdef DEBUG
	std::cout << "start pixel: (" << path.vec_i[0] << ", " << path.vec_i[1] << std::endl;
	std::cout << "target pixel: (" << path.vec_f[0] << ", " << path.vec_f[1] << std::endl;
#endif

	// find closest voronoi vertices
	const auto& voro_vertices = m_voro_results.GetVoronoiVertices();

	// no voronoi vertices available
	if(voro_vertices.size() == 0)
		return path;


	std::size_t idx_i = 0;
	std::size_t idx_f = 0;

	// calculation of closest voronoi vertices using the index tree
	if(m_voro_results.GetIndexTreeSize())
	{
		std::vector<std::size_t> indices_i =
			m_voro_results.GetClosestVoronoiVertices(path.vec_i, 1);
		if(indices_i.size())
			idx_i = indices_i[0];

		std::vector<std::size_t> indices_f =
			m_voro_results.GetClosestVoronoiVertices(path.vec_f, 1);
		if(indices_f.size())
			idx_f = indices_f[0];
	}

	// alternate calculation without index tree
	else
	{
		t_real mindist_i = std::numeric_limits<t_real>::max();
		t_real mindist_f = std::numeric_limits<t_real>::max();

		for(std::size_t idx_vert = 0; idx_vert < voro_vertices.size(); ++idx_vert)
		{
			const t_vec2& cur_vert = voro_vertices[idx_vert];
			//std::cout << "cur_vert: " << cur_vert[0] << " " << cur_vert[1] << std::endl;

			t_vec2 diff_i = path.vec_i - cur_vert;
			t_vec2 diff_f = path.vec_f - cur_vert;

			t_real dist_i_sq = tl2::inner<t_vec2>(diff_i, diff_i);
			t_real dist_f_sq = tl2::inner<t_vec2>(diff_f, diff_f);

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
	}

#ifdef DEBUG
	std::cout << "Nearest voronoi vertices: " << idx_i << ", " << idx_f << "." << std::endl;
#endif


	// find the shortest path between the voronoi vertices
	const auto& voro_graph = m_voro_results.GetVoronoiGraph();

	// are the graph vertex indices valid?
	if(idx_i >= voro_graph.GetNumVertices() || idx_f >= voro_graph.GetNumVertices())
		return path;

	const std::string& ident_i = voro_graph.GetVertexIdent(idx_i);

#if TASPATHS_SSSP_IMPL==1
	const auto predecessors = geo::dijk(voro_graph, ident_i);
#elif TASPATHS_SSSP_IMPL==2
	const auto predecessors = geo::dijk_mod(voro_graph, ident_i);
#elif TASPATHS_SSSP_IMPL==3
	const auto [distvecs, predecessors] = geo::bellman(voro_graph, ident_i);
#else
	#error No suitable value for TASPATHS_SSSP_IMPL has been set!
#endif
	std::size_t cur_vertidx = idx_f;

	while(true)
	{
		path.voronoi_indices.push_back(cur_vertidx);

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

#ifdef DEBUG
	std::cout << "Path ok: " << std::boolalpha << path.ok << std::endl;
	for(std::size_t idx=0; idx<path.voronoi_indices.size(); ++idx)
	{
		std::size_t voro_idx = path.voronoi_indices[idx];
		const t_vec2& voro_vertex = voro_vertices[voro_idx];
		const t_vec2 voro_angle = PixelToAngle(voro_vertex[0], voro_vertex[1], true);

		std::cout << "\tvertex index " << voro_idx << ": pixel ("
			<< voro_vertex[0] << ", " << voro_vertex[1] << "), angle ("
			<< voro_angle[0] << ", " << voro_angle[1] << ")"
			<< std::endl;
	}
#endif


	// find closest point on a path segment
	auto closest_point = [&voro_vertices]
	(std::size_t idx1, std::size_t idx2, const t_vec2& vec)
		-> std::tuple<t_real, t_real>
	{
		const t_vec2& vert1 = voro_vertices[idx1];
		const t_vec2& vert2 = voro_vertices[idx2];

		t_vec2 dir = vert2 - vert1;
		t_real dir_len = tl2::norm<t_vec2>(dir);
		dir /= dir_len;

		auto [ptProj, dist, paramProj] =
			tl2::project_line<t_vec2, t_real>(
				vec, vert1, dir, true);

		paramProj /= dir_len;
		return std::make_tuple(paramProj, dist);
	};


	if(path.voronoi_indices.size() >= 2)
	{
		// find closest start point
		std::size_t vert_idx1_begin = path.voronoi_indices[0];
		std::size_t vert_idx2_begin = path.voronoi_indices[1];

		std::size_t min_dist_idx_begin = vert_idx2_begin;
		auto [min_param_begin, min_dist_begin] =
			closest_point(vert_idx1_begin, vert_idx2_begin, path.vec_i);

		// check if any neighbour path before first vertex is even closer
		for(std::size_t neighbour_idx :
			voro_graph.GetNeighbours(vert_idx1_begin))
		{
			if(neighbour_idx == vert_idx2_begin)
				continue;

			auto [neighbour_param, neighbour_dist] =
				closest_point(neighbour_idx, vert_idx1_begin, path.vec_i);

			// choose a new position on the adjacent edge if it's either
			// closer or if the former parameters had been out of bounds
			// and are now within [0, 1]
			if((neighbour_param >= 0. && neighbour_param <= 1.)
				&& (neighbour_dist < min_dist_begin
				|| (min_param_begin < 0. || min_param_begin > 1.)))
			{
				min_dist_begin = neighbour_dist;
				min_param_begin = neighbour_param;
				min_dist_idx_begin = neighbour_idx;
			}
		}

		// a neighbour edge is closer
		if(min_dist_idx_begin != vert_idx2_begin)
			path.voronoi_indices.insert(path.voronoi_indices.begin(), min_dist_idx_begin);

		path.param_begin = min_param_begin;

		if(path.param_begin > 1.)
			path.param_begin = 1.;
		else if(path.param_begin < 0.)
			path.param_begin = 0.;


		// find closest end point
		std::size_t vert_idx1_end = *(path.voronoi_indices.rbegin()+1);
		std::size_t vert_idx2_end = *path.voronoi_indices.rbegin();
		std::size_t min_dist_idx_end = vert_idx1_end;
		auto [min_param_end, min_dist_end] =
			closest_point(vert_idx1_end, vert_idx2_end, path.vec_f);

		// check if any neighbour path before first vertex is even closer
		for(std::size_t neighbour_idx : voro_graph.GetNeighbours(vert_idx2_end))
		{
			if(neighbour_idx == vert_idx1_end)
				continue;

			auto [neighbour_param, neighbour_dist] =
				closest_point(vert_idx2_end, neighbour_idx, path.vec_f);

			// choose a new position on the adjacent edge if it's either
			// closer or if the former parameters had been out of bounds
			// and are now within [0, 1]
			if((neighbour_param >= 0. && neighbour_param <= 1.)
				&& (neighbour_dist < min_dist_end
				|| (min_param_end < 0. || min_param_end > 1.)))
			{
				min_dist_end = neighbour_dist;
				min_param_end = neighbour_param;
				min_dist_idx_end = neighbour_idx;
			}
		}

		// a neighbour edge is closer
		if(min_dist_idx_end != vert_idx1_end)
			path.voronoi_indices.push_back(min_dist_idx_end);

		path.param_end = min_param_end;

		if(path.param_end > 1.)
			path.param_end = 1.;
		else if(path.param_end < 0.)
			path.param_end = 0.;
	}

	return path;
}


/**
 * get individual vertices on an instrument path
 */
std::vector<t_vec2> PathsBuilder::GetPathVertices(
	const InstrumentPath& path, bool subdivide_lines, bool deg) const
{
	std::vector<t_vec2> path_vertices;

	if(!path.ok)
		return path_vertices;

	const auto& voro_results = GetVoronoiResults();
	const auto& voro_vertices = voro_results.GetVoronoiVertices();


	// convert pixel to angular coordinates and add vertex to path
	auto add_curve_vertex = [&path_vertices, deg, this](const t_vec2& vertex)
	{
		const t_vec2 angle = PixelToAngle(vertex[0], vertex[1], deg);
		path_vertices.emplace_back(std::move(angle));
	};


	// add starting point
	add_curve_vertex(path.vec_i);

	// iterate voronoi vertices
	for(std::size_t idx=1; idx<path.voronoi_indices.size(); ++idx)
	{
		std::size_t voro_idx = path.voronoi_indices[idx];
		const t_vec2& voro_vertex = voro_vertices[voro_idx];
		bool is_linear_bisector = true;

		// check if the current one is a quadratic bisector
		std::size_t prev_voro_idx = path.voronoi_indices[idx-1];
		auto iter_quadr = voro_results.GetParabolicEdges().find(
			std::make_pair(prev_voro_idx, voro_idx));

		if(iter_quadr != voro_results.GetParabolicEdges().end())
		{
			// it's a quadratic bisector
			is_linear_bisector = false;

			// get correct iteration order of bisector,
			// which is stored in an unordered fashion
			bool inverted_iter_order = false;
			const std::vector<t_vec2>& vertices = iter_quadr->second;
			if(vertices.size() && tl2::equals<t_vec2>(vertices[0], voro_vertex, m_eps))
				inverted_iter_order = true;

			std::ptrdiff_t begin_idx = 0;
			std::ptrdiff_t end_idx = 0;

			// use the closest position on the path for the initial vertex
			if(idx == 1)
			{
				begin_idx = path.param_begin * vertices.size();
				if(begin_idx >= (std::ptrdiff_t)vertices.size())
					begin_idx = (std::ptrdiff_t)(vertices.size()-1);
				if(begin_idx < 0)
					begin_idx = 0;
			}
			// use the closest position on the path for the final vertex
			else if(idx == path.voronoi_indices.size()-1)
			{
				end_idx = (1.-path.param_end) * vertices.size();
				if(end_idx >= (std::ptrdiff_t)vertices.size())
					end_idx = (std::ptrdiff_t)(vertices.size()-1);
				if(end_idx < 0)
					end_idx = 0;
			}

			if(inverted_iter_order)
			{
				for(auto iter_vert = vertices.rbegin()+begin_idx; iter_vert != vertices.rend()-end_idx; ++iter_vert)
					add_curve_vertex(*iter_vert);
			}
			else
			{
				for(auto iter_vert = vertices.begin()+begin_idx; iter_vert != vertices.end()-end_idx; ++iter_vert)
					add_curve_vertex(*iter_vert);
			}
		}

		// if it's a linear bisector, just connect the voronoi vertices
		if(is_linear_bisector)
		{
			// use the closest position on the path for the initial vertex
			if(idx == 1 && path.voronoi_indices.size() > 1)
			{
				const t_vec2& voro_vertex1 = voro_vertices[path.voronoi_indices[0]];
				add_curve_vertex(voro_vertex1 + path.param_begin*(voro_vertex-voro_vertex1));
			}
			// use the closest position on the path for the final vertex
			else if(idx == path.voronoi_indices.size()-1 && idx > 1)
			{
				const t_vec2& voro_vertex1 = voro_vertices[path.voronoi_indices[idx-1]];
				add_curve_vertex(voro_vertex1 + path.param_end*(voro_vertex-voro_vertex1));
			}
			else
			{
				add_curve_vertex(voro_vertex);
			}
		}
	}

	// add target point
	add_curve_vertex(path.vec_f);


	// interpolate points on path line segments
	if(subdivide_lines)
	{
		path_vertices = geo::subdivide_lines<t_vec2>(path_vertices, m_subdiv_len);
	}

	return path_vertices;
}


/**
 * get individual vertices on an instrument path
 * helper function for the scripting interface
 */
std::vector<std::pair<t_real, t_real>> PathsBuilder::GetPathVerticesAsPairs(
	const InstrumentPath& path, bool subdivide_lines, bool deg) const
{
	std::vector<t_vec2> vertices = GetPathVertices(path, subdivide_lines, deg);

	std::vector<std::pair<t_real, t_real>> pairs;
	pairs.reserve(vertices.size());

	for(const t_vec2& vec : vertices)
		pairs.emplace_back(std::make_pair(vec[0], vec[1]));

	return pairs;
}


/**
 * get a line segment group
 * helper function for the scripting interface
 */
std::vector<std::array<t_real, 4>>
PathsBuilder::GetLineSegmentRegionAsArray(std::size_t groupidx) const
{
	std::vector<std::array<t_real, 4>> lines;

	if(groupidx >= GetNumberOfLineSegmentRegions())
		return lines;
	auto[startidx, endidx] = m_linegroups[groupidx];

	lines.reserve(endidx - startidx);

	for(std::size_t lineidx=startidx; lineidx<endidx; ++lineidx)
	{
		const t_line& line = m_lines[lineidx];

		t_vec2 pt1 = PixelToAngle(std::get<0>(line)[0], std::get<0>(line)[1], true, false);
		t_vec2 pt2 = PixelToAngle(std::get<1>(line)[0], std::get<1>(line)[1], true, false);

		std::array<t_real, 4> arr{{ pt1[0], pt1[1], pt2[0], pt2[1] }};
		lines.emplace_back(std::move(arr));
	}

	return lines;
}
