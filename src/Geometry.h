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

#include "globals.h"


enum class GeometryType
{
	BOX,
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
	virtual bool Load(const boost::property_tree::ptree& prop) = 0;

	virtual std::tuple<std::vector<t_vec>, std::vector<t_vec>, std::vector<t_vec>, t_mat>
	GetTriangles() = 0;

	virtual const std::string& GetId() const { return m_id; }
	virtual void SetId(const std::string& id) { m_id = id; }

	static std::tuple<bool, std::vector<std::shared_ptr<Geometry>>>
	load(const boost::property_tree::ptree& prop, const std::string& basePath="root");

protected:
	std::string m_id;
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
	t_vec m_pos1, m_pos2;
	t_real m_height, m_depth, m_length;
};
// ----------------------------------------------------------------------------

#endif
