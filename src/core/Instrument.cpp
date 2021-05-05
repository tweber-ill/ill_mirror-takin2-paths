/**
 * instrument and walls
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


// ----------------------------------------------------------------------------
// instrument axis
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// instrument
// ----------------------------------------------------------------------------
Instrument::Instrument()
	: m_sigUpdate{std::make_shared<t_sig_update>()}
{
}


Instrument::~Instrument()
{
}


Instrument::Instrument(const Instrument& instr)
{
	*this = operator=(instr);
}


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

	this->m_sigUpdate = std::make_shared<t_sig_update>();
	return *this;
}


void Instrument::Clear()
{
	m_mono.Clear();
	m_sample.Clear();
	m_ana.Clear();

	m_sigUpdate = std::make_shared<t_sig_update>();
}


bool Instrument::Load(const pt::ptree& prop)
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


pt::ptree Instrument::Save() const
{
	pt::ptree prop;

	prop.put_child("monochromator", m_mono.Save());
	prop.put_child("sample", m_sample.Save());
	prop.put_child("analyser", m_ana.Save());

	return prop;
}


void Instrument::DragObject(bool drag_start, const std::string& obj, 
	t_real x_start, t_real y_start, t_real x, t_real y)
{
	Axis* ax = nullptr;
	Axis* ax_prev = nullptr;

	if(obj=="sample")
	{
		ax = &m_sample;
		ax_prev = &m_mono;
	}
	else if(obj=="analyser")
	{
		ax = &m_ana;
		ax_prev = &m_sample;
	}

	if(!ax || !ax_prev)
		return;

	auto pos_startcur = tl2::create<t_vec>({ x_start, y_start });
	auto pos_cur = tl2::create<t_vec>({ x, y });

	auto pos_ax = ax->GetTrafo(AxisAngle::IN) * tl2::create<t_vec>({ 0, 0, 0, 1 });
	auto pos_ax_prev = ax_prev->GetTrafo(AxisAngle::IN) * tl2::create<t_vec>({ 0, 0, 0, 1 });
	auto pos_ax_prev_in = ax_prev->GetTrafo(AxisAngle::IN) * tl2::create<t_vec>({ -1, 0, 0, 1 });
	pos_ax.resize(2);
	pos_ax_prev.resize(2);
	pos_ax_prev_in.resize(2);

	if(drag_start)
		m_drag_pos_axis_start = pos_ax;
	auto pos_drag = pos_cur - pos_startcur + m_drag_pos_axis_start;
	//t_real angle = tl2::angle<t_vec>(pos_ax_prev - pos_ax_prev_in, pos_ax - pos_ax_prev);
	t_real new_angle = tl2::angle<t_vec>(pos_ax_prev - pos_ax_prev_in, pos_drag - pos_ax_prev);

	//std::cout << angle/tl2::pi<t_real>*180. << " -> " << new_angle/tl2::pi<t_real>*180. << std::endl;
	ax_prev->SetAxisAngleOut(new_angle);
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

	return *this;
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
		for(const auto& comp : comps)
		{
			auto matGeo = comp->GetTrafo();
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
		for(AxisAngle axisangle : {AxisAngle::IN, AxisAngle::INTERNAL, AxisAngle::OUT})
		{
			const t_mat matAxis = axis.GetTrafo(axisangle);
			get_comp_circles(axis.GetComps(axisangle), circles, &matAxis);
		}
	};


	// extract 2d polygons from box geometry
	auto get_comp_polys = [](
		const std::vector<std::shared_ptr<Geometry>>& comps,
		std::vector<std::vector<t_vec>>& polys,
		const t_mat* matAxis = nullptr)
	{
		for(const auto& comp : comps)
		{
			auto matGeo = comp->GetTrafo();
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
				polys.emplace_back(std::move(vertices));
			}
		}
	};


	auto get_polys = [&get_comp_polys](const Axis& axis, 
		std::vector<std::vector<t_vec>>& polys)
	{
		// get geometries relative to incoming, internal, and outgoing axis
		for(AxisAngle axisangle : {AxisAngle::IN, AxisAngle::INTERNAL, AxisAngle::OUT})
		{
			const t_mat matAxis = axis.GetTrafo(axisangle);
			get_comp_polys(axis.GetComps(axisangle), polys, &matAxis);
		}
	};


	// check if two polygonal objects collide
	auto check_collision_poly_poly = [](
		const std::vector<std::vector<t_vec>>& polys1,
		const std::vector<std::vector<t_vec>>& polys2) -> bool
	{
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
		monoPolys, samplePolys, anaPolys, wallPolys;

	get_circles(mono, monoCircles);
	get_circles(sample, sampleCircles);
	get_circles(ana, anaCircles);
	get_comp_circles(walls, wallCircles);

	get_polys(mono, monoPolys);
	get_polys(sample, samplePolys);
	get_polys(ana, anaPolys);
	get_comp_polys(walls, wallPolys);

	// check for collisions
	// TODO: exclude checks for objects that are already colliding
	//       in the instrument definition file
	//if(check_collision_poly_poly(monoPolys, wallPolys))
	//	return true;
	if(check_collision_poly_poly(samplePolys, wallPolys))
		return true;
	if(check_collision_poly_poly(anaPolys, wallPolys))
		return true;

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

	//if(check_collision_circle_poly(monoCircles, wallPolys))
	//	return true;
	if(check_collision_circle_poly(sampleCircles, wallPolys))
		return true;
	if(check_collision_circle_poly(anaCircles, wallPolys))
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


void InstrumentSpace::DragObject(bool drag_start, const std::string& obj, 
	t_real x_start, t_real y_start, t_real x, t_real y)
{
	// cases concerning instrument axes
	if(obj=="monochromator" || obj=="sample" || obj=="analyser" || obj=="detector")
		m_instr.DragObject(drag_start, obj, x_start, y_start, x, y);
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------
/**
 * load an instrument space definition from an xml file
 */
std::tuple<bool, std::string> load_instrumentspace(
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
// ----------------------------------------------------------------------------
