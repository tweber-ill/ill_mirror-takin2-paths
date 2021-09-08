/**
 * tas instrument
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

#include <unordered_map>
#include <optional>

#include "Instrument.h"
#include "src/libs/lines.h"
#include "src/libs/ptree.h"
#include "tlibs2/libs/file.h"
#include "tlibs2/libs/str.h"

namespace pt = boost::property_tree;


Instrument::Instrument()
	: m_sigUpdate{std::make_shared<t_sig_update>()}
{
}


Instrument::~Instrument()
{
	Clear();
}


Instrument::Instrument(const Instrument& instr)
{
	*this = operator=(instr);
}


const Instrument& Instrument::operator=(const Instrument& instr)
{
	this->m_mono = instr.m_mono;
	this->m_sample = instr.m_sample;
	this->m_ana = instr.m_ana;

	this->m_mono.SetParentInstrument(this);
	this->m_sample.SetParentInstrument(this);
	this->m_ana.SetParentInstrument(this);

	this->m_mono.SetPreviousAxis(nullptr);
	this->m_sample.SetPreviousAxis(&this->m_mono);
	this->m_ana.SetPreviousAxis(&this->m_sample);

	this->m_mono.SetNextAxis(&this->m_sample);
	this->m_sample.SetNextAxis(&this->m_ana);
	this->m_ana.SetNextAxis(nullptr);

	this->m_drag_pos_axis_start =  instr.m_drag_pos_axis_start;
	this->m_sigUpdate = std::make_shared<t_sig_update>();

	return *this;
}


void Instrument::Clear()
{
	m_mono.Clear();
	m_sample.Clear();
	m_ana.Clear();

	m_sigUpdate = std::make_shared<t_sig_update>();
}


bool Instrument::Load(const pt::ptree& prop)
{
	bool mono_ok = false;
	bool sample_ok = false;
	bool ana_ok = false;

	if(auto mono = prop.get_child_optional("monochromator"); mono)
		mono_ok = m_mono.Load(*mono);
	if(auto sample = prop.get_child_optional("sample"); sample)
		sample_ok = m_sample.Load(*sample);
	if(auto ana = prop.get_child_optional("analyser"); ana)
		ana_ok = m_ana.Load(*ana);

	return mono_ok && sample_ok && ana_ok;
}


pt::ptree Instrument::Save() const
{
	pt::ptree prop;

	prop.put_child("monochromator", m_mono.Save());
	prop.put_child("sample", m_sample.Save());
	prop.put_child("analyser", m_ana.Save());

	return prop;
}


/**
 * an instrument component is requested to be dragged from the gui
 */
void Instrument::DragObject(bool drag_start, const std::string& obj,
	t_real x_start, t_real y_start, t_real x, t_real y)
{
	Axis* ax = nullptr;
	Axis* ax_prev = nullptr;
	bool set_xtal_angle = false;
	bool use_out_axis = false;

	// move sample position around monochromator axis
	if(m_sample.IsObjectOnAxis(obj, AxisAngle::INTERNAL)
		/*|| m_sample.IsObjectOnAxis(obj, AxisAngle::OUTGOING)
		|| m_sample.IsObjectOnAxis(obj, AxisAngle::INCOMING)*/)
	{
		ax = &m_sample;
		ax_prev = &m_mono;
		set_xtal_angle = true;
	}

	// move analyser position around sample axis
	else if(m_ana.IsObjectOnAxis(obj, AxisAngle::INTERNAL)
		/*|| m_ana.IsObjectOnAxis(obj, AxisAngle::INCOMING)*/)
	{
		ax = &m_ana;
		ax_prev = &m_sample;
	}

	// move detector around analyser axis
	else if(m_ana.IsObjectOnAxis(obj, AxisAngle::OUTGOING))
	{
		ax = &m_ana;
		ax_prev = &m_ana;
		use_out_axis = true;
		set_xtal_angle = true;
	}

	if(!ax || !ax_prev)
		return;

	t_vec pos_startcur = tl2::create<t_vec>({ x_start, y_start });
	t_vec pos_cur = tl2::create<t_vec>({ x, y });

	t_vec pos_ax;
	if(!use_out_axis)
	{
		// get center of axis
		pos_ax = ax->GetTrafo(AxisAngle::INCOMING) * tl2::create<t_vec>({ 0, 0, 0, 1 });
	}
	else
	{
		// get a position on the outgoing vector of an axis
		// TODO: replace the "2 0 0" with the actual centre of the "detector" object
		pos_ax = ax->GetTrafo(AxisAngle::OUTGOING) * tl2::create<t_vec>({ 2, 0, 0, 1 });
	}

	t_vec pos_ax_prev = ax_prev->GetTrafo(AxisAngle::INCOMING) * tl2::create<t_vec>({ 0, 0, 0, 1 });
	t_vec pos_ax_prev_in = ax_prev->GetTrafo(AxisAngle::INCOMING) * tl2::create<t_vec>({ -1, 0, 0, 1 });
	pos_ax.resize(2);
	pos_ax_prev.resize(2);
	pos_ax_prev_in.resize(2);

	if(drag_start)
		m_drag_pos_axis_start = pos_ax;
	t_vec pos_drag = pos_cur - pos_startcur + m_drag_pos_axis_start;

	t_real new_angle = tl2::angle<t_vec>(pos_ax_prev - pos_ax_prev_in, pos_drag - pos_ax_prev);
	new_angle = tl2::mod_pos(new_angle, t_real(2)*tl2::pi<t_real>);
	if(new_angle > tl2::pi<t_real>)
		new_angle -= t_real(2)*tl2::pi<t_real>;

	// set scattering and crystal angle
	if(!use_out_axis)
	{
		ax_prev->SetAxisAngleOut(new_angle);
		if(set_xtal_angle)
			ax_prev->SetAxisAngleInternal(new_angle * t_real(0.5));
	}
	else
	{
		ax->SetAxisAngleOut(new_angle);
		if(set_xtal_angle)
			ax->SetAxisAngleInternal(new_angle * t_real(0.5));
	}
}
