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

#include "types.h"
#include "Geometry.h"



// ----------------------------------------------------------------------------
// instrument axis
// ----------------------------------------------------------------------------
class Axis
{
public:
	Axis(const std::string& id="", const Axis* prev=nullptr);
	~Axis();

	void Clear();
	bool Load(const boost::property_tree::ptree& prop, const std::string& basePath);

	const std::string& GetId() const { return m_id; }
	const t_vec& GetZeroPos() const { return m_pos; }
	t_real GetAxisAngle() const { return m_angle; }

	t_mat GetTrafo() const;

	const std::vector<std::shared_ptr<Geometry>>& GetComps() const { return m_comps; }

private:
	// identifier
	std::string m_id;
	// previous axis
	const Axis* m_prev = nullptr;

	// coordinate origin
	t_vec m_pos = tl2::create<t_vec>({0,0});
	// angle with respect to previous axis
	t_real m_angle = 0;

	// components
	std::vector<std::shared_ptr<Geometry>> m_comps;
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

	const Axis& GetMonochromator() const { return m_mono; }
	const Axis& GetSample() const { return m_sample; }
	const Axis& GetAnalyser() const { return m_ana; }


private:
	Axis m_mono{"monochromator"};
	Axis m_sample{"sample", &m_mono};
	Axis m_ana{"analyser", &m_sample};
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

	const std::vector<std::shared_ptr<Geometry>>& GetWalls() const { return m_walls; }
	const Instrument& GetInstrument() const { return m_instr; }


private:
	t_real m_floorlen[2] = { 10., 10. };

	// wall segments
	std::vector<std::shared_ptr<Geometry>> m_walls;
	// instrument geometry
	Instrument m_instr;
};
// ----------------------------------------------------------------------------

#endif
