/**
 * instrument walls
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Instrument.h"
#include "src/libs/lines.h"

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


void Axis::Clear()
{
	m_comps_in.clear();
	m_comps_out.clear();
	m_comps_internal.clear();
}


bool Axis::Load(const boost::property_tree::ptree& prop)
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


void Instrument::Clear()
{
	m_mono.Clear();
	m_sample.Clear();
	m_ana.Clear();

	m_sigUpdate = std::make_shared<t_sig_update>();
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
					std::get<0>(circle), std::get<1>(circle),
					poly))
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
// ----------------------------------------------------------------------------
