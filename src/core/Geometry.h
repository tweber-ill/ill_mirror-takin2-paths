/**
 * geometry objects
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __GEO_OBJ_H__
#define __GEO_OBJ_H__

#include <tuple>
#include <memory>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "types.h"


enum class GeometryType
{
	BOX,
	CYLINDER,
};


// ----------------------------------------------------------------------------
// geometry base class
// ----------------------------------------------------------------------------
class Geometry
{
public:
	Geometry();
	virtual ~Geometry();

	virtual GeometryType GetType() const = 0;

	virtual void Clear() = 0;
	virtual bool Load(const boost::property_tree::ptree& prop);

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>, t_mat>
	GetTriangles() = 0;

	virtual const std::string& GetId() const { return m_id; }
	virtual void SetId(const std::string& id) { m_id = id; }

	virtual const t_vec& GetColour() const { return m_colour; }
	virtual void SetColour(const t_vec& col) { m_colour = col; }

	static std::tuple<bool, std::vector<std::shared_ptr<Geometry>>>
	load(const boost::property_tree::ptree& prop);

protected:
	std::string m_id;
	t_vec m_colour = tl2::create<t_vec>({1, 0, 0});
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// box
// ----------------------------------------------------------------------------
class BoxGeometry : public Geometry
{
public:
	BoxGeometry();
	virtual ~BoxGeometry();

	virtual GeometryType GetType() const override { return GeometryType::BOX; }

	virtual void Clear() override;
	virtual bool Load(const boost::property_tree::ptree& prop) override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>, t_mat>
	GetTriangles() override;


private:
	t_vec m_pos1 = tl2::create<t_vec>({0, 0, 0});
	t_vec m_pos2 = tl2::create<t_vec>({0, 0, 0});
	t_real m_height{}, m_depth{}, m_length{};
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// cylinder
// ----------------------------------------------------------------------------
class CylinderGeometry : public Geometry
{
public:
	CylinderGeometry();
	virtual ~CylinderGeometry();

	virtual GeometryType GetType() const override { return GeometryType::CYLINDER; }

	virtual void Clear() override;
	virtual bool Load(const boost::property_tree::ptree& prop) override;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>, t_mat>
	GetTriangles() override;


private:
	t_vec m_pos = tl2::create<t_vec>({0, 0, 0});
	t_real m_height{}, m_radius{};
};
// ----------------------------------------------------------------------------

#endif
