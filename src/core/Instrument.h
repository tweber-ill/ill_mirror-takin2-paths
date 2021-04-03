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
enum class AxisAngle
{
	IN,
	INTERNAL,
	OUT
};

class Axis
{
public:
	Axis(const std::string& id="", const Axis* prev=nullptr);
	~Axis();

	void Clear();
	bool Load(const boost::property_tree::ptree& prop);

	const std::string& GetId() const { return m_id; }
	const t_vec& GetZeroPos() const { return m_pos; }

	t_real GetAxisAngleIn() const { return m_angle_in; }
	t_real GetAxisAngleOut() const { return m_angle_out; }

	void SetAxisAngleIn(t_real angle) { m_angle_in = angle; }
	void SetAxisAngleOut(t_real angle) { m_angle_out = angle; }

	// which==1: in, which==2: internal, which==3: out
	t_mat GetTrafo(AxisAngle which=AxisAngle::IN) const;
	const std::vector<std::shared_ptr<Geometry>>& GetComps(AxisAngle which=AxisAngle::IN) const;

private:
	// identifier
	std::string m_id;
	// previous axis
	const Axis* m_prev = nullptr;

	// coordinate origin
	t_vec m_pos = tl2::create<t_vec>({0,0});

	// angle of incoming axis and outgoing axis
	t_real m_angle_in = 0, m_angle_out = 0;
	// internal rotation angle
	t_real m_angle_internal;

	// components relative to incoming and outgoing axis
	std::vector<std::shared_ptr<Geometry>> m_comps_in, m_comps_out;
	// components rotated internally
	std::vector<std::shared_ptr<Geometry>> m_comps_internal;
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
	bool Load(const boost::property_tree::ptree& prop);

	const Axis& GetMonochromator() const { return m_mono; }
	const Axis& GetSample() const { return m_sample; }
	const Axis& GetAnalyser() const { return m_ana; }

	Axis& GetMonochromator() { return m_mono; }
	Axis& GetSample() { return m_sample; }
	Axis& GetAnalyser() { return m_ana; }


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
	bool Load(const boost::property_tree::ptree& prop);


	t_real GetFloorLenX() const { return m_floorlen[0]; }
	t_real GetFloorLenY() const { return m_floorlen[1]; }

	const std::vector<std::shared_ptr<Geometry>>& GetWalls() const { return m_walls; }
	const Instrument& GetInstrument() const { return m_instr; }
	Instrument& GetInstrument() { return m_instr; }


private:
	t_real m_floorlen[2] = { 10., 10. };

	// wall segments
	std::vector<std::shared_ptr<Geometry>> m_walls;
	// instrument geometry
	Instrument m_instr;
};
// ----------------------------------------------------------------------------

#endif
