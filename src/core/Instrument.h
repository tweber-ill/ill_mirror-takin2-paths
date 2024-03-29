/**
 * instrument
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
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

#ifndef __INSTR_H__
#define __INSTR_H__

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/signals2/signal.hpp>

#include "types.h"
#include "Axis.h"



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

	// send an update signal
	void EmitUpdate();
	void SetBlockUpdates(bool b);
	bool GetBlockUpdates() const;

	std::vector<ObjectProperty> GetProperties(const std::string& obj) const;
	std::tuple<bool, std::shared_ptr<Geometry>> SetProperties(
		const std::string& obj, const std::vector<ObjectProperty>& props);


private:
	Axis m_mono{"monochromator", nullptr, &m_sample, this};
	Axis m_sample{"sample", &m_mono, &m_ana, this};
	Axis m_ana{"analyser", &m_sample, nullptr, this};

	// block the update signal
	bool m_block_updates = false;

	// TODO: allow instrument editing
	bool m_allow_editing = false;

	// starting position for drag operation
	t_vec m_drag_pos_axis_start{};

	// update signal
	using t_sig_update = boost::signals2::signal<void(const Instrument&)>;
	std::shared_ptr<t_sig_update> m_sigUpdate{};
};
// ----------------------------------------------------------------------------


#endif
