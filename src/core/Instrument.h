/**
 * instrument space and walls
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __INSTR_H__
#define __INSTR_H__

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/signals2/signal.hpp>

#include "types.h"
#include "Geometry.h"


#define FILE_BASENAME "taspaths."
#define PROG_IDENT "takin_taspaths"



// ----------------------------------------------------------------------------
// instrument axis
// ----------------------------------------------------------------------------
enum class AxisAngle
{
	INCOMING,	// defined with respect to incoming axis
	INTERNAL,	// defined with respect to local rotation
	OUTGOING	// defined with respect to outgoing axis
};

class Instrument;

class Axis
{
public:
	// constructor and destructor
	Axis(const std::string &id="", const Axis *prev=nullptr, const Axis *next=nullptr, Instrument *instr=nullptr);
	~Axis();

	// copy constructor and operator
	Axis(const Axis& axis);
	const Axis& operator=(const Axis& axis);

	void SetPreviousAxis(const Axis* axis) { m_prev = axis; m_trafos_need_update = true; }
	void SetNextAxis(const Axis* axis) { m_next = axis; m_trafos_need_update = true; }
	void SetParentInstrument(Instrument* instr) { m_instr = instr; m_trafos_need_update = true; }

	void Clear();
	bool Load(const boost::property_tree::ptree& prop);
	boost::property_tree::ptree Save() const;

	const std::string& GetId() const { return m_id; }
	const t_vec& GetZeroPos() const { return m_pos; }

	t_real GetAxisAngleIn() const { return m_angle_in; }
	t_real GetAxisAngleOut() const { return m_angle_out; }
	t_real GetAxisAngleInternal() const { return m_angle_internal; }

	void SetAxisAngleIn(t_real angle);
	void SetAxisAngleOut(t_real angle);
	void SetAxisAngleInternal(t_real angle);

	// which==1: in, which==2: internal, which==3: out
	const t_mat& GetTrafo(AxisAngle which=AxisAngle::INCOMING) const;
	void UpdateTrafos() const;
	void TrafosNeedUpdate() const;

	const std::vector<std::shared_ptr<Geometry>>& 
		GetComps(AxisAngle which=AxisAngle::INCOMING) const;

	bool IsObjectOnAxis(const std::string& obj, AxisAngle ax) const;

private:
	// identifier
	std::string m_id;
	// previous and next axis
	const Axis *m_prev{nullptr}, *m_next{nullptr};;
	// parent instrument
	Instrument *m_instr{nullptr};

	// trafo matrices
	mutable t_mat m_trafoIncoming = tl2::unit<t_mat>(4);
	mutable t_mat m_trafoInternal = tl2::unit<t_mat>(4);
	mutable t_mat m_trafoOutgoing = tl2::unit<t_mat>(4);
	mutable bool m_trafos_need_update = true;

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
	// constructor and destructor
	Instrument();
	~Instrument();

	// copy constructor and operator
	Instrument(const Instrument& instr);
	const Instrument& operator=(const Instrument& instr);

	void Clear();
	bool Load(const boost::property_tree::ptree& prop);
	boost::property_tree::ptree Save() const;

	const Axis& GetMonochromator() const { return m_mono; }
	const Axis& GetSample() const { return m_sample; }
	const Axis& GetAnalyser() const { return m_ana; }

	Axis& GetMonochromator() { return m_mono; }
	Axis& GetSample() { return m_sample; }
	Axis& GetAnalyser() { return m_ana; }

	void DragObject(bool drag_start, const std::string& obj, 
		t_real x_start, t_real y_start, t_real x, t_real y);

	// connection to update signal
	template<class t_slot>
	void AddUpdateSlot(const t_slot& slot)
		{ m_sigUpdate->connect(slot); }

	void EmitUpdate() { (*m_sigUpdate)(*this); }

private:
	Axis m_mono{"monochromator", nullptr, &m_sample, this};
	Axis m_sample{"sample", &m_mono, &m_ana, this};
	Axis m_ana{"analyser", &m_sample, nullptr, this};

	// starting position for drag operation
	t_vec m_drag_pos_axis_start;

	// update signal
	using t_sig_update = boost::signals2::signal<void(const Instrument&)>;
	std::shared_ptr<t_sig_update> m_sigUpdate;
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// instrument space
// ----------------------------------------------------------------------------
class InstrumentSpace
{
public:
	// constructor and destructor
	InstrumentSpace();
	~InstrumentSpace();

	// copy constructor and operator
	InstrumentSpace(const InstrumentSpace& instr);
	const InstrumentSpace& operator=(const InstrumentSpace& instr);

	void Clear();
	bool Load(const boost::property_tree::ptree& prop);
	boost::property_tree::ptree Save() const;

	t_real GetFloorLenX() const { return m_floorlen[0]; }
	t_real GetFloorLenY() const { return m_floorlen[1]; }

	const std::vector<std::shared_ptr<Geometry>>& GetWalls() const { return m_walls; }
	const Instrument& GetInstrument() const { return m_instr; }
	Instrument& GetInstrument() { return m_instr; }

	bool CheckCollision2D() const;
	void DragObject(bool drag_start, const std::string& obj, 
		t_real x_start, t_real y_start, t_real x, t_real y);

	// connection to update signal
	template<class t_slot>
	void AddUpdateSlot(const t_slot& slot)
		{ m_sigUpdate->connect(slot); }

	void EmitUpdate() { (*m_sigUpdate)(*this); }

public:
	static std::tuple<bool, std::string> load(
		const std::string& filename, InstrumentSpace& instrspace);

private:
	t_real m_floorlen[2] = { 10., 10. };

	// wall segments
	std::vector<std::shared_ptr<Geometry>> m_walls;
	// instrument geometry
	Instrument m_instr;

	// starting position for drag operation
	t_vec m_drag_pos_axis_start;

	// update signal
	using t_sig_update = boost::signals2::signal<void(const InstrumentSpace&)>;
	std::shared_ptr<t_sig_update> m_sigUpdate;
};
// ----------------------------------------------------------------------------
#endif
