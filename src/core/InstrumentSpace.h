/**
 * instrument space and walls
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __INSTR_SPACE_H__
#define __INSTR_SPACE_H__

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/signals2/signal.hpp>

#include "types.h"
#include "Geometry.h"
#include "Instrument.h"


#define FILE_BASENAME "taspaths."
#define PROG_IDENT "takin_taspaths"



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

	void AddWall(const std::vector<std::shared_ptr<Geometry>>& wallsegs, const std::string& id);

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
	static std::pair<bool, std::string> load(
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
