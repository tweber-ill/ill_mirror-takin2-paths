/**
 * instrument axis
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include <unordered_map>
#include <optional>

#include "Instrument.h"
#include "src/libs/lines.h"
#include "src/libs/ptree.h"
#include "tlibs2/libs/file.h"
#include "tlibs2/libs/str.h"

namespace pt = boost::property_tree;


Axis::Axis(const std::string& id, const Axis* prev, Instrument *instr)
	: m_id{id}, m_prev{prev}, m_instr{instr}
{
}


Axis::~Axis()
{
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
	this->m_instr = nullptr;

	this->m_pos = axis.m_pos;

	this->m_angle_in = axis.m_angle_in;
	this->m_angle_out = axis.m_angle_out;
	this->m_angle_internal = axis.m_angle_internal;

	this->m_comps_in = axis.m_comps_in;
	this->m_comps_out = axis.m_comps_out;
	this->m_comps_internal = axis.m_comps_internal;

	return *this;
}


void Axis::Clear()
{
	m_comps_in.clear();
	m_comps_out.clear();
	m_comps_internal.clear();
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


t_mat Axis::GetTrafo(AxisAngle which) const
{
	// trafo of previous axis
	t_mat matPrev = tl2::unit<t_mat>(4);
	if(m_prev)
		matPrev = m_prev->GetTrafo(AxisAngle::OUT);

	// local trafos
	t_vec upaxis = tl2::create<t_vec>({0, 0, 1});
	t_mat matRotIn = tl2::hom_rotation<t_mat, t_vec>(upaxis, m_angle_in);
	t_mat matRotOut = tl2::hom_rotation<t_mat, t_vec>(upaxis, m_angle_out);
	t_mat matRotInternal = tl2::hom_rotation<t_mat, t_vec>(upaxis, m_angle_internal);
	t_mat matTrans = tl2::hom_translation<t_mat, t_real>(m_pos[0], m_pos[1], 0.);
	auto [matTransInv, transinvok] = tl2::inv<t_mat>(matTrans);

	auto matTotal = matPrev * matTrans * matRotIn;
	if(which==AxisAngle::INTERNAL)
		matTotal *= matRotInternal;
	if(which==AxisAngle::OUT)
		matTotal *= matRotOut;
	return matTotal;
}


const std::vector<std::shared_ptr<Geometry>>& Axis::GetComps(AxisAngle which) const
{
	switch(which)
	{
		case AxisAngle::IN: return m_comps_in;
		case AxisAngle::OUT: return m_comps_out;
		case AxisAngle::INTERNAL: return m_comps_internal;
	}

	return m_comps_in;
}


void Axis::SetAxisAngleIn(t_real angle)
{
	m_angle_in = angle;
	if(m_instr)
		m_instr->EmitUpdate();
}


void Axis::SetAxisAngleOut(t_real angle)
{
	m_angle_out = angle;
	if(m_instr)
		m_instr->EmitUpdate();
}


void Axis::SetAxisAngleInternal(t_real angle)
{
	m_angle_internal = angle;
	if(m_instr)
		m_instr->EmitUpdate();
}


bool Axis::IsObjectOnAxis(const std::string& obj, AxisAngle ax) const
{
	// get component collection depending on axis angle type
	const std::vector<std::shared_ptr<Geometry>>* comps = nullptr;

	if(ax == AxisAngle::IN)
		comps = &m_comps_in;
	else if(ax == AxisAngle::INTERNAL)
		comps = &m_comps_internal;
	else if(ax == AxisAngle::OUT)
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
