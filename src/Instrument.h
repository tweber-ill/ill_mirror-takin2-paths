/**
 * instrument walls
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __INSTR_H__
#define __INSTR_H__

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "globals.h"


// ----------------------------------------------------------------------------
// wall segment
// ----------------------------------------------------------------------------
struct Wall
{
	std::string id;
	t_vec pos1, pos2;
	t_real height, depth, length;
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// instrument axis
// ----------------------------------------------------------------------------
class Axis
{
public:
	Axis();
	~Axis();

	void Clear();
	bool Load(const boost::property_tree::ptree& prop, const std::string& basePath);

private:
	t_vec m_pos;	// coordinate origin
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// instrument
// ----------------------------------------------------------------------------
class Instrument
{
public:
	Instrument();
	~Instrument();

	void Clear();
	bool Load(const boost::property_tree::ptree& prop, const std::string& basePath);

private:
	Axis m_mono, m_sample, m_ana;
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// instrument space
// ----------------------------------------------------------------------------
class InstrumentSpace
{
public:
	InstrumentSpace();
	~InstrumentSpace();

	void Clear();
	bool Load(const boost::property_tree::ptree& prop, const std::string& basePath);


	t_real GetFloorLenX() const { return m_floorlen[0]; }
	t_real GetFloorLenY() const { return m_floorlen[1]; }

	const std::vector<Wall>& GetWalls() const { return m_walls; }


private:
	t_real m_floorlen[2] = { 10., 10. };

	std::vector<Wall> m_walls;

	Instrument m_instr;
};
// ----------------------------------------------------------------------------

#endif
