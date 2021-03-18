/**
 * geometry objects
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Geometry.h"
#include "tlibs2/libs/str.h"

#include <iostream>

namespace pt = boost::property_tree;



// ----------------------------------------------------------------------------
// geometry base class
// ----------------------------------------------------------------------------

Geometry::Geometry()
{
}


Geometry::~Geometry()
{
}


std::tuple<bool, std::vector<std::shared_ptr<Geometry>>>
Geometry::load(const boost::property_tree::ptree& prop, const std::string& basePath)
{
	std::vector<std::shared_ptr<Geometry>> geo_objs;

	// iterate geometry items
	if(auto geos = prop.get_child_optional(basePath); geos)
	{
		// iterate geometry item properties
		for(const auto &geo : *geos)
		{
			std::string geotype = geo.first;
			std::string geoid = geo.second.get<std::string>("<xmlattr>.id", "");
			//std::cout << "type = " << geotype << ", id = " << geoid << std::endl;

			if(geotype == "box")
			{
				auto box = std::make_shared<BoxGeometry>();
				box->m_id = geoid;

				if(box->Load(geo.second))
					geo_objs.emplace_back(std::move(box));
			}
			else if(geotype == "cylinder")
			{
				auto cyl = std::make_shared<CylinderGeometry>();
				cyl->m_id = geoid;

				if(cyl->Load(geo.second))
					geo_objs.emplace_back(std::move(cyl));
			}
			else
			{
				std::cerr << "Unknown geometry type \"" << geotype << "\"." << std::endl;
				continue;
			}
		}
	}

	return std::make_tuple(true, geo_objs);
}


bool Geometry::Load(const boost::property_tree::ptree& prop)
{
	// colour
	if(auto col = prop.get_optional<std::string>("colour"); col)
	{
		m_colour.clear();
		tl2::get_tokens<t_real>(tl2::trimmed(*col), std::string{" \t,;"}, m_colour);

		if(m_colour.size() < 3)
			m_colour.resize(3);
	}

	return true;
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// box
// ----------------------------------------------------------------------------

BoxGeometry::BoxGeometry()
{
}


BoxGeometry::~BoxGeometry()
{
}


void BoxGeometry::Clear()
{
}


bool BoxGeometry::Load(const boost::property_tree::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	if(auto optPos = prop.get_optional<std::string>("pos1"); optPos)
	{
		m_pos1.clear();

		tl2::get_tokens<t_real>(tl2::trimmed(*optPos), std::string{" \t,;"}, m_pos1);
		if(m_pos1.size() < 3)
			m_pos1.resize(3);
	}

	if(auto optPos = prop.get_optional<std::string>("pos2"); optPos)
	{
		m_pos2.clear();

		tl2::get_tokens<t_real>(tl2::trimmed(*optPos), std::string{" \t,;"}, m_pos2);
		if(m_pos2.size() < 3)
			m_pos2.resize(3);
	}

	m_height = prop.get<t_real>("height", 1.);
	m_depth = prop.get<t_real>("depth", 0.1);
	m_length = tl2::norm<t_vec>(m_pos1 - m_pos2);

	return true;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>, t_mat>
BoxGeometry::GetTriangles()
{
	auto solid = tl2::create_cuboid<t_vec>(m_length*0.5, m_depth*0.5, m_height*0.5);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	t_vec vecFrom = tl2::create<t_vec>({1, 0, 0});
	t_vec vecTo = m_pos2 - m_pos1;
	t_vec preTranslate = 0.5*(m_pos1 + m_pos2);
	t_vec postTranslate = tl2::create<t_vec>({0, 0, m_height*0.5});

	auto mat = tl2::get_arrow_matrix<t_vec, t_mat, t_real>(
		vecTo, 1., postTranslate, vecFrom, 1., preTranslate);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs, mat);
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// cylinder
// ----------------------------------------------------------------------------

CylinderGeometry::CylinderGeometry()
{
}


CylinderGeometry::~CylinderGeometry()
{
}


void CylinderGeometry::Clear()
{
}


bool CylinderGeometry::Load(const boost::property_tree::ptree& prop)
{
	if(!Geometry::Load(prop))
		return false;

	if(auto optPos = prop.get_optional<std::string>("pos"); optPos)
	{
		m_pos.clear();

		tl2::get_tokens<t_real>(tl2::trimmed(*optPos), std::string{" \t,;"}, m_pos);
		if(m_pos.size() < 3)
			m_pos.resize(3);
	}

	m_height = prop.get<t_real>("height", 1.);
	m_radius = prop.get<t_real>("radius", 0.1);

	return true;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>, t_mat>
CylinderGeometry::GetTriangles()
{
	auto solid = tl2::create_cylinder<t_vec>(m_radius, m_height, 1, 32);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	auto mat = tl2::hom_translation<t_mat, t_real>(0, 0, m_height*0.5);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs, mat);
}

// ----------------------------------------------------------------------------
