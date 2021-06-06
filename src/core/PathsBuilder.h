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


class PathsBuilder
{
public:
	using t_contourvec = tl2::vec<int, std::vector>;


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
	void CalculateVoronoi();
	void SimplifyVoronoi();

	// outputs
	const geo::Image<std::uint8_t>& GetImage() const { return m_img; }
	const std::vector<std::vector<t_contourvec>>& GetWallContours() const 
	{ return m_wallcontours; }
	void SaveToLinesTool(std::ostream& ostr);

	// connection to progress signal
	template<class t_slot>
	void AddProgressSlot(const t_slot& slot)
		{ m_sigProgress->connect(slot); }


private:
	const InstrumentSpace *m_instrspace{};
	const t_real* m_sensesCCW{nullptr};

	// calculation progress signal
	using t_sig_progress = boost::signals2::signal<bool(bool start, bool end, t_real progress)>;
	std::shared_ptr<t_sig_progress> m_sigProgress;

	geo::Image<std::uint8_t> m_img;
	std::vector<std::vector<t_contourvec>> m_wallcontours;
};

#endif
