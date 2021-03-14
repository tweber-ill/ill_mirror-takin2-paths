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
Axis::Axis()
{
}


Axis::~Axis()
{
}


void Axis::Clear()
{
}


bool Axis::Load(const boost::property_tree::ptree& prop, const std::string& basePath)
{
	m_pos = tl2::create<t_vec>({0, 0});
	if(auto opt = prop.get_optional<t_real>(basePath + "x"); opt)
		m_pos[0] = *opt;
	if(auto opt = prop.get_optional<t_real>(basePath + "y"); opt)
		m_pos[1] = *opt;

	return true;
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


bool Instrument::Load(const boost::property_tree::ptree& prop, const std::string& basePath)
{
	bool mono_ok = m_mono.Load(prop, basePath + "monochromator.");
	bool sample_ok = m_sample.Load(prop, basePath + "sample.");
	bool ana_ok = m_ana.Load(prop, basePath + "analyser.");

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
}


/**
 * load instrument and wall configuration
 */
bool InstrumentSpace::Load(const pt::ptree& prop, const std::string& basePath)
{
	Clear();
	m_instr.Clear();


	// floor size
	if(auto opt = prop.get_optional<t_real>(basePath + "floor_len_x"); opt)
		m_floorlen[0] = *opt;
	if(auto opt = prop.get_optional<t_real>(basePath + "floor_len_y"); opt)
		m_floorlen[1] = *opt;


	// walls
	if(auto walls = prop.get_child_optional(basePath + "walls"); walls)
	{
		// iterate wall segments
		for(const auto &wall : *walls)
		{
			auto id = wall.second.get<std::string>("<xmlattr>.id", "");

			if(auto geoobj = Geometry::load(wall.second, "geometry"); std::get<0>(geoobj))
			{
				// TODO
				for(auto& wallseg : std::get<1>(geoobj))
				{
					// override id
					if(id != "")
						wallseg->SetId(id);
					m_walls.emplace_back(std::move(wallseg));
				}
			}
		}
	}


	// instrument
	bool instr_ok = m_instr.Load(prop, basePath + "instrument.");

	return instr_ok;
}
// ----------------------------------------------------------------------------
