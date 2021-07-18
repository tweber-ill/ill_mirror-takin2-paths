/**
 * voronoi diagrams for line segments
 * @author Tobias Weber <tweber@ill.fr>
 * @date Oct/Nov-2020 - Jul-2021
 * @note Forked on 19-apr-2021 and 3-jun-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 *
 * References for the algorithms:
 *   - (Klein 2005) "Algorithmische Geometrie" (2005), ISBN: 978-3540209560 (http://dx.doi.org/10.1007/3-540-27619-X).
 *   - (FUH 2020) "Algorithmische Geometrie" (2020), Kurs 1840, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/1840).
 *   - (Berg 2008) "Computational Geometry" (2008), ISBN: 978-3-642-09681-5 (http://dx.doi.org/10.1007/978-3-540-77974-2).
 */

#ifndef __GEO_ALGOS_VORONOI_LINES_H__
#define __GEO_ALGOS_VORONOI_LINES_H__

#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>
#include <voronoi_visual_utils.hpp>

#ifdef USE_OVD
	#include <openvoronoi/voronoidiagram.hpp>
#endif

#include "tlibs2/libs/maths.h"
#include "hull.h"
#include "lines.h"
#include "graphs.h"
#include "hashes.h"
#include "circular_iterator.h"


// ----------------------------------------------------------------------------
// make point and line segment classes known for boost.polygon
// @see https://www.boost.org/doc/libs/1_75_0/libs/polygon/doc/gtl_custom_point.htm
// @see https://github.com/boostorg/polygon/blob/develop/example/voronoi_basic_tutorial.cpp
// ----------------------------------------------------------------------------
template<class t_vec> requires tl2::is_vec<t_vec>
struct boost::polygon::geometry_concept<t_vec>
{
	using type = boost::polygon::point_concept;
};


template<class t_vec> requires tl2::is_vec<t_vec>
struct boost::polygon::geometry_concept<std::pair<t_vec, t_vec>>
{
	using type = boost::polygon::segment_concept;
};


template<class t_vec> requires tl2::is_vec<t_vec>
struct boost::polygon::point_traits<t_vec>
{
	using coordinate_type = typename t_vec::value_type;

	static coordinate_type get(const t_vec& vec, boost::polygon::orientation_2d orientation)
	{
		return vec[orientation.to_int()];
	}
};


template<class t_vec> requires tl2::is_vec<t_vec>
struct boost::polygon::segment_traits<std::pair<t_vec, t_vec>>
{
	using coordinate_type = typename t_vec::value_type;
	using point_type = t_vec;
	using line_type = std::pair<t_vec, t_vec>; // for convenience, not part of interface

	static const point_type& get(const line_type& line, boost::polygon::direction_1d direction)
	{
		switch(direction.to_int())
		{
			case 1: return std::get<1>(line);
			case 0: default: return std::get<0>(line);
		}
	}
};
// ----------------------------------------------------------------------------


namespace geo {

template<class t_vec, 
	class t_line = std::pair<t_vec, t_vec>,
	class t_graph = AdjacencyMatrix<typename t_vec::value_type>>
requires tl2::is_vec<t_vec>
struct VoronoiLinesResults
{
	using t_vert_index = std::size_t;
	using t_vert_indices = std::pair<t_vert_index, t_vert_index>;

	using t_vert_index_opt = std::optional<t_vert_index>;
	using t_vert_indices_opt = std::pair<t_vert_index_opt, t_vert_index_opt>;


	struct t_vert_hash
	{
		std::size_t operator()(const t_vert_indices& idx) const
		{
			return unordered_hash(std::get<0>(idx), std::get<1>(idx));
		}
	};

	struct t_vert_equ
	{
		bool operator()(const t_vert_indices& a, const t_vert_indices& b) const
		{
			bool ok1 = (std::get<0>(a) == std::get<0>(b)) && (std::get<1>(a) == std::get<1>(b));
			bool ok2 = (std::get<0>(a) == std::get<1>(b)) && (std::get<1>(a) == std::get<0>(b));

			return ok1 || ok2;
		}
	};


	struct t_vert_hash_opt
	{
		std::size_t operator()(const t_vert_indices_opt& idx) const
		{
			t_vert_indices _idx =
				std::make_pair(
					std::get<0>(idx) ? *std::get<0>(idx) : std::numeric_limits<t_vert_index>::max(),
					std::get<1>(idx) ? *std::get<1>(idx) : std::numeric_limits<t_vert_index>::max());

			return t_vert_hash{}(_idx);
		}
	};

	struct t_vert_equ_opt
	{
		bool operator()(const t_vert_indices_opt& a, const t_vert_indices_opt& b) const
		{
			t_vert_indices _a =
				std::make_pair(
					std::get<0>(a) ? *std::get<0>(a) : std::numeric_limits<t_vert_index>::max(),
					std::get<1>(a) ? *std::get<1>(a) : std::numeric_limits<t_vert_index>::max());

			t_vert_indices _b =
				std::make_pair(
					std::get<0>(b) ? *std::get<0>(b) : std::numeric_limits<t_vert_index>::max(),
					std::get<1>(b) ? *std::get<1>(b) : std::numeric_limits<t_vert_index>::max());

			return t_vert_equ{}(_a, _b);
		}
	};


	// linear bisectors
	std::unordered_map<t_vert_indices_opt, t_line, t_vert_hash_opt, t_vert_equ_opt>
		linear_edges{};

	// quadratic bisectors
	std::unordered_map<t_vert_indices, std::vector<t_vec>, t_vert_hash, t_vert_equ>
		parabolic_edges{};

	// vertices
	std::vector<t_vec> vertices;

	// voronoi vertex graph
	// graph vertex indices correspond to those of the "vertices" vector
	t_graph graph;
};


/**
 * voronoi diagram for line segments
 * @see https://github.com/boostorg/polygon/blob/develop/example/voronoi_basic_tutorial.cpp
 * @see https://www.boost.org/doc/libs/1_76_0/libs/polygon/example/voronoi_advanced_tutorial.cpp
 * @see https://github.com/boostorg/polygon/blob/develop/example/voronoi_visual_utils.hpp
 * @see https://github.com/boostorg/polygon/blob/develop/example/voronoi_visualizer.cpp
 * @see https://www.boost.org/doc/libs/1_75_0/libs/polygon/doc/voronoi_diagram.htm
 */
template<class t_vec, 
	class t_line = std::pair<t_vec, t_vec>,
	class t_graph = AdjacencyMatrix<typename t_vec::value_type>,
	class t_int = int>
VoronoiLinesResults<t_vec, t_line, t_graph>
calc_voro(const std::vector<t_line>& lines, 
	std::vector<std::pair<std::size_t, std::size_t>>& line_groups,
	bool group_lines = true, bool remove_voronoi_vertices_in_regions = false,
	typename t_vec::value_type edge_eps = 1e-2)
requires tl2::is_vec<t_vec> && is_graph<t_graph>
{
	using t_real = typename t_vec::value_type;
	namespace poly = boost::polygon;

	VoronoiLinesResults<t_vec, t_line, t_graph> results;

	// internal scale for int-conversion
	const t_real eps = edge_eps*edge_eps;
	const t_real scale = std::ceil(1./eps);

	// length of infinite edges
	t_real infline_len = 1.;
	for(const t_line& line : lines)
	{
		t_vec dir = std::get<1>(line) - std::get<0>(line);
		t_real len = tl2::norm(dir);
		infline_len = std::max(infline_len, len);
	}
	infline_len *= 10.;


	// type traits
	struct t_vorotraits
	{
		using coordinate_type = t_real;
		using vertex_type = poly::voronoi_vertex<coordinate_type>;
		using edge_type = poly::voronoi_edge<coordinate_type>;
		using cell_type = poly::voronoi_cell<coordinate_type>;

		struct vertex_equality_predicate_type
		{
			bool operator()(const vertex_type& vert1, const vertex_type& vert2) const
			{
				const t_real eps = 1e-3;
				return tl2::equals(vert1.x(), vert2.x(), eps)
					&& tl2::equals(vert1.y(), vert2.y(), eps);
			}
		};
	};


	poly::voronoi_builder<t_int> vorobuilder;
	for(const t_line& line : lines)
	{
		t_int x1 = t_int(std::get<0>(line)[0]*scale);
		t_int y1 = t_int(std::get<0>(line)[1]*scale);
		t_int x2 = t_int(std::get<1>(line)[0]*scale);
		t_int y2 = t_int(std::get<1>(line)[1]*scale);

		vorobuilder.insert_segment(x1, y1, x2, y2);
	}

	poly::voronoi_diagram<t_real, t_vorotraits> voro;
	vorobuilder.construct(&voro);


	// get line segment index
	auto get_segment_idx = 
		[](const typename t_vorotraits::edge_type& edge, bool twin) 
			-> std::optional<std::size_t>
	{
		const auto* cell = twin ? edge.twin()->cell() : edge.cell();
		if(!cell)
			return std::nullopt;

		return cell->source_index();
	};

	// get the group index of the line segment
	auto get_group_idx = [&line_groups](std::size_t segidx)
		-> std::optional<std::size_t>
	{
		for(std::size_t grpidx=0; grpidx<line_groups.size(); ++grpidx)
		{
			auto [grp_beg, grp_end] = line_groups[grpidx];

			if(segidx >= grp_beg && segidx < grp_end)
				return grpidx;
		}

		// line is in neither region
		return std::nullopt;
	};


	// graph of voronoi vertices
	t_graph& graph = results.graph;

	// voronoi vertices
	std::vector<const typename t_vorotraits::vertex_type*> vorovertices;
	auto& vertices = results.vertices;
	vorovertices.reserve(voro.vertices().size());
	vertices.reserve(voro.vertices().size());

	for(std::size_t vertidx=0; vertidx<voro.vertices().size(); ++vertidx)
	{
		const typename t_vorotraits::vertex_type* vert = &voro.vertices()[vertidx];
		t_vec vorovert = tl2::create<t_vec>({ vert->x()/scale, vert->y()/scale });

		vorovertices.push_back(vert);
		vertices.emplace_back(std::move(vorovert));
		graph.AddVertex(std::to_string(vertices.size()));
	}


	auto get_vertex_idx = 
		[&vorovertices](const typename t_vorotraits::vertex_type* vert) 
			-> std::optional<std::size_t>
	{
		// infinite edge?
		if(!vert)
			return std::nullopt;

		std::size_t idx = 0;
		for(const typename t_vorotraits::vertex_type* vertex : vorovertices)
		{
			if(vertex == vert)
				return idx;
			++idx;
		}

		return std::nullopt;
	};


	// edges
	auto& all_parabolic_edges = results.parabolic_edges;
	auto& linear_edges = results.linear_edges;

	// TODO: get rid of these and use the above maps directly
	std::vector<std::tuple<t_line,
		std::optional<std::size_t>,
		std::optional<std::size_t>>> linear_edges_vec;

	std::vector<std::tuple<std::vector<t_vec>,
		std::size_t, std::size_t>> all_parabolic_edges_vec;

	linear_edges_vec.reserve(voro.edges().size());


	for(const auto& edge : voro.edges())
	{
		// only bisectors, no internal edges
		if(edge.is_secondary())
			continue;

		// add graph edges
		const auto* vert0 = edge.vertex0();
		const auto* vert1 = edge.vertex1();
		auto vert0idx = get_vertex_idx(vert0);
		auto vert1idx = get_vertex_idx(vert1);
		bool valid_vertices = vert0idx && vert1idx;

		// line groups defined?
		if(line_groups.size())
		{
			auto seg1idx = get_segment_idx(edge, false);
			auto seg2idx = get_segment_idx(edge, true);

			if(seg1idx && seg2idx)
			{
				auto region1 = get_group_idx(*seg1idx);
				auto region2 = get_group_idx(*seg2idx);

				if(group_lines)
				{
					// are the generating line segments part of the same group?
					// if so, ignore this voronoi edge and skip to next one
					if(region1 && region2 && *region1 == *region2)
						continue;
				}
			}


			// remove the voronoi vertex if it's inside a region defined by a line group
			if(remove_voronoi_vertices_in_regions)
			{
				bool vert_inside_region = false;

				for(std::size_t grpidx=0; grpidx<line_groups.size(); ++grpidx)
				{
					auto [grp_beg, grp_end] = line_groups[grpidx];

					// check edge vertex 0
					if(vert0idx)
					{
						const auto& vorovert = vertices[*vert0idx];
						if(vert_inside_region = pt_inside_poly<t_vec>(lines, vorovert, grp_beg, grp_end, eps); vert_inside_region)
							break;
					}

					// check edge vertex 1
					if(vert1idx)
					{
						const auto& vorovert = vertices[*vert1idx];
						if(vert_inside_region = pt_inside_poly<t_vec>(lines, vorovert, grp_beg, grp_end, eps); vert_inside_region)
							break;
					}
				}

				// ignore this voronoi edge and skip to next one
				if(vert_inside_region)
					continue;
			}
		}

		if(valid_vertices)
		{
			// add to graph, TODO: arc length of parabolic edges
			t_real len = tl2::norm(vertices[*vert1idx] - vertices[*vert0idx]);

			graph.AddEdge(*vert0idx, *vert1idx, len);
			graph.AddEdge(*vert1idx, *vert0idx, len);
		}

		if(edge.is_finite() && !valid_vertices)
			continue;


		// get line segment
		auto get_segment = [&get_segment_idx, &edge, &lines](bool twin) 
			-> const t_line*
		{
			auto idx = get_segment_idx(edge, twin);
			if(!idx)
				return nullptr;

			const t_line& line = lines[*idx];
			return &line;
		};


		// get line segment endpoint
		auto get_segment_point = [&edge, &get_segment](bool twin) 
			-> const t_vec*
		{
			const auto* cell = twin ? edge.twin()->cell() : edge.cell();
			if(!cell)
				return nullptr;

			const t_line* line = get_segment(twin);
			const t_vec* vec = nullptr;
			if(!line)
				return nullptr;

			switch(cell->source_category())
			{
				case poly::SOURCE_CATEGORY_SEGMENT_START_POINT:
					vec = &std::get<0>(*line);
					break;
				case poly::SOURCE_CATEGORY_SEGMENT_END_POINT:
					vec = &std::get<1>(*line);
					break;
				default:
					break;
			}

			return vec;
		};


		// converter functions
		auto to_point_data = [](const t_vec& vec) -> poly::point_data<t_real>
		{
			return poly::point_data<t_real>{vec[0], vec[1]};
		};

		auto vertex_to_point_data = [&scale](const typename t_vorotraits::vertex_type& vec) 
			-> poly::point_data<t_real>
		{
			return poly::point_data<t_real>{vec.x()/scale, vec.y()/scale};
		};

		auto to_vec = [](const poly::point_data<t_real>& pt) -> t_vec
		{
			return tl2::create<t_vec>({ pt.x(), pt.y() });
		};

		auto vertex_to_vec = [](const typename t_vorotraits::vertex_type& vec) 
			-> t_vec
		{
			return tl2::create<t_vec>({ vec.x(), vec.y() });
		};

		auto to_segment_data = [&to_point_data](const t_line& line) 
			-> poly::segment_data<t_real>
		{
			auto pt1 = to_point_data(std::get<0>(line));
			auto pt2 = to_point_data(std::get<1>(line));

			return poly::segment_data<t_real>{pt1, pt2};
		};


		// parabolic edge
		if(edge.is_curved() && edge.is_finite())
		{
			const t_line* seg = get_segment(edge.cell()->contains_point());
			const t_vec* pt = get_segment_point(!edge.cell()->contains_point());
			if(!seg || !pt)
				continue;

			std::vector<poly::point_data<t_real>> parabola{{ 
				vertex_to_point_data(*vert0),
				vertex_to_point_data(*vert1)
			}};

			poly::voronoi_visual_utils<t_real>::discretize(
				to_point_data(*pt), to_segment_data(*seg),
				edge_eps, &parabola);

			if(parabola.size())
			{
				std::vector<t_vec> parabolic_edges;
				parabolic_edges.reserve(parabola.size());

				for(const auto& parabola_pt : parabola)
					parabolic_edges.emplace_back(to_vec(parabola_pt));

				all_parabolic_edges_vec.emplace_back(
					std::make_tuple(
						std::move(parabolic_edges), 
						*vert0idx, *vert1idx));
				/*all_parabolic_edges.emplace(
					std::make_pair(
						std::make_pair(*vert0idx, *vert1idx), 
						std::move(parabolic_edges)));*/
			}
		}

		// linear edge
		else
		{
			// finite edge
			if(edge.is_finite())
			{
				t_line line = std::make_pair(
					vertex_to_vec(*vert0) / scale,
					vertex_to_vec(*vert1) / scale);

				linear_edges_vec.emplace_back(
					std::make_tuple(line, vert0idx, vert1idx));
				/*linear_edges.emplace(
					std::make_pair(
						std::make_pair(vert0idx, vert1idx),
						line));*/
			}

			// infinite edge
			else
			{
				t_vec lineorg;
				bool inverted = false;
				if(vert0)
				{
					lineorg = vertex_to_vec(*vert0);
					inverted = false;
				}
				else if(vert1)
				{
					lineorg = vertex_to_vec(*vert1);
					inverted = true;
				}
				else
				{
					continue;
				}

				lineorg /= scale;

				const t_vec* vec = get_segment_point(false);
				const t_vec* twinvec = get_segment_point(true);

				if(!vec || !twinvec)
					continue;

				t_vec perpdir = *vec - *twinvec;
				if(inverted)
					perpdir = -perpdir;
				t_vec linedir = tl2::create<t_vec>({ perpdir[1], -perpdir[0] });

				linedir /= tl2::norm(linedir);
				linedir *= infline_len;

				t_line line = std::make_pair(lineorg, lineorg + linedir);
				linear_edges_vec.emplace_back(
					std::make_tuple(line, vert0idx, vert1idx));
				/*linear_edges.emplace(
					std::make_pair(
						std::make_pair(vert0idx, vert1idx), 
						line));*/
			}
		}
	}

	// remove vertices with no connection
	if(line_groups.size())
	{
		std::vector<std::string> verts;
		verts.reserve(graph.GetNumVertices());

		// get vertex identifiers
		for(std::size_t vert=0; vert<graph.GetNumVertices(); ++vert)
		{
			const std::string& id = graph.GetVertexIdent(vert);
			verts.push_back(id);
		}

		// remove vertices with no connections from graph
		std::vector<std::size_t> removed_indices;
		removed_indices.reserve(verts.size());

		for(std::size_t vertidx=0; vertidx<verts.size(); ++vertidx)
		{
			const std::string& id = verts[vertidx];
			auto neighbours_outgoing = graph.GetNeighbours(id, 1);

			if(neighbours_outgoing.size() == 0)
			{
				graph.RemoveVertex(id);
				removed_indices.push_back(vertidx);
			}
		}

		// remove the vertex coordinates
		//std::sort(removed_indices.begin(), removed_indices.end(),
		//	[](std::size_t idx1, std::size_t idx2) -> bool { return idx1 > idx2; });
		std::reverse(removed_indices.begin(), removed_indices.end());

		for(std::size_t idx : removed_indices)
		{
			if(idx < vertices.size())
			{
				vertices.erase(vertices.begin() + idx);
			}
			else
			{
				std::ostringstream ostrErr;
				ostrErr << "Vertex index out of range: " << idx << ". ";
				ostrErr << "Vector size: " << vertices.size() << ".";
				throw std::out_of_range(ostrErr.str());
				break;
			}

			// remove linear bisectors containing the removed vertex (and correct other indices)
			for(auto iter = linear_edges_vec.begin(); iter != linear_edges_vec.end();)
			{
				// remove bisector
				if((std::get<1>(*iter) && *std::get<1>(*iter)==idx) ||
					(std::get<2>(*iter) && *std::get<2>(*iter)==idx))
				{
					iter = linear_edges_vec.erase(iter);
					continue;
				}

				// correct indices
				if(std::get<1>(*iter) && *std::get<1>(*iter) >= idx)
					--*std::get<1>(*iter);
				if(std::get<2>(*iter) && *std::get<2>(*iter) >= idx)
					--*std::get<2>(*iter);

				++iter;
			}

			// remove quadratic bisectors containing the removed vertex (and correct other indices)
			for(auto iter = all_parabolic_edges_vec.begin(); iter != all_parabolic_edges_vec.end();)
			{
				// remove bisector
				if(std::get<1>(*iter)==idx || std::get<2>(*iter)==idx)
				{
					iter = all_parabolic_edges_vec.erase(iter);
					continue;
				}

				// correct indices
				if(std::get<1>(*iter) >= idx)
					--std::get<1>(*iter);
				if(std::get<2>(*iter) >= idx)
					--std::get<2>(*iter);

				++iter;
			}
		}
	}


	// convert edge vectors to edge map
	// TODO: generate them directly and remove the vector types
	for(const auto& edge : all_parabolic_edges_vec)
	{
		std::size_t idx1 = std::get<1>(edge);
		std::size_t idx2 = std::get<2>(edge);

		all_parabolic_edges.emplace(
			std::make_pair(
				std::make_pair(idx1, idx2),	// key
				std::move(std::get<0>(edge))));
	}

	for(const auto& edge : linear_edges_vec)
	{
		const auto& _idx1 = std::get<1>(edge);
		const auto& _idx2 = std::get<2>(edge);

		std::size_t idx1 = _idx1 ? *_idx1 : std::numeric_limits<std::size_t>::max();
		std::size_t idx2 = _idx2 ? *_idx2 : std::numeric_limits<std::size_t>::max();

		linear_edges.emplace(
			std::make_pair(
				std::make_pair(idx1, idx2),	// key 
				std::move(std::get<0>(edge))));
	}

	return results;
}


#ifdef USE_OVD
/**
 * voronoi diagram for line segments
 * @see https://github.com/aewallin/openvoronoi/blob/master/cpp_examples/random_line_segments/main.cpp
 * @see https://github.com/aewallin/openvoronoi/blob/master/src/utility/vd2svg.hpp
 */
template<class t_vec, 
	class t_line = std::pair<t_vec, t_vec>,
	class t_graph = AdjacencyMatrix<typename t_vec::value_type>,
	class t_int = int>
VoronoiLinesResults<t_vec, t_line, t_graph>
calc_voro_ovd(const std::vector<t_line>& lines, 
	std::vector<std::pair<std::size_t, std::size_t>>& line_groups,
	bool group_lines = true, bool remove_voronoi_vertices_in_regions = false,	// TODO
	typename t_vec::value_type edge_eps = 1e-2)
requires tl2::is_vec<t_vec> && is_graph<t_graph>
{
	using t_real = typename t_vec::value_type;

	VoronoiLinesResults<t_vec, t_line, t_graph> results;
	auto& vertices = results.vertices;;
	auto& linear_edges = results.linear_edges;
	auto& all_parabolic_edges = results.parabolic_edges;
	t_graph& graph = results.graph;

	// get minimal and maximal extents of vertices
	t_real maxRadSq = 1.;
	for(const t_line& line : lines)
	{
		t_real d0 = tl2::inner<t_vec>(std::get<0>(line), std::get<0>(line));
		t_real d1 = tl2::inner<t_vec>(std::get<1>(line), std::get<1>(line));

		maxRadSq = std::max(maxRadSq, d0);
		maxRadSq = std::max(maxRadSq, d1);
	}

	ovd::VoronoiDiagram voro(std::sqrt(maxRadSq)*1.5, lines.size()*2);
	//voro.debug_on();
	//voro.set_silent(0);

	std::vector<std::pair<int, int>> linesites;
	linesites.reserve(lines.size());

	for(const t_line& line : lines)
	{
		try
		{
			const t_vec& vec1 = std::get<0>(line);
			const t_vec& vec2 = std::get<1>(line);

			int idx1 = voro.insert_point_site(ovd::Point(vec1[0], vec1[1]));
			int idx2 = voro.insert_point_site(ovd::Point(vec2[0], vec2[1]));

			linesites.emplace_back(std::make_pair(idx1, idx2));
		}
		catch(const std::exception& ex)
		{
			std::cerr << "Error inserting voronoi point sites: " 
				<< ex.what() << std::endl;
		}
	}

	for(const auto& line : linesites)
	{
		try
		{
			voro.insert_line_site(std::get<0>(line), std::get<1>(line));
		}
		catch(const std::exception& ex)
		{
			std::cerr << "Error inserting voronoi line segment sites: " 
				<< ex.what() << std::endl;
		}
	}

	const auto& vdgraph = voro.get_graph_reference();

	// maps ovd graph vertex pointer to identifier for own graph
	std::unordered_map<ovd::HEVertex, std::size_t> vert_to_idx;

	vertices.reserve(vdgraph.vertices().size());
	for(const auto& vert : vdgraph.vertices())
	{
		if(vdgraph[vert].type != ovd::NORMAL)
			continue;

		const auto& pos = vdgraph[vert].position;
		vertices.emplace_back(tl2::create<t_vec>({ pos.x, pos.y }));
	}

	std::size_t curidx = 0;
	//linear_edges.reserve(vdgraph.edges().size());

	for(const auto& edge : vdgraph.edges())
	{
		const auto ty = vdgraph[edge].type;
		// ignore perpendicular lines separating regions
		if(ty == ovd::SEPARATOR)
			continue;

		const auto vert1 = vdgraph.source(edge);
		const auto vert2 = vdgraph.target(edge);

		const auto& pos1 = vdgraph[vert1].position;
		const auto& pos2 = vdgraph[vert2].position;

		std::size_t vert1idx = 0;
		std::size_t vert2idx = 0;

		// bisector handled?
		if(ty == ovd::LINE || ty == ovd::LINELINE || 
			ty == ovd::PARA_LINELINE || ty == ovd::PARABOLA)
		{
			// add graph vertex
			auto iter1 = vert_to_idx.find(vert1);
			if(iter1 == vert_to_idx.end())
			{
				vert1idx = curidx++;
				iter1 = vert_to_idx.insert(std::make_pair(vert1, vert1idx)).first;
				graph.AddVertex(std::to_string(vert1idx));
			}
			else
			{
				vert1idx = iter1->second;
			}

			// add graph vertex
			auto iter2 = vert_to_idx.find(vert2);
			if(iter2 == vert_to_idx.end())
			{
				vert2idx = curidx++;
				iter2 = vert_to_idx.insert(std::make_pair(vert2, vert2idx)).first;
				graph.AddVertex(std::to_string(vert2idx));
			}
			else
			{
				vert2idx = iter2->second;
			}

			// add graph edge
			if(iter1 != vert_to_idx.end() && iter2 != vert_to_idx.end())
			{
				// TODO: arc length of parabolic edges
				const auto& pos1 = vdgraph[vert1].position;
				const auto& pos2 = vdgraph[vert2].position;

				t_real len = tl2::norm(
					tl2::create<t_vec>({ pos1.x, pos1.y }) - 
					tl2::create<t_vec>({ pos2.x, pos2.y }));

				graph.AddEdge(
					std::to_string(vert1idx), 
					std::to_string(vert2idx),
					len);
			}
		}


		if(ty == ovd::LINE || ty == ovd::LINELINE || ty == ovd::PARA_LINELINE)
		{
			t_line line = std::make_pair(
				tl2::create<t_vec>({ pos1.x, pos1.y }),
				tl2::create<t_vec>({ pos2.x, pos2.y }));

			linear_edges.emplace(
				std::make_pair(
					std::make_pair(vert1idx, vert2idx), 
					std::move(line)));
		}
		else if(ty == ovd::PARABOLA)
		{
			std::vector<t_vec> para_edge;
			para_edge.reserve(std::size_t(std::ceil(1./edge_eps)));

			// TODO: check parameter range because of gaps in the bisector
			for(t_real param = 0.; param <= 1.; param += edge_eps)
			{
				t_real para_pos = 
					std::lerp(vdgraph[vert1].dist(), vdgraph[vert2].dist(), param);
				auto pt = vdgraph[edge].point(para_pos);

				if(std::isfinite(pt.x) && std::isfinite(pt.y))
					para_edge.emplace_back(
						tl2::create<t_vec>({ pt.x, pt.y }));
			}

			all_parabolic_edges.emplace(
				std::make_pair(
					std::make_pair(vert1idx, vert2idx), 
					std::move(para_edge)));
		}
	}

	return results;
}
#endif


/**
 * split a concave polygon into convex sub-polygons
 * @see algorithm: lecture notes by D. Hegazy, 2015
 */
template<class t_vec, class t_real = typename t_vec::value_type>
requires tl2::is_vec<t_vec>
std::vector<std::vector<t_vec>> convex_split(
	const std::vector<t_vec>& poly, t_real eps = 1e-6)
{
	std::vector<std::vector<t_vec>>	split{};

	// number of vertices
	const std::size_t N = poly.size();
	if(N <= 3)
		return split;

	//auto [poly, mean] = sort_vertices_by_angle<t_vec, t_real>(_poly);

	/*using namespace tl2_ops;
	std::cout << "polygon to split:" << std::endl;
	for(const t_vec& vec : poly)
		std::cout << "\t" << vec << std::endl;
	std::cout << std::endl;*/


	// find concave corner
	std::optional<std::size_t> idx_concave;
	//t_real total_angle = 0.;

	for(std::size_t idx1=0; idx1<N; ++idx1)
	{
		std::size_t idx2 = (idx1+1) % N;
		std::size_t idx3 = (idx1+2) % N;

		const t_vec& vert1 = poly[idx1];
		const t_vec& vert2 = poly[idx2];
		const t_vec& vert3 = poly[idx3];

		t_real angle = tl2::pi<t_real> - line_angle<t_vec, t_real>(
			vert1, vert2, vert2, vert3);
		//total_angle += angle;
		angle = tl2::mod_pos<t_real>(angle, t_real(2)*tl2::pi<t_real>);

		// corner angle > 180°  =>  concave corner found
		if(!idx_concave && angle > tl2::pi<t_real> + eps)
		{
			idx_concave = idx1;
			break;
		}
	}
	//std::cout << "total angle: " << total_angle/tl2::pi<t_real>*180. << std::endl;


	// get intersection of concave edge with contour
	// TODO: handle int->real conversion if needed
	//t_vec intersection;
	std::optional<std::size_t> idx_intersection;

	if(idx_concave)
	{
		std::size_t idx2 = (*idx_concave+1) % N;

		const t_vec& vert1 = poly[*idx_concave];
		const t_vec& vert2 = poly[idx2];
		t_vec dir1 = vert2 - vert1;

		circular_wrapper circularverts(const_cast<std::vector<t_vec>&>(poly));

		auto iterBeg = circularverts.begin() + (*idx_concave + 2);
		auto iterEnd = circularverts.begin() + (*idx_concave + N);

		for(auto iter = iterBeg; iter != iterEnd; ++iter)
		{
			const t_vec& vert3 = *iter;
			const t_vec& vert4 = *(iter + 1);
			t_vec dir2 = vert4 - vert3;

			// intersect infinite line from concave edge with contour line segment
			auto[pt1, pt2, valid, dist, param1, param2] =
				tl2::intersect_line_line<t_vec, t_real>(
					vert1, dir1, vert3, dir2, eps);

			if(valid && param2>=0. && param2<1. &&
				tl2::equals<t_vec>(pt1, pt2, eps))
			{
				auto iterInters = (iter+1).GetIter();
				idx_intersection = iterInters - poly.begin();
				//intersection = pt1;
				break;
			}
		}
	}


	// split polygon
	split.reserve(N);

	if(idx_concave && idx_intersection)
	{
		circular_wrapper circularverts(const_cast<std::vector<t_vec>&>(poly));

		auto iter1 = circularverts.begin() + (*idx_concave);
		auto iter2 = circularverts.begin() + (*idx_intersection);

		// split polygon along the line [idx_concave+1], intersection
		std::vector<t_vec> poly1, poly2;
		poly1.reserve(N);
		poly2.reserve(N);

		// sub-polygon 1
		//poly1.push_back(intersection);
		for(auto iter = iter2; true; ++iter)
		{
			//if(!tl2::equals<t_vec, t_real>(intersection, *iter, eps))
				poly1.push_back(*iter);

			if(iter.GetIter() == (iter1 + 1).GetIter())
				break;
		}

		// sub-polygon 2
		for(auto iter = iter1+1; true; ++iter)
		{
			//if(!tl2::equals<t_vec, t_real>(intersection, *iter, eps))
				poly2.push_back(*iter);

			if(iter.GetIter() == (iter2 /*- 1*/).GetIter())
				break;
		}
		//poly2.push_back(intersection);

		// recursively split new polygons
		if(auto subsplit1 = convex_split<t_vec, t_real>(poly1, eps);
			subsplit1.size())
		{
			for(auto&& newpoly : subsplit1)
			{
				if(newpoly.size() >= 3)
					split.emplace_back(std::move(newpoly));
			}
		}
		else
		{
			// poly1 was already convex
			split.emplace_back(std::move(poly1));
		}

		if(auto subsplit2 = convex_split<t_vec, t_real>(poly2, eps);
			subsplit2.size())
		{
			for(auto&& newpoly : subsplit2)
			{
				if(newpoly.size() >= 3)
					split.emplace_back(std::move(newpoly));
			}
		}
		else
		{
			// poly2 was already convex
			split.emplace_back(std::move(poly2));
		}
	}

	return split;
}

}
#endif