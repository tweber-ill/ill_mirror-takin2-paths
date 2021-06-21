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
#include "Instrument.h"
#include "src/libs/img.h"
#include "src/libs/graphs.h"



class PathsBuilder
{
public:
	using t_contourvec = tl2::vec<int, std::vector>;
	using t_graph = geo::AdjacencyList<t_real>;
	using t_line = std::pair<t_vec, t_vec>;


public:
	PathsBuilder();
	~PathsBuilder();

	// input instrument
	void SetInstrumentSpace(const InstrumentSpace* instr) { m_instrspace = instr; }
	void SetScatteringSenses(const t_real *senses) { m_sensesCCW = senses; }

	// calculation workflow
	void CalculateConfigSpace(t_real da2, t_real da4);
	void CalculateWallContours();
	void SimplifyWallContours();
	void CalculateLineSegments();
	void CalculateVoronoi();
	void SimplifyVoronoi();

	// outputs
	const geo::Image<std::uint8_t>& GetImage() const { return m_img; }
	const std::vector<std::vector<t_contourvec>>& GetWallContours() const 
	{ return m_wallcontours; }
	bool SaveToLinesTool(std::ostream& ostr);

	// connection to progress signal
	template<class t_slot>
	boost::signals2::connection AddProgressSlot(const t_slot& slot)
		{ return m_sigProgress->connect(slot); }

private:
	const InstrumentSpace *m_instrspace{};
	const t_real* m_sensesCCW{nullptr};

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
			bool(bool start, bool end, t_real progress),
			combine_sigret>;
	std::shared_ptr<t_sig_progress> m_sigProgress{};

	geo::Image<std::uint8_t> m_img{};
	std::vector<std::vector<t_contourvec>> m_wallcontours{};
	std::vector<t_line> m_lines{};
	std::vector<std::pair<std::size_t, std::size_t>> m_linegroups{};
};

#endif
