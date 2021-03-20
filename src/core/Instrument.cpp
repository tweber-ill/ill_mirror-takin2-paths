/**
 * instrument walls
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Instrument.h"

namespace pt = boost::property_tree;


// ----------------------------------------------------------------------------
// instrument axis
// ----------------------------------------------------------------------------
Axis::Axis(const std::string& id, const Axis* prev) : m_id{id}, m_prev{prev}
{
}


Axis::~Axis()
{
}


void Axis::Clear()
{
	m_comps.clear();
}


bool Axis::Load(const boost::property_tree::ptree& prop)
{
	// zero position
	m_pos = tl2::create<t_vec>({0, 0});
	if(auto opt = prop.get_optional<t_real>("x"); opt)
		m_pos[0] = *opt;
	if(auto opt = prop.get_optional<t_real>("y"); opt)
		m_pos[1] = *opt;

	// axis angle
	if(auto opt = prop.get_optional<t_real>("angle"); opt)
		m_angle = *opt;

	// geometry
	if(auto geo = prop.get_child_optional("geometry"); geo)
	{
		if(auto geoobj = Geometry::load(*geo); std::get<0>(geoobj))
		{
			// get individual 3d primitives that comprise this object
			for(auto& comp : std::get<1>(geoobj))
			{
				if(comp->GetId() == "")
					comp->SetId(m_id);
				m_comps.emplace_back(std::move(comp));
			}
		}
	}

	return true;
}


t_mat Axis::GetTrafo() const
{
	// trafo of previous axis
	t_mat matPrev = tl2::unit<t_mat>(4);
	if(m_prev)
		matPrev = m_prev->GetTrafo();

	// local trafos
	t_vec upaxis = tl2::create<t_vec>({0, 0, 1});
	t_mat matRot = tl2::hom_rotation<t_mat, t_vec>(upaxis, m_angle/t_real{180}*tl2::pi<t_real>);
	t_mat matTrans = tl2::hom_translation<t_mat, t_real>(m_pos[0], m_pos[1], 0.);

	return matPrev * matRot * matTrans;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// instrument
// ----------------------------------------------------------------------------
Instrument::Instrument()
{
}


Instrument::~Instrument()
{
}


void Instrument::Clear()
{
	m_mono.Clear();
	m_sample.Clear();
	m_ana.Clear();
}


bool Instrument::Load(const boost::property_tree::ptree& prop)
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
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// instrument space
// ----------------------------------------------------------------------------
InstrumentSpace::InstrumentSpace()
{
}


InstrumentSpace::~InstrumentSpace()
{
}


void InstrumentSpace::Clear()
{
	// reset to defaults
	m_floorlen[0] = m_floorlen[1] = 10.;

	// clear
	m_walls.clear();
	m_instr.Clear();
}


/**
 * load instrument and wall configuration
 */
bool InstrumentSpace::Load(const pt::ptree& prop)
{
	Clear();
	m_instr.Clear();


	// floor size
	if(auto opt = prop.get_optional<t_real>("floor.len_x"); opt)
		m_floorlen[0] = *opt;
	if(auto opt = prop.get_optional<t_real>("floor.len_y"); opt)
		m_floorlen[1] = *opt;


	// walls
	if(auto walls = prop.get_child_optional("walls"); walls)
	{
		// iterate wall segments
		for(const auto &wall : *walls)
		{
			auto id = wall.second.get<std::string>("<xmlattr>.id", "");
			auto geo = wall.second.get_child_optional("geometry");
			if(!geo)
				continue;

			if(auto geoobj = Geometry::load(*geo); std::get<0>(geoobj))
			{
				// get individual 3d primitives that comprise this wall
				for(auto& wallseg : std::get<1>(geoobj))
				{
					if(wallseg->GetId() == "")
						wallseg->SetId(id);
					m_walls.emplace_back(std::move(wallseg));
				}
			}
		}
	}


	// instrument
	bool instr_ok = false;
	if(auto instr = prop.get_child_optional("instrument"); instr)
		instr_ok = m_instr.Load(*instr);

	return instr_ok;
}
// ----------------------------------------------------------------------------
