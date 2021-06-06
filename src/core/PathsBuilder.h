/**
 * calculate obstacles' voronoi edge paths
 * @author Tobias Weber <tweber@ill.fr>
 * @date jun-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GEO_PATHS_BUILDER_H__
#define __GEO_PATHS_BUILDER_H__

#include <iostream>

#include "types.h"
#include "Instrument.h"
#include "src/libs/img.h"


class PathsBuilder
{
public:
	PathsBuilder();
	~PathsBuilder();

	// input instrument
	void SetInstrumentSpace(const InstrumentSpace* instr) { m_instrspace = instr; }
	void SetScatteringSenses(const t_real *senses) { m_sensesCCW = senses; }

	// calculation workflow
	void CalculateConfigSpace();
	void CalculateWallContours();
	void SimplifyWallContours();
	void CalculateVoronoi();
	void SimplifyVoronoi();

	// output results
	void SaveToLinesTool(std::ostream& ostr);

private:
	const InstrumentSpace *m_instrspace{};
	const t_real* m_sensesCCW{nullptr};

	geo::Image<std::uint8_t> m_img;
};

#endif
