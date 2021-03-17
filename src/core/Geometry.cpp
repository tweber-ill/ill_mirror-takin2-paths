/**
 * geometry objects
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include <iostream>

#include "Geometry.h"
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
	auto optX1 = prop.get_optional<t_real>("x1");
	auto optX2 = prop.get_optional<t_real>("x2");
	auto optY1 = prop.get_optional<t_real>("y1");
	auto optY2 = prop.get_optional<t_real>("y2");

	if(!optX1 || !optX2 || !optY1 || !optY2)
	{
		std::cerr << "Box \"" << m_id << "\" definition is incomplete, ignoring." << std::endl;
		return false;
	}

	m_pos1 = tl2::create<t_vec>({ *optX1, *optY1, 0 });
	m_pos2 = tl2::create<t_vec>({ *optX2, *optY2, 0 });

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
	auto optX = prop.get_optional<t_real>("x");
	auto optY = prop.get_optional<t_real>("y");

	if(!optX || !optY)
	{
		std::cerr << "Cylinder \"" << m_id << "\" definition is incomplete, ignoring." << std::endl;
		return false;
	}

	m_pos = tl2::create<t_vec>({ *optX, *optY, 0 });

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
