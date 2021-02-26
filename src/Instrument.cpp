/**
 * instrument walls
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Instrument.h"
namespace pt = boost::property_tree;


Instrument::Instrument()
{
}


Instrument::~Instrument()
{
}


void Instrument::Clear()
{
	// reset to defaults
	m_floorlen[0] = m_floorlen[1] = 10.;

	// clear
	m_walls.clear();
}


/**
 * load instrument and wall configuration
 */
bool Instrument::Load(const pt::ptree& prop, const std::string& basePath)
{
	Clear();

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

			auto optX1 = wall.second.get_optional<t_real>("x1");
			auto optX2 = wall.second.get_optional<t_real>("x2");
			auto optY1 = wall.second.get_optional<t_real>("y1");
			auto optY2 = wall.second.get_optional<t_real>("y2");
			auto height = wall.second.get<t_real>("height", 1.);
			auto depth = wall.second.get<t_real>("depth", 0.1);

			if(!optX1 || !optX2 || !optY1 || !optY2)
			{
				std::cerr << "Wall \"" << id << "\" definition is incomplete, ignoring." << std::endl;
				continue;
			}

			t_vec pos1 = tl2::create<t_vec>({ *optX1, *optY1 });
			t_vec pos2 = tl2::create<t_vec>({ *optX2, *optY2 });

			Wall thewall
			{
				.id = id,
				.pos1 = pos1,
				.pos2 = pos2,
				.height = height,
				.depth = depth,
				.length = tl2::norm<t_vec>(pos1 - pos2)
			};

			m_walls.emplace_back(std::move(thewall));
		}
	}

	return true;
}
