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
// helper functions
// ----------------------------------------------------------------------------

/**
 * convert a vector to a serialisable string
 */
std::string geo_vec_to_str(const t_vec& vec)
{
	std::ostringstream ostr;
	for(t_real val : vec)
		ostr << val << " ";

	return ostr.str();
}


// ----------------------------------------------------------------------------



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
Geometry::load(const pt::ptree& prop)
{
	std::vector<std::shared_ptr<Geometry>> geo_objs;

	// iterate geometry items
	for(const auto& geo : prop)
	{
		const std::string& geotype = geo.first;
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
		else if(geotype == "sphere")
		{
			auto sph = std::make_shared<SphereGeometry>();
			sph->m_id = geoid;

			if(sph->Load(geo.second))
				geo_objs.emplace_back(std::move(sph));
		}
		else
		{
			std::cerr << "Unknown geometry type \"" << geotype << "\"." << std::endl;
			continue;
		}
	}

	return std::make_tuple(true, geo_objs);
}


bool Geometry::Load(const pt::ptree& prop)
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


pt::ptree Geometry::Save() const
{
	pt::ptree prop;

	prop.put<std::string>("<xmlattr>.id", GetId());
	prop.put<std::string>("colour", geo_vec_to_str(m_colour));

	return prop;
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


bool BoxGeometry::Load(const pt::ptree& prop)
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


pt::ptree BoxGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<std::string>("pos1", geo_vec_to_str(m_pos1));
	prop.put<std::string>("pos2", geo_vec_to_str(m_pos2));
	prop.put<t_real>("height", m_height);
	prop.put<t_real>("depth", m_depth);

	pt::ptree propBox;
	propBox.put_child("box", prop);
	return propBox;
}


t_mat BoxGeometry::GetTrafo() const
{
	t_vec vecFrom = tl2::create<t_vec>({1, 0, 0});
	t_vec vecTo = m_pos2 - m_pos1;
	t_vec preTranslate = 0.5*(m_pos1 + m_pos2);
	t_vec postTranslate = tl2::create<t_vec>({0, 0, m_height*0.5});

	auto mat = tl2::get_arrow_matrix<t_vec, t_mat, t_real>(
		vecTo, 1., postTranslate, vecFrom, 1., preTranslate);
	return mat;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
BoxGeometry::GetTriangles() const
{
	auto solid = tl2::create_cuboid<t_vec>(m_length*0.5, m_depth*0.5, m_height*0.5);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs);
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


bool CylinderGeometry::Load(const pt::ptree& prop)
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


pt::ptree CylinderGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<std::string>("pos", geo_vec_to_str(m_pos));
	prop.put<t_real>("height", m_height);
	prop.put<t_real>("radius", m_radius);

	pt::ptree propCyl;
	propCyl.put_child("cylinder", prop);
	return propCyl;
}


t_mat CylinderGeometry::GetTrafo() const
{
	auto mat = tl2::hom_translation<t_mat, t_real>(0, 0, m_height*0.5);
	return mat;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
CylinderGeometry::GetTriangles() const
{
	auto solid = tl2::create_cylinder<t_vec>(m_radius, m_height, 1, 32);
	auto [verts, norms, uvs] = tl2::create_triangles<t_vec>(solid);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(verts, norms, uvs);
}

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// sphere
// ----------------------------------------------------------------------------

SphereGeometry::SphereGeometry()
{
}


SphereGeometry::~SphereGeometry()
{
}


void SphereGeometry::Clear()
{
}


bool SphereGeometry::Load(const pt::ptree& prop)
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

	m_radius = prop.get<t_real>("radius", 0.1);

	return true;
}


pt::ptree SphereGeometry::Save() const
{
	pt::ptree prop = Geometry::Save();

	prop.put<std::string>("pos", geo_vec_to_str(m_pos));
	prop.put<t_real>("radius", m_radius);

	pt::ptree propSphere;
	propSphere.put_child("sphere", prop);
	return propSphere;
}


t_mat SphereGeometry::GetTrafo() const
{
	auto mat = tl2::hom_translation<t_mat, t_real>(0, 0, m_radius*0.5);
	return mat;
}


std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>>
SphereGeometry::GetTriangles() const
{
	const int numsubdivs = 2;
	auto solid = tl2::create_icosahedron<t_vec>(1);
	auto [triagverts, norms, uvs] = tl2::spherify<t_vec>(
		tl2::subdivide_triangles<t_vec>(
			tl2::create_triangles<t_vec>(solid), numsubdivs), m_radius);

	//tl2::transform_obj(verts, norms, mat, true);
	return std::make_tuple(triagverts, norms, uvs);
}

// ----------------------------------------------------------------------------
