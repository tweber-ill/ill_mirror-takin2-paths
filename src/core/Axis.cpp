/**
 * instrument axis
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include <unordered_map>
#include <optional>

#include "Axis.h"
#include "Instrument.h"

#include "src/libs/lines.h"
#include "src/libs/ptree.h"
#include "tlibs2/libs/file.h"
#include "tlibs2/libs/str.h"

namespace pt = boost::property_tree;


Axis::Axis(const std::string& id, const Axis* prev, const Axis* next, Instrument *instr)
	: m_id{id}, m_prev{prev}, m_next{next}, m_instr{instr}
{}


Axis::~Axis()
{
	Clear();
}


Axis::Axis(const Axis& axis)
{
	*this = operator=(axis);
}


const Axis& Axis::operator=(const Axis& axis)
{
	this->m_id = axis.m_id;

	// has to be set separately from the instrument
	this->m_prev = nullptr;
	this->m_next = nullptr;
	this->m_instr = nullptr;

	this->m_pos = axis.m_pos;

	this->m_angle_in = axis.m_angle_in;
	this->m_angle_out = axis.m_angle_out;
	this->m_angle_internal = axis.m_angle_internal;

	for(int i=0; i<2; ++i)
	{
		this->m_angle_in_limits[i] = axis.m_angle_in_limits[i];
		this->m_angle_out_limits[i] = axis.m_angle_out_limits[i];
		this->m_angle_internal_limits[i] = axis.m_angle_internal_limits[i];
	}

	this->m_comps_in = axis.m_comps_in;
	this->m_comps_out = axis.m_comps_out;
	this->m_comps_internal = axis.m_comps_internal;

	this->m_trafos_need_update = axis.m_trafos_need_update;
	this->m_trafoIncoming = axis.m_trafoIncoming;
	this->m_trafoInternal = axis.m_trafoInternal;
	this->m_trafoOutgoing = axis.m_trafoOutgoing;

	return *this;
}


void Axis::SetPreviousAxis(const Axis* axis)
{
	m_prev = axis; 
	m_trafos_need_update = true;
}


void Axis::SetNextAxis(const Axis* axis)
{
	m_next = axis;
	m_trafos_need_update = true;
}


void Axis::SetParentInstrument(Instrument* instr)
{
	m_instr = instr;
	m_trafos_need_update = true;
}


t_real Axis::GetAxisAngleInLowerLimit() const 
{ 
	if(m_angle_in_limits[0]) 
		return *m_angle_in_limits[0];
	else
		return -tl2::pi<t_real>;
}


t_real Axis::GetAxisAngleInUpperLimit() const 
{
	if(m_angle_in_limits[1])
		return *m_angle_in_limits[1];
	else
		return tl2::pi<t_real>; 
}


t_real Axis::GetAxisAngleOutLowerLimit() const 
{
	if(m_angle_out_limits[0])
		return *m_angle_out_limits[0];
	else
		return -tl2::pi<t_real>;
}


t_real Axis::GetAxisAngleOutUpperLimit() const 
{
	if(m_angle_out_limits[1])
		return *m_angle_out_limits[1];
	else
		return tl2::pi<t_real>;
}


t_real Axis::GetAxisAngleInternalLowerLimit() const 
{
	if(m_angle_internal_limits[0]) 
		return *m_angle_internal_limits[0];
	else
		return -tl2::pi<t_real>;
}


t_real Axis::GetAxisAngleInternalUpperLimit() const 
{
	if(m_angle_internal_limits[1])
		return *m_angle_internal_limits[1];
	else
		return tl2::pi<t_real>;
}


void Axis::SetAxisAngleInLowerLimit(t_real angle)
{
	m_angle_in_limits[0] = angle;
}


void Axis::SetAxisAngleInUpperLimit(t_real angle)
{
	m_angle_in_limits[1] = angle;
}


void Axis::SetAxisAngleOutLowerLimit(t_real angle)
{
	m_angle_out_limits[0] = angle;
}


void Axis::SetAxisAngleOutUpperLimit(t_real angle)
{
	m_angle_out_limits[1] = angle;
}


void Axis::SetAxisAngleInternalLowerLimit(t_real angle)
{
	m_angle_internal_limits[0] = angle;
}


void Axis::SetAxisAngleInternalUpperLimit(t_real angle)
{
	m_angle_internal_limits[1] = angle;
}


void Axis::Clear()
{
	m_comps_in.clear();
	m_comps_out.clear();
	m_comps_internal.clear();

	for(int i=0; i<2; ++i)
	{
		this->m_angle_in_limits[i] = std::nullopt;
		this->m_angle_out_limits[i] = std::nullopt;
		this->m_angle_internal_limits[i] = std::nullopt;
	}

	m_trafos_need_update = true;
}


bool Axis::Load(const pt::ptree& prop)
{
	// zero position
	if(auto optPos = prop.get_optional<std::string>("pos"); optPos)
	{
		m_pos.clear();

		tl2::get_tokens<t_real>(tl2::trimmed(*optPos), std::string{" \t,;"}, m_pos);
		if(m_pos.size() < 3)
			m_pos.resize(3);
	}

	// axis angles
	if(auto opt = prop.get_optional<t_real>("angle_in"); opt)
		m_angle_in = *opt/t_real{180}*tl2::pi<t_real>;
	if(auto opt = prop.get_optional<t_real>("angle_internal"); opt)
		m_angle_internal = *opt/t_real{180}*tl2::pi<t_real>;
	if(auto opt = prop.get_optional<t_real>("angle_out"); opt)
		m_angle_out = *opt/t_real{180}*tl2::pi<t_real>; //- m_angle_in;

	// angular limits
	if(auto opt = prop.get_optional<t_real>("angle_in_lower_limit"); opt)
		m_angle_in_limits[0] = *opt/t_real{180}*tl2::pi<t_real>;
	if(auto opt = prop.get_optional<t_real>("angle_in_upper_limit"); opt)
		m_angle_in_limits[1] = *opt/t_real{180}*tl2::pi<t_real>;
	if(auto opt = prop.get_optional<t_real>("angle_internal_lower_limit"); opt)
		m_angle_internal_limits[0] = *opt/t_real{180}*tl2::pi<t_real>;
	if(auto opt = prop.get_optional<t_real>("angle_internal_upper_limit"); opt)
		m_angle_internal_limits[1] = *opt/t_real{180}*tl2::pi<t_real>;
	if(auto opt = prop.get_optional<t_real>("angle_out_lower_limit"); opt)
		m_angle_out_limits[0] = *opt/t_real{180}*tl2::pi<t_real>;
	if(auto opt = prop.get_optional<t_real>("angle_out_upper_limit"); opt)
		m_angle_out_limits[1] = *opt/t_real{180}*tl2::pi<t_real>;

	auto load_geo = [this, &prop](const std::string& name,
		std::vector<std::shared_ptr<Geometry>>& comp_geo) -> void
	{
		if(auto geo = prop.get_child_optional(name); geo)
		{
			if(auto geoobj = Geometry::load(*geo); std::get<0>(geoobj))
			{
				// get individual 3d primitives that comprise this object
				for(auto& comp : std::get<1>(geoobj))
				{
					if(comp->GetId() == "")
						comp->SetId(m_id);
					comp_geo.emplace_back(std::move(comp));
				}
			}
		}
	};

	// geometry relative to incoming axis
	load_geo("geometry_in", m_comps_in);
	// internally rotated geometry
	load_geo("geometry_internal", m_comps_internal);
	// geometry relative to outgoing axis
	load_geo("geometry_out", m_comps_out);

	TrafosNeedUpdate();
	return true;
}


pt::ptree Axis::Save() const
{
	pt::ptree prop;

	// position
	prop.put<std::string>("pos", geo_vec_to_str(m_pos));

	// axis angles
	prop.put<t_real>("angle_in", m_angle_in/tl2::pi<t_real>*t_real(180));
	prop.put<t_real>("angle_internal", m_angle_internal/tl2::pi<t_real>*t_real(180));
	prop.put<t_real>("angle_out", m_angle_out/tl2::pi<t_real>*t_real(180));

	// angular limits
	if(m_angle_in_limits[0])
		prop.put<t_real>("angle_in_lower_limit", 
			*m_angle_in_limits[0]/tl2::pi<t_real>*t_real(180));
	if(m_angle_in_limits[1])
		prop.put<t_real>("angle_in_upper_limit", 
			*m_angle_in_limits[1]/tl2::pi<t_real>*t_real(180));
	if(m_angle_internal_limits[0])
		prop.put<t_real>("angle_internal_lower_limit", 
			*m_angle_internal_limits[0]/tl2::pi<t_real>*t_real(180));
	if(m_angle_internal_limits[1])
		prop.put<t_real>("angle_internal_upper_limit", 
			*m_angle_internal_limits[1]/tl2::pi<t_real>*t_real(180));
	if(m_angle_out_limits[0])
		prop.put<t_real>("angle_out_lower_limit", 
			*m_angle_out_limits[0]/tl2::pi<t_real>*t_real(180));
	if(m_angle_out_limits[1])
		prop.put<t_real>("angle_out_upper_limit", 
			*m_angle_out_limits[1]/tl2::pi<t_real>*t_real(180));

	// geometries
	auto allcomps = { m_comps_in, m_comps_internal, m_comps_out };
	auto allcompnames = { "geometry_in" , "geometry_internal", "geometry_out" };

	auto itercompname = allcompnames.begin();
	for(auto itercomp = allcomps.begin(); itercomp != allcomps.end(); ++itercomp)
	{
		pt::ptree propGeo;
		for(const auto& comp : *itercomp)
		{
			pt::ptree propgeo = comp->Save();
			propGeo.insert(propGeo.end(), propgeo.begin(), propgeo.end());
		}
		prop.put_child(*itercompname, propGeo);
		++itercompname;
	}

	return prop;
}


void Axis::TrafosNeedUpdate() const
{
	m_trafos_need_update = true;
	if(m_next)
		m_next->TrafosNeedUpdate();
}


void Axis::UpdateTrafos() const
{
	// trafo of previous axis
	t_mat matPrev = m_prev ? m_prev->GetTrafo(AxisAngle::OUTGOING) : tl2::unit<t_mat>(4);

	// local trafos
	const t_vec upaxis = tl2::create<t_vec>({0, 0, 1});
	t_mat matRotIn = tl2::hom_rotation<t_mat, t_vec>(upaxis, m_angle_in);
	t_mat matTrans = tl2::hom_translation<t_mat, t_real>(m_pos[0], m_pos[1], 0.);
	m_trafoIncoming = matPrev * matTrans * matRotIn;

	t_mat matRotInternal = tl2::hom_rotation<t_mat, t_vec>(upaxis, m_angle_internal);
	m_trafoInternal = m_trafoIncoming * matRotInternal;

	t_mat matRotOut = tl2::hom_rotation<t_mat, t_vec>(upaxis, m_angle_out);
	m_trafoOutgoing = m_trafoIncoming * matRotOut;
}


const t_mat& Axis::GetTrafo(AxisAngle which) const
{
	if(m_trafos_need_update)
	{
		UpdateTrafos();
		m_trafos_need_update = false;
	}

	switch(which)
	{
		case AxisAngle::INCOMING: return m_trafoIncoming;
		case AxisAngle::INTERNAL: return m_trafoInternal;
		case AxisAngle::OUTGOING: return m_trafoOutgoing;
	}

	return m_trafoIncoming;
}


const std::vector<std::shared_ptr<Geometry>>& Axis::GetComps(AxisAngle which) const
{
	switch(which)
	{
		case AxisAngle::INCOMING: return m_comps_in;
		case AxisAngle::OUTGOING: return m_comps_out;
		case AxisAngle::INTERNAL: return m_comps_internal;
	}

	return m_comps_in;
}


void Axis::SetAxisAngleIn(t_real angle)
{
	m_angle_in = angle;
	TrafosNeedUpdate();

	if(m_instr)
		m_instr->EmitUpdate();
}


void Axis::SetAxisAngleOut(t_real angle)
{
	m_angle_out = angle;
	TrafosNeedUpdate();

	if(m_instr)
		m_instr->EmitUpdate();
}


void Axis::SetAxisAngleInternal(t_real angle)
{
	m_angle_internal = angle;
	TrafosNeedUpdate();

	if(m_instr)
		m_instr->EmitUpdate();
}


bool Axis::IsObjectOnAxis(const std::string& obj, AxisAngle ax) const
{
	// get component collection depending on axis angle type
	const std::vector<std::shared_ptr<Geometry>>* comps = nullptr;

	if(ax == AxisAngle::INCOMING)
		comps = &m_comps_in;
	else if(ax == AxisAngle::INTERNAL)
		comps = &m_comps_internal;
	else if(ax == AxisAngle::OUTGOING)
		comps = &m_comps_out;
	
	if(!comps)
		return false;

	for(const auto& comp : *comps)
	{
		if(comp->GetId() == obj)
			return true;
	}

	return false;
}
