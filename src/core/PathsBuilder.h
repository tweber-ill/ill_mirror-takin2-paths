/**
 * calculate obstacles' voronoi edge paths
 * @author Tobias Weber <tweber@ill.fr>
 * @date jun-2021
 * @license GPLv3, see 'LICENSE' file
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

#ifndef __GEO_PATHS_BUILDER_H__
#define __GEO_PATHS_BUILDER_H__

#include <memory>
#include <iostream>

#include <boost/signals2/signal.hpp>

#include "src/libs/voronoi_lines.h"
#include "src/libs/voronoi.h"
#include "src/libs/img.h"
#include "src/libs/graphs.h"
#include "src/libs/hull.h"
#include "tlibs2/libs/maths.h"

#include "types.h"
#include "InstrumentSpace.h"
#include "TasCalculator.h"
#include "PathsExporter.h"


struct InstrumentPath
{
	// path mesh ok?
	bool ok = false;

	// initial and final vertices on path
	t_vec2 vec_i, vec_f;

	// indices of the voronoi vertices on the path mesh
	std::vector<std::size_t> voronoi_indices;

	// position parameter along the entry and exit path
	t_real param_begin = 0;
	t_real param_end = 1;
};


/**
 * strategy for finding the path
 */
enum class PathStrategy
{
	// find shortest path
	SHORTEST,

	// avoid paths close to walls
	PENALISE_WALLS,
};


class PathsBuilder
{
public:
	// contour point
	using t_contourvec = t_vec2_int;

	// line segment
	using t_line = std::pair<t_vec2, t_vec2>;

	// voronoi edges
	using t_voronoiedge_linear =
		std::tuple<t_line,
		std::optional<std::size_t>,
		std::optional<std::size_t>>;
	using t_voronoiedge_parabolic =
		std::tuple<std::vector<t_vec2>,
		std::size_t, std::size_t>;

	// voronoi graph
	using t_graph = geo::AdjacencyList<t_real>;
	//using t_graph = geo::AdjacencyMatrix<t_real>;


public:
	PathsBuilder();
	~PathsBuilder();

	PathsBuilder(const PathsBuilder&) = default;
	PathsBuilder& operator=(const PathsBuilder&) = default;

	// ------------------------------------------------------------------------
	// input instrument
	// ------------------------------------------------------------------------
	void SetInstrumentSpace(const InstrumentSpace* instr) { m_instrspace = instr; }
	const InstrumentSpace* GetInstrumentSpace() const { return m_instrspace; }

	void SetTasCalculator(const TasCalculator* tascalc) { m_tascalc = tascalc; }
	const TasCalculator* GetTasCalculator() const { return m_tascalc; }
	// ------------------------------------------------------------------------

	// ------------------------------------------------------------------------
	// conversion functions
	// ------------------------------------------------------------------------
	t_vec2 PixelToAngle(const t_vec2& pix, bool deg = true, bool inc_sense = false) const;
	t_vec2 AngleToPixel(const t_vec2& angle, bool deg = true, bool inc_sense = false) const;
	t_vec2 PixelToAngle(t_real x, t_real y, bool deg = true, bool inc_sense = false) const;
	t_vec2 AngleToPixel(t_real x, t_real y, bool deg = true, bool inc_sense = false) const;
	// ------------------------------------------------------------------------

	// clear all data
	void Clear();

	// get contour image and wall contour points
	const geo::Image<std::uint8_t>& GetImage() const { return m_img; }
	const std::vector<std::vector<t_contourvec>>& GetWallContours(bool full=false) const;

	// get voronoi vertices, edges and graph
	const geo::VoronoiLinesResults<t_vec2, t_line, t_graph>& GetVoronoiResults() const
	{ return m_voro_results; }

	// ------------------------------------------------------------------------
	// path mesh calculation workflow
	// ------------------------------------------------------------------------
	bool CalculateConfigSpace(t_real da2, t_real da4,
		t_real starta2 = 0., t_real enda2 = tl2::pi<t_real>,
		t_real starta4 = 0., t_real enda4 = tl2::pi<t_real>);
	bool CalculateWallsIndexTree();
	bool CalculateWallContours(bool simplify = true, bool convex_split = false);
	bool CalculateLineSegments();
	bool CalculateVoronoi(bool group_lines=true);

	// number of line segment groups -- for scripting interface
	std::size_t GetNumberOfLineSegmentRegions() const { return m_linegroups.size(); }

	// test if the region is inverted -- for scripting interface
	bool IsRegionInverted(std::size_t groupidx) { return m_inverted_regions[groupidx]; }

	// get line segment group -- for scripting interface
	std::vector<std::array<t_real, 4>> GetLineSegmentRegionAsArray(std::size_t groupidx) const;
	// ------------------------------------------------------------------------

	// ------------------------------------------------------------------------
	// path calculation
	// ------------------------------------------------------------------------
	// find a path from an initial (a2, a4) to a final (a2, a4)
	InstrumentPath FindPath(t_real a2_i, t_real a4_i, t_real a2_f, t_real a4_f,
		PathStrategy pathstrategy = PathStrategy::SHORTEST);

	// get individual vertices on an instrument path
	std::vector<t_vec2> GetPathVertices(const InstrumentPath& path,
		bool subdivide_lines = false, bool deg = false) const;

	// get individual vertices on an instrument path -- for scripting interface
	std::vector<std::pair<t_real, t_real>>
		GetPathVerticesAsPairs(const InstrumentPath& path,
			bool subdivide_lines = false, bool deg = false) const;
	// ------------------------------------------------------------------------

	// ------------------------------------------------------------------------
	// options
	// ------------------------------------------------------------------------
	t_real GetEpsilon() const { return m_eps; }
	void SetEpsilon(t_real eps) { m_eps = eps; }

	t_real GetAngularEpsilon() const { return m_eps_angular; }
	void SetAngularEpsilon(t_real eps) { m_eps_angular = eps; }

	t_real GetVoronoiEdgeEpsilon() const { return m_voroedge_eps; }
	void SetVoronoiEdgeEpsilon(t_real eps) { m_voroedge_eps = eps; }

	t_real GetSubdivisionLength() const { return m_subdiv_len; }
	void SetSubdivisionLength(t_real len) { m_subdiv_len = len; }

	unsigned int GetMaxNumThreads() const { return m_maxnum_threads; }
	void SetMaxNumThreads(unsigned int n) { m_maxnum_threads = n; }
	// ------------------------------------------------------------------------

	// ------------------------------------------------------------------------
	// progress status handler
	// ------------------------------------------------------------------------
	// connection to progress signal
	template<class t_slot>
	boost::signals2::connection AddProgressSlot(const t_slot& slot)
	{ return m_sigProgress->connect(slot); }

	// show progress messages on the console
	void AddConsoleProgressHandler();
	// ------------------------------------------------------------------------

	// ------------------------------------------------------------------------
	// exporting of data
	// ------------------------------------------------------------------------
	// save contour line segments to lines test tools
	bool SaveToLinesTool(std::ostream& ostr);
	bool SaveToLinesTool(const std::string& filename);

	// export the path to various formats using a visitor
	bool AcceptExporter(const PathsExporterBase *exporter,
		const std::vector<t_vec2>& path, bool path_in_rad = false)
	{ return exporter->Export(this, path, path_in_rad); }
	// ------------------------------------------------------------------------


private:
	const InstrumentSpace *m_instrspace{};
	const TasCalculator *m_tascalc{};

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

	// index tree for wall positions
	geo::ClosestPixelTreeResults<t_contourvec> m_wallsindextree{};

	// wall contours in configuration space
	geo::Image<std::uint8_t> m_img{};
	std::vector<std::vector<t_contourvec>> m_wallcontours = {};
	std::vector<std::vector<t_contourvec>> m_fullwallcontours = {};

	// line segments (in pixel coordinates) and groups from the wall contours
	std::vector<t_line> m_lines{};
	std::vector<std::pair<std::size_t, std::size_t>> m_linegroups{};

	// arbitrary points outside the regions
	std::vector<t_vec2> m_points_outside_regions{};
	std::vector<bool> m_inverted_regions{};

	// voronoi vertices, edges and graph from the line segments
	geo::VoronoiLinesResults<t_vec2, t_line, t_graph> m_voro_results{};

	// general, angular and voronoi edge calculation epsilon
	t_real m_eps = 1e-3;
	t_real m_eps_angular = 1e-3;
	t_real m_voroedge_eps = 1e-2;

	// minimum distance to consider "staircase artefacts"
	t_real m_simplify_mindist = 3.;

	// line segment length for subdivisions
	t_real m_subdiv_len = 0.1;

	unsigned int m_maxnum_threads = 4;
};

#endif
