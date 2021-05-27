/**
 * instrument space and walls
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


InstrumentSpace::InstrumentSpace()
	: m_sigUpdate{std::make_shared<t_sig_update>()}
{
}


InstrumentSpace::~InstrumentSpace()
{
}


InstrumentSpace::InstrumentSpace(const InstrumentSpace& instr)
{
	*this = instr;
}


const InstrumentSpace& InstrumentSpace::operator=(const InstrumentSpace& instr)
{
	this->m_floorlen[0] = instr.m_floorlen[0];
	this->m_floorlen[1] = instr.m_floorlen[1];

	this->m_walls = instr.m_walls;
	this->m_instr = instr.m_instr;

	this->m_sigUpdate = std::make_shared<t_sig_update>();
	return *this;
}


void InstrumentSpace::Clear()
{
	// reset to defaults
	m_floorlen[0] = m_floorlen[1] = 10.;

	// clear
	m_walls.clear();
	m_instr.Clear();

	m_sigUpdate = std::make_shared<t_sig_update>();
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


pt::ptree InstrumentSpace::Save() const
{
	pt::ptree prop;

	// floor
	prop.put<t_real>(FILE_BASENAME "instrument_space.floor.len_x", m_floorlen[0]);
	prop.put<t_real>(FILE_BASENAME "instrument_space.floor.len_y", m_floorlen[1]);

	// walls
	pt::ptree propwalls;
	for(std::size_t wallidx=0; wallidx<m_walls.size(); ++wallidx)
	{
		const auto& wall = m_walls[wallidx];

		pt::ptree propwall;
		propwall.put<std::string>("<xmlattr>.id", "wall " + std::to_string(wallidx+1));
		propwall.put_child("geometry", wall->Save());

		pt::ptree propwall2;
		propwall2.put_child("wall", propwall);
		propwalls.insert(propwalls.end(), propwall2.begin(), propwall2.end());
	}

	prop.put_child(FILE_BASENAME "instrument_space.walls", propwalls);
	prop.put_child(FILE_BASENAME "instrument_space.instrument", m_instr.Save());

	return prop;
}


/**
 * check for collisions, using a 2d representation of the instrument space
 */
bool InstrumentSpace::CheckCollision2D() const
{
	// extract circles from cylinder and sphere geometries
	auto get_comp_circles = [](
		const std::vector<std::shared_ptr<Geometry>>& comps,
		std::vector<std::tuple<t_vec, t_real>>& circles,
		const t_mat* matAxis = nullptr)
	{
		circles.reserve(circles.size() + comps.size());

		for(const auto& comp : comps)
		{
			const t_mat& matGeo = comp->GetTrafo();
			t_mat mat = matAxis ? (*matAxis) * matGeo : matGeo;

			if(comp->GetType() == GeometryType::CYLINDER)
			{
				auto cyl = std::dynamic_pointer_cast<CylinderGeometry>(comp);

				t_vec pos = cyl->GetPos();
				t_real rad = cyl->GetRadius();

				// trafo in homogeneous coordinates
				if(pos.size() < 4)
					pos.push_back(1);
				pos = mat * pos;

				// only two dimensions needed
				pos.resize(2);
				circles.emplace_back(std::make_tuple(pos, rad));
			}
			else if(comp->GetType() == GeometryType::SPHERE)
			{
				auto sph = std::dynamic_pointer_cast<SphereGeometry>(comp);

				t_vec pos = sph->GetPos();
				t_real rad = sph->GetRadius();

				// trafo in homogeneous coordinates
				if(pos.size() < 4)
					pos.push_back(1);
				pos = mat * pos;

				// only two dimensions needed
				pos.resize(2);
				circles.emplace_back(std::make_tuple(pos, rad));
			}
		}
	};
	

	auto get_circles = [&get_comp_circles](const Axis& axis, 
		std::vector<std::tuple<t_vec, t_real>>& circles)
	{
		// get geometries relative to incoming, internal, and outgoing axis
		for(AxisAngle axisangle : {AxisAngle::INCOMING, AxisAngle::INTERNAL, AxisAngle::OUTGOING})
		{
			const t_mat& matAxis = axis.GetTrafo(axisangle);
			get_comp_circles(axis.GetComps(axisangle), circles, &matAxis);
		}
	};


	// extract 2d polygon from box geometry
	auto get_comp_polys = [](
		const std::shared_ptr<Geometry>& comp,
		std::vector<t_vec>& poly,
		const t_mat* matAxis = nullptr)
	{
		const t_mat& matGeo = comp->GetTrafo();
		t_mat mat = matAxis ? (*matAxis) * matGeo : matGeo;

		if(comp->GetType() == GeometryType::BOX)
		{
			auto cyl = std::dynamic_pointer_cast<BoxGeometry>(comp);

			t_real lx = cyl->GetLength() * t_real(0.5);
			t_real ly = cyl->GetDepth() * t_real(0.5);
			t_real lz = cyl->GetHeight() * t_real(0.5);

			std::vector<t_vec> vertices =
			{
				mat * tl2::create<t_vec>({ +lx, -ly, -lz, 1 }),	// vertex 0
				mat * tl2::create<t_vec>({ -lx, -ly, -lz, 1 }),	// vertex 1
				mat * tl2::create<t_vec>({ -lx, +ly, -lz, 1 }),	// vertex 2
				mat * tl2::create<t_vec>({ +lx, +ly, -lz, 1 }),	// vertex 3
			};

			// only two dimensions needed
			for(t_vec& vec : vertices)
				vec.resize(2);

			poly = std::move(vertices);
		}
	};


	// extract 2d polygons from box geometries
	auto get_comps_polys = [&get_comp_polys](
		const std::vector<std::shared_ptr<Geometry>>& comps,
		std::vector<std::vector<t_vec>>& polys,
		const t_mat* matAxis = nullptr)
	{
		polys.reserve(polys.size() + comps.size());

		for(const auto& comp : comps)
		{
			std::vector<t_vec> poly;
			get_comp_polys(comp, poly, matAxis);
			if(poly.size())
				polys.emplace_back(std::move(poly));
		}
	};


	auto get_polys = [&get_comps_polys](const Axis& axis, 
		std::vector<std::vector<t_vec>>& polys)
	{
		// get geometries relative to incoming, internal, and outgoing axis
		for(AxisAngle axisangle : {AxisAngle::INCOMING, AxisAngle::INTERNAL, AxisAngle::OUTGOING})
		{
			const t_mat& matAxis = axis.GetTrafo(axisangle);
			get_comps_polys(axis.GetComps(axisangle), polys, &matAxis);
		}
	};


	// check if two polygonal objects collide
	auto check_collision_poly_poly = [](
		const std::vector<std::vector<t_vec>>& polys1,
		const std::vector<std::vector<t_vec>>& polys2,
		const std::tuple<t_vec, t_vec>& bb1,
		const std::tuple<t_vec, t_vec>& bb2) -> bool
	{
		if(!tl2::collide_bounding_boxes(bb1, bb2))
			return false;

		for(std::size_t idx1=0; idx1<polys1.size(); ++idx1)
		{
			const auto& poly1 = polys1[idx1];

			for(std::size_t idx2=0; idx2<polys2.size(); ++idx2)
			{
				const auto& poly2 = polys2[idx2];

				if(geo::collide_poly_poly<t_vec>(poly1, poly2))
					return true;
			}
		}

		return false;
	};


	// check if two circular objects collide
	auto check_collision_circle_circle = [](
		const std::vector<std::tuple<t_vec, t_real>>& circles1,
		const std::vector<std::tuple<t_vec, t_real>>& circles2) -> bool
	{
		for(std::size_t idx1=0; idx1<circles1.size(); ++idx1)
		{
			const auto& circle1 = circles1[idx1];

			for(std::size_t idx2=0; idx2<circles2.size(); ++idx2)
			{
				const auto& circle2 = circles2[idx2];

				//using namespace tl2_ops;
				//std::cout << "circle 1: " << std::get<0>(circle1) << " " << std::get<1>(circle1) << std::endl;
				//std::cout << "circle 2: " << std::get<0>(circle2) << " " << std::get<1>(circle2) << std::endl;

				if(geo::collide_circle_circle<t_vec>(
					std::get<0>(circle1), std::get<1>(circle1),
					std::get<0>(circle2), std::get<1>(circle2)))
					return true;
			}
		}

		return false;
	};


	// check if a circular and a polygonal object collide
	auto check_collision_circle_poly = [](
		const std::vector<std::tuple<t_vec, t_real>>& circles,
		const std::vector<std::vector<t_vec>>& polys) -> bool
	{
		for(std::size_t idx1=0; idx1<circles.size(); ++idx1)
		{
			const auto& circle = circles[idx1];

			for(std::size_t idx2=0; idx2<polys.size(); ++idx2)
			{
				const auto& poly = polys[idx2];

				if(geo::collide_circle_poly<t_vec>(
					std::get<0>(circle), std::get<1>(circle),poly))
					return true;
			}
		}

		return false;
	};


	// get 2d objects from instrument components
	const Axis& mono = GetInstrument().GetMonochromator();
	const Axis& sample = GetInstrument().GetSample();
	const Axis& ana = GetInstrument().GetAnalyser();
	const auto& walls = GetWalls();

	std::vector<std::tuple<t_vec, t_real>> 
		monoCircles, sampleCircles, anaCircles, wallCircles;
	std::vector<std::vector<t_vec>> 
		monoPolys, samplePolys, anaPolys;

	get_circles(mono, monoCircles);
	get_circles(sample, sampleCircles);
	get_circles(ana, anaCircles);
	get_comp_circles(walls, wallCircles);

	get_polys(mono, monoPolys);
	get_polys(sample, samplePolys);
	get_polys(ana, anaPolys);

	// get bounding boxes
	auto monoBB = tl2::bounding_box<t_vec, std::vector>(monoPolys, 2);
	auto sampleBB = tl2::bounding_box<t_vec, std::vector>(samplePolys, 2);
	auto anaBB = tl2::bounding_box<t_vec, std::vector>(anaPolys, 2);


	// check for collisions with the walls
	for(const auto& wall : walls)
	{
		std::vector<t_vec> wallPoly;
		get_comp_polys(wall, wallPoly);
		std::vector<std::vector<t_vec>> wallPolys;
		if(wallPoly.size() == 0)
			continue;
		wallPolys.emplace_back(std::move(wallPoly));

		auto wallBB = tl2::bounding_box<t_vec, std::vector>(wallPolys);

		// check for collisions
		// TODO: exclude checks for objects that are already colliding
		//       in the instrument definition file

		//if(check_collision_poly_poly(monoPolys, wallPolys, monoBB, wallBB))
		//	return true;
		if(check_collision_poly_poly(samplePolys, wallPolys, sampleBB, wallBB))
			return true;
		if(check_collision_poly_poly(anaPolys, wallPolys, anaBB, wallBB))
			return true;

		//if(check_collision_circle_poly(monoCircles, wallPolys))
		//	return true;
		if(check_collision_circle_poly(sampleCircles, wallPolys))
			return true;
		if(check_collision_circle_poly(anaCircles, wallPolys))
			return true;
	}

	//if(check_collision_circle_circle(monoCircles, wallCircles))
	//	return true;
	if(check_collision_circle_circle(sampleCircles, wallCircles))
		return true;
	if(check_collision_circle_circle(anaCircles, wallCircles))
		return true;

	if(check_collision_circle_circle(monoCircles, sampleCircles))
		return true;
	if(check_collision_circle_circle(sampleCircles, anaCircles))
		return true;
	if(check_collision_circle_circle(monoCircles, anaCircles))
		return true;

	if(check_collision_circle_poly(monoCircles, anaPolys))
		return true;
	if(check_collision_circle_poly(monoCircles, samplePolys))
		return true;
	//if(check_collision_circle_poly(sampleCircles, monoPolys))
	//	return true;
	if(check_collision_circle_poly(sampleCircles, anaPolys))
		return true;
	if(check_collision_circle_poly(anaCircles, monoPolys))
		return true;
	//if(check_collision_circle_poly(anaCircles, samplePolys))
	//	return true;

	return false;
}


/**
 * an object is requested to be dragged from the gui
 */
void InstrumentSpace::DragObject(bool drag_start, const std::string& obj, 
	t_real x_start, t_real y_start, t_real x, t_real y)
{
	// cases concerning instrument axes
	m_instr.DragObject(drag_start, obj, x_start, y_start, x, y);


	// cases involving walls
	bool wall_dragged = false;

	for(auto& wall : GetWalls())
	{
		if(wall->GetId() != obj)
			continue;

		t_vec pos_startcur = tl2::create<t_vec>({ x_start, y_start });
		t_vec pos_cur = tl2::create<t_vec>({ x, y });

		t_vec pos_obj3 = wall->GetCentre();
		t_vec pos_obj = pos_obj3;
		pos_obj.resize(2);

		if(drag_start)
			m_drag_pos_axis_start = pos_obj;

		t_vec pos_drag = pos_cur - pos_startcur + m_drag_pos_axis_start;
		pos_obj3[0] = pos_drag[0];
		pos_obj3[1] = pos_drag[1];

		wall->SetCentre(pos_obj3);
		wall_dragged = true;
	}

	if(wall_dragged)
		EmitUpdate();
}


/**
 * load an instrument space definition from an xml file
 */
std::tuple<bool, std::string> InstrumentSpace::load(
	const std::string& filename, InstrumentSpace& instrspace)
{
	if(filename == "" || !fs::exists(fs::path(filename)))
		return std::make_tuple(false, "Instrument file \"" + filename + "\" does not exist.");

	// open xml
	std::ifstream ifstr{filename};
	if(!ifstr)
		return std::make_tuple(false, "Could not read instrument file \"" + filename + "\".");

	// read xml
	pt::ptree prop;
	pt::read_xml(ifstr, prop);
	// check format and version
	if(auto opt = prop.get_optional<std::string>(FILE_BASENAME "ident");
		!opt || *opt != PROG_IDENT)
	{
		return std::make_tuple(false, "Instrument file \"" + filename + "\" has invalid identifier.");
	}

	// get variables from config file
	std::unordered_map<std::string, std::string> propvars;

	if(auto vars = prop.get_child_optional(FILE_BASENAME "variables"); vars)
	{
		// iterate variables
		for(const auto &var : *vars)
		{
			const auto& key = var.first;
			std::string val = var.second.get<std::string>("<xmlattr>.value", "");
			//std::cout << key << " = " << val << std::endl;

			propvars.insert(std::make_pair(key, val));
		}

		replace_ptree_values(prop, propvars);
	}

	// load instrument definition
	if(auto instr = prop.get_child_optional(FILE_BASENAME "instrument_space"); instr)
	{
		if(!instrspace.Load(*instr))
			return std::make_tuple(false, "Instrument configuration \"" + filename + "\" could not be loaded.");
	}
	else
	{
		return std::make_tuple(false, "No instrument definition found in \"" + filename + "\".");
	}

	std::ostringstream timestamp;
	if(auto optTime = prop.get_optional<t_real>(FILE_BASENAME "timestamp"); optTime)
		timestamp << tl2::epoch_to_str(*optTime);;

	return std::make_tuple(true, timestamp.str());
}
