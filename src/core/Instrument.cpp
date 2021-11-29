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


/**
 * assign data from another instrument
 */
Instrument::Instrument(const Instrument& instr)
{
	*this = operator=(instr);
}


/**
 * assign data from another instrument
 */
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


/**
 * clear all data in the instrument
 */
void Instrument::Clear()
{
	GetMonochromator().Clear();
	GetSample().Clear();
	GetAnalyser().Clear();

	m_sigUpdate = std::make_shared<t_sig_update>();
}


/**
 * load an instrument from a property tree
 */
bool Instrument::Load(const pt::ptree& prop)
{
	bool mono_ok = false;
	bool sample_ok = false;
	bool ana_ok = false;

	const std::string& mono_id = GetMonochromator().GetId();
	const std::string& sample_id = GetSample().GetId();
	const std::string& ana_id = GetAnalyser().GetId();

	if(auto mono = prop.get_child_optional(mono_id); mono)
		mono_ok = GetMonochromator().Load(*mono);
	if(auto sample = prop.get_child_optional(sample_id); sample)
		sample_ok = GetSample().Load(*sample);
	if(auto ana = prop.get_child_optional(ana_id); ana)
		ana_ok = GetAnalyser().Load(*ana);

	return mono_ok && sample_ok && ana_ok;
}


/**
 * save an instrument to a property tree
 */
pt::ptree Instrument::Save() const
{
	pt::ptree prop;

	const std::string& mono_id = GetMonochromator().GetId();
	const std::string& sample_id = GetSample().GetId();
	const std::string& ana_id = GetAnalyser().GetId();

	prop.put_child(mono_id, GetMonochromator().Save());
	prop.put_child(sample_id, GetSample().Save());
	prop.put_child(ana_id, GetAnalyser().Save());

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
	if(GetSample().IsObjectOnAxis(obj, AxisAngle::INTERNAL)
		/*|| GetSample().IsObjectOnAxis(obj, AxisAngle::OUTGOING)
		 | | GetSample()*.IsObjectOnAxis(obj, AxisAngle::INCOMING)*/)
	{
		ax = &GetSample();
		ax_prev = &GetMonochromator();
		set_xtal_angle = true;
	}

	// move analyser position around sample axis
	else if(GetAnalyser().IsObjectOnAxis(obj, AxisAngle::INTERNAL)
		/*|| GetAnalyser().IsObjectOnAxis(obj, AxisAngle::INCOMING)*/)
	{
		ax = &GetAnalyser();
		ax_prev = &GetSample();
	}

	// move detector around analyser axis
	else if(GetAnalyser().IsObjectOnAxis(obj, AxisAngle::OUTGOING))
	{
		ax = &GetAnalyser();
		ax_prev = &GetAnalyser();
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


/**
 * emit an update signal
 */
void Instrument::EmitUpdate()
{
	if(m_block_updates)
		return;

	(*m_sigUpdate)(*this);
}


/**
 * get the properties of an object in the instrument
 */
std::vector<ObjectProperty> Instrument::GetProperties(const std::string& objname) const
{
	// find the axis with the given id
	if(GetMonochromator().GetId() == objname)
	{
		return GetMonochromator().GetProperties();
	}
	else if(GetSample().GetId() == objname)
	{
		return GetSample().GetProperties();
	}
	else if(GetAnalyser().GetId() == objname)
	{
		return GetAnalyser().GetProperties();
	}


	// find mono/sample/ana geometry objects
	if(m_allow_editing)
	{
		if(auto props = GetMonochromator().GetProperties(objname); props.size())
			return props;
		if(auto props = GetSample().GetProperties(objname); props.size())
			return props;
		if(auto props = GetAnalyser().GetProperties(objname); props.size())
			return props;
	}

	return {};
}


/**
 * set the properties of an object in the instrument
 */
std::tuple<bool, std::shared_ptr<Geometry>>
Instrument::SetProperties(const std::string& objname,
	const std::vector<ObjectProperty>& props)
{
	// find the axis with the given id
	if(GetMonochromator().GetId() == objname)
	{
		GetMonochromator().SetProperties(props);
		return std::make_tuple(true, nullptr);
	}
	else if(GetSample().GetId() == objname)
	{
		GetSample().SetProperties(props);
		return std::make_tuple(true, nullptr);
	}
	else if(GetAnalyser().GetId() == objname)
	{
		GetAnalyser().SetProperties(props);
		return std::make_tuple(true, nullptr);
	}


	// find mono/sample/ana geometry objects
	if(m_allow_editing)
	{
		if(auto retval = GetMonochromator().SetProperties(objname, props); std::get<0>(retval))
			return retval;
		if(auto retval = GetSample().SetProperties(objname, props); std::get<0>(retval))
			return retval;
		if(auto retval = GetAnalyser().SetProperties(objname, props); std::get<0>(retval))
			return retval;
	}

	return std::make_tuple(false, nullptr);
}
