/**
 * calculate obstacles' voronoi edge paths
 * @author Tobias Weber <tweber@ill.fr>
 * @date jun-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GEO_PATHS_BUILDER_H__
#define __GEO_PATHS_BUILDER_H__

#include <memory>
#include <iostream>

#include <boost/signals2/signal.hpp>

#include "types.h"
#include "InstrumentSpace.h"

#include "tlibs2/libs/maths.h"
#include "src/libs/img.h"
#include "src/libs/graphs.h"
#include "src/libs/voronoi.h"
#include "src/libs/voronoi_lines.h"
#include "src/libs/hull.h"


struct InstrumentPath
{
	bool ok = false;

	std::vector<std::size_t> voronoi_indices;
};


class PathsBuilder
{
public:
	// contour point
	using t_contourvec = tl2::vec<int, std::vector>;

	// line segment
	using t_line = std::pair<t_vec, t_vec>;

	// voronoi edges
	using t_voronoiedge_linear =
		std::tuple<t_line,
		std::optional<std::size_t>,
		std::optional<std::size_t>>;
	using t_voronoiedge_parabolic =
		std::tuple<std::vector<t_vec>,
		std::size_t, std::size_t>;

	// voronoi graph
	using t_graph = geo::AdjacencyList<t_real>;


public:
	PathsBuilder();
	~PathsBuilder();

	t_vec PixelToAngle(t_real x, t_real y, bool deg = true, bool inc_sense = false) const;
	t_vec AngleToPixel(t_real x, t_real y, bool deg = true, bool inc_sense = false) const;

	// input instrument
	void SetInstrumentSpace(const InstrumentSpace* instr) { m_instrspace = instr; }
	const InstrumentSpace* GetInstrumentSpace() const { return m_instrspace; }

	void SetScatteringSenses(const t_real *senses) { m_sensesCCW = senses; }

	// calculation workflow
	bool CalculateConfigSpace(t_real da2, t_real da4,
		t_real starta2 = 0., t_real enda2 = tl2::pi<t_real>,
		t_real starta4 = 0., t_real enda4 = tl2::pi<t_real>);
	bool CalculateWallContours(bool simplify = true, bool convex_split = false);
	bool CalculateLineSegments();
	bool CalculateVoronoi(bool group_lines=true);

	// clear all data
	void Clear();

	// get contour image and wall contour points
	const geo::Image<std::uint8_t>& GetImage() const { return m_img; }
	const std::vector<std::vector<t_contourvec>>& GetWallContours(bool full=false) const;

	// get voronoi vertices, edges and graph
	const geo::VoronoiLinesResults<t_vec, t_line, t_graph>& GetVoronoiResults() const
	{ return m_voro_results; }

	// save contour line segments to lines test tools
	bool SaveToLinesTool(std::ostream& ostr);

	// connection to progress signal
	template<class t_slot>
	boost::signals2::connection AddProgressSlot(const t_slot& slot)
	{ return m_sigProgress->connect(slot); }

	t_real GetEpsilon() const { return m_eps; }
	void SetEpsilon(t_real eps) { m_eps = eps; }

	t_real GetAngularEpsilon() const { return m_eps_angular; }
	void SetAngularEpsilon(t_real eps) { m_eps_angular = eps; }

	t_real GetVoronoiEdgeEpsilon() const { return m_voroedge_eps; }
	void SetVoronoiEdgeEpsilon(t_real eps) { m_voroedge_eps = eps; }

	unsigned int GetMaxNumThreads() const { return m_maxnum_threads; }
	void SetMaxNumThreads(unsigned int n) { m_maxnum_threads = n; }

	// find a path from an initial (a2, a4) to a final (a2, a4)
	InstrumentPath FindPath(t_real a2_i, t_real a4_i, t_real a2_f, t_real a4_f);

private:
	const InstrumentSpace *m_instrspace{};
	const t_real *m_sensesCCW{nullptr};

	// and-combine return values for calculation progress signal
	struct combine_sigret
	{
		using result_type = bool;

		template<class t_iter>
		bool operator()(t_iter begin, t_iter end) const
		{
			result_type ret = true;

			for(t_iter iter = begin; iter != end; iter = std::next(iter))
				ret = ret && *iter;

			return ret;
		}
	};

	// calculation progress signal
	using t_sig_progress =
		boost::signals2::signal<
			bool(bool start, bool end, t_real progress, const std::string& message),
			combine_sigret>;
	std::shared_ptr<t_sig_progress> m_sigProgress{};

	// angular ranges
	t_real m_monoScatteringRange[2]{0, tl2::pi<t_real>};
	t_real m_sampleScatteringRange[2]{0, tl2::pi<t_real>};

	// wall contours in configuration space
	geo::Image<std::uint8_t> m_img{};
	std::vector<std::vector<t_contourvec>> m_wallcontours = {}, m_fullwallcontours = {};

	// line segments and groups from the wall contours
	std::vector<t_line> m_lines{};
	std::vector<std::pair<std::size_t, std::size_t>> m_linegroups{};

	// arbitrary points outside and inside the regions
	std::vector<t_vec> m_points_outside_regions{};
	std::vector<bool> m_inverted_regions{};

	// voronoi vertices, edges and graph from the line segments
	geo::VoronoiLinesResults<t_vec, t_line, t_graph> m_voro_results{};

	// general, angular and voronoi edge calculation epsilon
	t_real m_eps = 1e-3;
	t_real m_eps_angular = 1e-3;
	t_real m_voroedge_eps = 1e-2;

	unsigned int m_maxnum_threads = 4;
};

#endif
