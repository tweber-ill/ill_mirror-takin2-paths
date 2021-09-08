/**
 * trapezoid map
 * @author Tobias Weber <tweber@ill.fr>
 * @date Oct/Nov-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license GPLv3, see 'LICENSE' file
 *
 * Reference for the algorithms:
 *   - (Berg 2008) "Computational Geometry" (2008), ISBN: 978-3-642-09681-5 (http://dx.doi.org/10.1007/978-3-540-77974-2).
 */

#ifndef __GEO_ALGOS_TRAPEZOID_H__
#define __GEO_ALGOS_TRAPEZOID_H__

#include <vector>
#include <tuple>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <limits>
#include <iostream>
#include <fstream>

#include <boost/geometry.hpp>

#include "lines.h"
#include "tlibs2/libs/maths.h"


namespace geo {

// ----------------------------------------------------------------------------
// trapezoid map
// Reference: (Berg 2008), ch. 6.2, pp. 128-133.
// ----------------------------------------------------------------------------

/**
 * trapezoid defined by a top and bottom line as well as a left and right point
 */
template<class t_vec> requires tl2::is_vec<t_vec>
class Trapezoid
{
public:
	using t_real = typename t_vec::value_type;
	using t_line = std::pair<t_vec, t_vec>;


public:
	Trapezoid() = default;
	~Trapezoid() = default;

	const t_vec& GetLeftPoint() const { return m_pointLeft; }
	const t_vec& GetRightPoint() const { return m_pointRight; }
	t_vec& GetLeftPoint() { return m_pointLeft; }
	t_vec& GetRightPoint() { return m_pointRight; }
	void SetLeftPoint(const t_vec& pt) { m_pointLeft = pt; }
	void SetRightPoint(const t_vec& pt) { m_pointRight = pt; }

	const t_line& GetTopLine() const { return m_lineTop; }
	const t_line& GetBottomLine() const { return m_lineBottom; }
	t_line& GetTopLine() { return m_lineTop; }
	t_line& GetBottomLine() { return m_lineBottom; }

	void SetTopLine(const t_line& line)
	{
		m_lineTop = line;

		// x component of line vertex 1 has to be left of vertex 2
		if(std::get<0>(m_lineTop)[0] > std::get<1>(m_lineTop)[0])
			std::swap(std::get<0>(m_lineTop), std::get<1>(m_lineTop));
	}

	void SetBottomLine(const t_line& line)
	{
		m_lineBottom = line;

		// x component of line vertex 1 has to be left of vertex 2
		if(std::get<0>(m_lineBottom)[0] > std::get<1>(m_lineBottom)[0])
			std::swap(std::get<0>(m_lineBottom), std::get<1>(m_lineBottom));
	}


	/**
	 * is point inside trapezoid (excluding border)?
	 */
	bool Contains(const t_vec& pt) const
	{
		// is point x left of left point x or right of right point x
		if(pt[0] <= m_pointLeft[0] || pt[0] >= m_pointRight[0])
			return false;

		// is point left of top line?
		if(side_of_line<t_vec, t_real>(
			std::get<0>(m_lineTop), std::get<1>(m_lineTop), pt) >= t_real(0))
			return false;

		// is point right of bottom line?
		if(side_of_line<t_vec, t_real>(
			std::get<0>(m_lineBottom), std::get<1>(m_lineBottom), pt) <= t_real(0))
			return false;

		return true;
	}


	/**
	 * trapezoid with no area
	 */
	bool IsEmpty(t_real eps=std::numeric_limits<t_real>::epsilon()) const
	{
		if(tl2::equals<t_real>(m_pointLeft[0], m_pointRight[0], eps))
			return true;
		if(is_line_equal<t_vec>(m_lineTop, m_lineBottom, eps))
			return true;

		return false;
	}


	/**
	 * let the trapezoid be the bounding box of the given points
	 */
	void SetBoundingBox(const std::vector<t_vec>& pts, t_real eps=std::numeric_limits<t_real>::epsilon())
	{
		t_real xmin = std::numeric_limits<t_real>::max();
		t_real xmax = -xmin;
		t_real ymin = xmin;
		t_real ymax = -ymin;

		for(const t_vec& pt : pts)
		{
			xmin = std::min(xmin, pt[0]);
			xmax = std::max(xmax, pt[0]);
			ymin = std::min(ymin, pt[1]);
			ymax = std::max(ymax, pt[1]);
		}

		xmin -= eps; xmax += eps;
		ymin -= eps; ymax += eps;

		m_pointLeft = tl2::create<t_vec>({xmin, ymin});
		m_pointRight = tl2::create<t_vec>({xmax, ymax});

		m_lineBottom = std::make_pair(tl2::create<t_vec>({xmin, ymin}), tl2::create<t_vec>({xmax, ymin}));
		m_lineTop = std::make_pair(tl2::create<t_vec>({xmin, ymax}), tl2::create<t_vec>({xmax, ymax}));
	}


	/**
	 * let the trapezoid be the bounding box of the given line segments
	 */
	void SetBoundingBox(const std::vector<t_line>& lines, t_real eps=std::numeric_limits<t_real>::epsilon())
	{
		std::vector<t_vec> pts;
		pts.reserve(lines.size()*2);

		for(const auto& line : lines)
		{
			pts.push_back(std::get<0>(line));
			pts.push_back(std::get<1>(line));
		}

		SetBoundingBox(pts, eps);
	}


private:
	t_vec m_pointLeft{}, m_pointRight{};
	t_line m_lineTop{}, m_lineBottom{};
};


enum class TrapezoidNodeType
{
	POINT,
	LINE,
	TRAPEZOID,
};


/**
 * trapezoid tree node interface
 */
template<class t_vec> requires tl2::is_vec<t_vec>
class TrapezoidNode
{
public:
	TrapezoidNode() = default;
	virtual ~TrapezoidNode() = default;

	virtual TrapezoidNodeType GetType() const = 0;
	virtual bool IsLeft(const t_vec& vec) const = 0;

	virtual const std::shared_ptr<TrapezoidNode<t_vec>> GetLeft() const = 0;
	virtual const std::shared_ptr<TrapezoidNode<t_vec>> GetRight() const = 0;

	virtual void SetLeft(const std::shared_ptr<TrapezoidNode<t_vec>>& left) = 0;
	virtual void SetRight(const std::shared_ptr<TrapezoidNode<t_vec>>& right) = 0;
};


/**
 * point node speparating space in its x coodinate component
 */
template<class t_vec> requires tl2::is_vec<t_vec>
class TrapezoidNodePoint : public TrapezoidNode<t_vec>
{
public:
	TrapezoidNodePoint(const t_vec& vec = tl2::zero<t_vec>()) : m_vec{vec}
	{}

	TrapezoidNodePoint(
		const std::shared_ptr<TrapezoidNode<t_vec>>& left,
		const std::shared_ptr<TrapezoidNode<t_vec>>& right)
		: m_left{left}, m_right{right}
	{}

	virtual ~TrapezoidNodePoint() = default;

	virtual const std::shared_ptr<TrapezoidNode<t_vec>> GetLeft() const override
	{
		return m_left;
	}

	virtual const std::shared_ptr<TrapezoidNode<t_vec>> GetRight() const override
	{
		return m_right;
	}

	virtual void SetLeft(const std::shared_ptr<TrapezoidNode<t_vec>>& left) override
	{
		m_left = left;
	}

	virtual void SetRight(const std::shared_ptr<TrapezoidNode<t_vec>>& right) override
	{
		m_right = right;
	}

	virtual TrapezoidNodeType GetType() const override
	{
		return TrapezoidNodeType::POINT;
	}

	virtual bool IsLeft(const t_vec& vec) const override
	{
		// is vec left of m_vec?
		return vec[0] <= m_vec[0];
	}


	const t_vec& GetPoint() const { return m_vec; }
	t_vec& GetPoint() { return m_vec; }
	void SetPoint(const t_vec& vec) { m_vec = vec; }


private:
	std::shared_ptr<TrapezoidNode<t_vec>> m_left{}, m_right{};
	t_vec m_vec{};
};


/**
 * line node separating space
 */
template<class t_vec> requires tl2::is_vec<t_vec>
class TrapezoidNodeLine : public TrapezoidNode<t_vec>
{
public:
	using t_real = typename t_vec::value_type;
	using t_line = std::pair<t_vec, t_vec>;


public:
	TrapezoidNodeLine(const t_line& line
		= std::make_pair<t_vec, t_vec>(tl2::zero<t_vec>(), tl2::zero<t_vec>()))
		: m_line{line}
	{}

	TrapezoidNodeLine(
		const std::shared_ptr<TrapezoidNode<t_vec>>& left,
		const std::shared_ptr<TrapezoidNode<t_vec>>& right)
		: m_left{left}, m_right{right}
	{}

	virtual ~TrapezoidNodeLine() = default;


	virtual const std::shared_ptr<TrapezoidNode<t_vec>> GetLeft() const override
	{
		return m_left;
	}

	virtual const std::shared_ptr<TrapezoidNode<t_vec>> GetRight() const override
	{
		return m_right;
	}

	virtual void SetLeft(const std::shared_ptr<TrapezoidNode<t_vec>>& left) override
	{
		m_left = left;
	}

	virtual void SetRight(const std::shared_ptr<TrapezoidNode<t_vec>>& right) override
	{
		m_right = right;
	}

	virtual TrapezoidNodeType GetType() const override
	{
		return TrapezoidNodeType::LINE;
	}

	virtual bool IsLeft(const t_vec& vec) const override
	{
		// is vec left of m_line?
		return side_of_line<t_vec, t_real>(
			std::get<0>(m_line), std::get<1>(m_line), vec) >= t_real(0);
	}


	const t_line& GetLine() const { return m_line; }
	t_line& GetLine() { return m_line; }
	void SetLine(const t_line& line) { m_line = line; }


private:
	std::shared_ptr<TrapezoidNode<t_vec>> m_left{}, m_right{};
	t_line m_line{};
};


/**
 * leaf node pointing to a trapezoid
 */
template<class t_vec> requires tl2::is_vec<t_vec>
class TrapezoidNodeTrapezoid : public TrapezoidNode<t_vec>
{
public:
	TrapezoidNodeTrapezoid(const std::shared_ptr<Trapezoid<t_vec>>& trapezoid = nullptr)
		: m_trapezoid{trapezoid}
	{
	}

	virtual ~TrapezoidNodeTrapezoid() = default;

	virtual TrapezoidNodeType GetType() const override
	{
		return TrapezoidNodeType::TRAPEZOID;
	}

	const std::shared_ptr<Trapezoid<t_vec>> GetTrapezoid() const
	{
		return m_trapezoid;
	}

	void SetTrapezoid(const std::shared_ptr<Trapezoid<t_vec>>& trapezoid)
	{
		return m_trapezoid = trapezoid;
	}


protected:
	virtual const std::shared_ptr<TrapezoidNode<t_vec>> GetLeft() const override
	{ return nullptr; }

	virtual const std::shared_ptr<TrapezoidNode<t_vec>> GetRight() const override
	{ return nullptr; }

	virtual void SetLeft(const std::shared_ptr<TrapezoidNode<t_vec>>&) override
	{ }

	virtual void SetRight(const std::shared_ptr<TrapezoidNode<t_vec>>&) override
	{ }

	virtual bool IsLeft(const t_vec&) const override
	{ return false; }


private:
	std::shared_ptr<Trapezoid<t_vec>> m_trapezoid{};
};


template<class t_vec, class t_line = std::pair<t_vec, t_vec>>
requires tl2::is_vec<t_vec>
std::ostream& operator<<(std::ostream& ostr,
	const std::pair<std::shared_ptr<TrapezoidNode<t_vec>>, int>& node_depth)
{
	const auto& node = *std::get<0>(node_depth);
	int depth = std::get<1>(node_depth);

	auto print_indent = [&ostr, depth]()
	{
		for(int i=0; i<depth; ++i)
			ostr << "  ";
	};

	print_indent();

	switch(node.GetType())
	{
		case TrapezoidNodeType::POINT:
		{
			auto ptnode = dynamic_cast<const TrapezoidNodePoint<t_vec>&>(node);
			const auto& pt = ptnode.GetPoint();
			ostr << "point: " << pt;
			break;
		}

		case TrapezoidNodeType::LINE:
		{
			auto linenode = dynamic_cast<const TrapezoidNodeLine<t_vec>&>(node);
			const auto& line = linenode.GetLine();
			ostr << "line: ";
			print_line<t_vec>(ostr, line);

			break;
		}

		case TrapezoidNodeType::TRAPEZOID:
		{
			auto trnode = dynamic_cast<const TrapezoidNodeTrapezoid<t_vec>&>(node);

			ostr << "trapezoid: ";
			ostr << "left: " << trnode.GetTrapezoid()->GetLeftPoint();
			ostr << ", right: " << trnode.GetTrapezoid()->GetRightPoint();
			ostr << ", bottom: " << trnode.GetTrapezoid()->GetBottomLine();
			ostr << ", top: " << trnode.GetTrapezoid()->GetTopLine();

			break;
		}
	};

	ostr << "\n";

	if(node.GetLeft())
	{
		print_indent();
		ostr << "left node:\n";
		ostr << std::make_pair(node.GetLeft(), depth+1);
	}
	if(node.GetRight())
	{
		print_indent();
		ostr << "right node:\n";
		ostr << std::make_pair(node.GetRight(), depth+1);
	}

	return ostr;
}


/**
 * find a neighbouring trapezoid in the tree
 */
template<class t_vec, class t_line=std::pair<t_vec, t_vec>, 
	class t_real=typename t_vec::value_type>
requires tl2::is_vec<t_vec>
std::shared_ptr<TrapezoidNodeTrapezoid<t_vec>> find_neighbour_trapezoid(
	const std::shared_ptr<TrapezoidNode<t_vec>>& node,
	const std::shared_ptr<Trapezoid<t_vec>>& trap,
	bool left = 1, bool top = 1,
	t_real eps=std::numeric_limits<t_real>::epsilon())
{
	const t_line* lineTop = &trap->GetTopLine();
	const t_line* lineBottom = &trap->GetBottomLine();
	const t_vec* ptLeft = &trap->GetLeftPoint();
	const t_vec* ptRight = &trap->GetRightPoint();

	// the tree is a dag -> avoid writing the same pointer several times
	std::unordered_set<void*> cache;

	// possible neighbours
	std::vector<std::shared_ptr<TrapezoidNodeTrapezoid<t_vec>>> candidates;

	// function to traverse the tree
	std::function<void(const std::shared_ptr<TrapezoidNode<t_vec>>&)> traverse;
	traverse = [&candidates, &traverse, &cache, 
		lineTop, lineBottom, ptLeft, ptRight, left, top, eps]
		(const std::shared_ptr<TrapezoidNode<t_vec>>& node)
	{
		// TODO: better use of binary search tree, no need to check all nodes...
		if(node->GetLeft())
			traverse(node->GetLeft());
		if(node->GetRight())
			traverse(node->GetRight());

		if(node->GetType() == TrapezoidNodeType::TRAPEZOID)
		{
			auto trnode = std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(node);
			auto trap = trnode->GetTrapezoid();

			if(cache.find(trap.get()) == cache.end())
			{
				cache.insert(trap.get());

				const t_line& lineTop2 = trap->GetTopLine();
				const t_line& lineBottom2 = trap->GetBottomLine();
				const t_vec& ptLeft2 = trap->GetLeftPoint();
				const t_vec& ptRight2 = trap->GetRightPoint();

				if(left && top)
				{
					if(is_line_equal<t_vec>(*lineTop, lineTop2, eps)
						&& tl2::equals<t_vec>(*ptLeft, ptRight2, eps))
						candidates.push_back(trnode);
				}
				else if(left && !top)
				{
					if(is_line_equal<t_vec>(*lineBottom, lineBottom2, eps)
						&& tl2::equals<t_vec>(*ptLeft, ptRight2, eps))
						candidates.push_back(trnode);
				}
				else if(!left && top)
				{
					if(is_line_equal<t_vec>(*lineTop, lineTop2, eps)
						&& tl2::equals<t_vec>(*ptRight, ptLeft2, eps))
						candidates.push_back(trnode);
				}
				else if(!left && !top)
				{
					if(is_line_equal<t_vec>(*lineBottom, lineBottom2, eps)
						&& tl2::equals<t_vec>(*ptRight, ptLeft2, eps))
						candidates.push_back(trnode);
				}
			}
		}
	};

	// search the tree
	traverse(node);

	// no neighbours found
	if(candidates.size() == 0)
		return nullptr;

	if(candidates.size() > 1)
	{
		std::cerr << "Warning: more than one neighbour trapezoid found." << std::endl;

		// find maximum / minimum neighbour
		t_real limit_offs = -std::numeric_limits<t_real>::max();
		std::size_t limit_offs_idx = 0;

		for(std::size_t candidate_idx=0; candidate_idx<candidates.size(); ++candidate_idx)
		{
			const auto& candidate = candidates[candidate_idx];
			auto [slope, offs] = 
				get_line_slope_offs<t_vec, t_line>(
					candidate->GetTrapezoid()->GetTopLine());

			if(top && offs > limit_offs)
				limit_offs_idx = candidate_idx;
			else if(!top && offs < limit_offs)
				limit_offs_idx = candidate_idx;
		}

		return candidates[limit_offs_idx];
	}

	return candidates[0];
}


/**
 * find trapezoid containing a given point
 */
template<class t_vec, class t_real=typename t_vec::value_type>
requires tl2::is_vec<t_vec>
std::shared_ptr<TrapezoidNodeTrapezoid<t_vec>> find_trapezoid(
	const std::shared_ptr<TrapezoidNode<t_vec>>& node, const t_vec& pt)
{
	// possible trapezoids
	std::vector<std::shared_ptr<TrapezoidNodeTrapezoid<t_vec>>> candidates;

	// function to traverse the tree
	std::function<void(const std::shared_ptr<TrapezoidNode<t_vec>>&)> traverse;
	traverse = [&candidates, &pt, &traverse]
		(const std::shared_ptr<TrapezoidNode<t_vec>>& node)
	{
		if(node->IsLeft(pt))
		{
			if(node->GetLeft())
				traverse(node->GetLeft());
		}
		else
		{
			if(node->GetRight())
				traverse(node->GetRight());
		}

		if(node->GetType() == TrapezoidNodeType::TRAPEZOID)
		{
			auto trnode = std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(node);
			if(trnode->GetTrapezoid()->Contains(pt))
				candidates.push_back(trnode);
		}
	};

	// search the tree
	traverse(node);

	// no neighbours found
	if(candidates.size() == 0)
		return nullptr;

	if(candidates.size() > 1)
		std::cerr << "Warning: more than one trapezoid found." << std::endl;

	return candidates[0];
}


/**
 * replace old trapezoid node pointers -> new node pointers
 */
template<class t_vec> requires tl2::is_vec<t_vec>
void replace_trapezoid_ptr(std::shared_ptr<TrapezoidNode<t_vec>> node,
	std::shared_ptr<TrapezoidNodeTrapezoid<t_vec>> old_node,
	std::shared_ptr<TrapezoidNode<t_vec>> new_node)
{
	if(!node || !old_node || !new_node)
		return;

	auto old_trap = old_node->GetTrapezoid();
	if(!old_trap)
		return;

	if(node->GetLeft() && node->GetLeft()->GetType()==TrapezoidNodeType::TRAPEZOID)
	{
		auto trnode = std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(
			node->GetLeft());

		if(trnode && trnode->GetTrapezoid() == old_trap)
			node->SetLeft(new_node);
	}
	else
	{
		replace_trapezoid_ptr<t_vec>(node->GetLeft(), old_node, new_node);
	}

	if(node->GetRight() && node->GetRight()->GetType()==TrapezoidNodeType::TRAPEZOID)
	{
		auto trnode = std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(
			node->GetRight());

		if(trnode && trnode->GetTrapezoid() == old_trap)
			node->SetRight(new_node);
	}
	else
	{
		replace_trapezoid_ptr<t_vec>(node->GetRight(), old_node, new_node);
	}
}


/**
 * cut the bottom and top lines to not exceed the left and right point
 */
template<class t_vec, class t_line = std::pair<t_vec, t_vec>, class t_real = typename t_vec::value_type>
requires tl2::is_vec<t_vec>
void fit_trapezoid_lines(std::shared_ptr<TrapezoidNode<t_vec>> node)
{
	if(node->GetLeft())
		fit_trapezoid_lines<t_vec>(node->GetLeft());
	if(node->GetRight())
		fit_trapezoid_lines<t_vec>(node->GetRight());

	if(node->GetType() == TrapezoidNodeType::TRAPEZOID)
	{
		auto trnode = std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(node);
		auto trap = trnode->GetTrapezoid();

		t_real x0 = trap->GetLeftPoint()[0];
		t_real x1 = trap->GetRightPoint()[0];

		t_line lineBottom = trap->GetBottomLine();
		t_real bottom_y0 = get_line_y<t_vec>(lineBottom, x0);
		t_real bottom_y1 = get_line_y<t_vec>(lineBottom, x1);
		std::get<0>(lineBottom)[0] = x0;
		std::get<0>(lineBottom)[1] = bottom_y0;
		std::get<1>(lineBottom)[0] = x1;
		std::get<1>(lineBottom)[1] = bottom_y1;;
		trap->SetBottomLine(lineBottom);

		t_line lineTop = trap->GetTopLine();
		t_real top_y0 = get_line_y<t_vec>(lineTop, x0);
		t_real top_y1 = get_line_y<t_vec>(lineTop, x1);
		std::get<0>(lineTop)[0] = x0;
		std::get<0>(lineTop)[1] = top_y0;
		std::get<1>(lineTop)[0] = x1;
		std::get<1>(lineTop)[1] = top_y1;
		trap->SetTopLine(lineTop);
	}
}


/**
 * transform all points, lines and trapezoids in the tree
 */
template<class t_vec, class t_mat>
requires tl2::is_vec<t_vec> && tl2::is_mat<t_mat>
void trafo_trapezoid_tree(std::shared_ptr<TrapezoidNode<t_vec>> node, const t_mat& mat,
	std::shared_ptr<std::unordered_set<void*>> cache=nullptr)
{
	// prevent pointers in the tree to be transformed multiple times
	if(!cache)
		cache = std::make_shared<std::unordered_set<void*>>();

	if(node->GetType() == TrapezoidNodeType::POINT)
	{
		auto ptnode = std::dynamic_pointer_cast<TrapezoidNodePoint<t_vec>>(node);

		if(cache->find(ptnode.get()) == cache->end())
		{
			ptnode->SetPoint(mat * ptnode->GetPoint());
			cache->insert(ptnode.get());
		}
	}
	else if(node->GetType() == TrapezoidNodeType::LINE)
	{
		auto lnnode = std::dynamic_pointer_cast<TrapezoidNodeLine<t_vec>>(node);

		if(cache->find(lnnode.get()) == cache->end())
		{
			std::get<0>(lnnode->GetLine()) = mat * std::get<0>(lnnode->GetLine());
			std::get<1>(lnnode->GetLine()) = mat * std::get<1>(lnnode->GetLine());

			cache->insert(lnnode.get());
		}
	}
	else if(node->GetType() == TrapezoidNodeType::TRAPEZOID)
	{
		auto trnode = std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(node);
		auto trap = trnode->GetTrapezoid();

		if(cache->find(trap.get()) == cache->end())
		{
			trap->SetLeftPoint(mat * trap->GetLeftPoint());
			trap->SetRightPoint(mat * trap->GetRightPoint());
			std::get<0>(trap->GetTopLine()) = mat * std::get<0>(trap->GetTopLine());
			std::get<1>(trap->GetTopLine()) = mat * std::get<1>(trap->GetTopLine());
			std::get<0>(trap->GetBottomLine()) = mat * std::get<0>(trap->GetBottomLine());
			std::get<1>(trap->GetBottomLine()) = mat * std::get<1>(trap->GetBottomLine());

			cache->insert(trap.get());
		}
	}

	if(node->GetLeft())
		trafo_trapezoid_tree<t_vec, t_mat>(node->GetLeft(), mat, cache);
	if(node->GetRight())
		trafo_trapezoid_tree<t_vec, t_mat>(node->GetRight(), mat, cache);
}


/**
 * save the trapezoid tree as an svg
 */
template<class t_vec, class t_line = std::pair<t_vec, t_vec>, class t_real = typename t_vec::value_type>
requires tl2::is_vec<t_vec>
void save_trapezoid_svg(const std::shared_ptr<TrapezoidNode<t_vec>>& node,
	const std::string& file, const std::vector<t_line>* lines = nullptr)
{
	namespace geo = boost::geometry;
	using t_geovertex = geo::model::point<t_real, 2, geo::cs::cartesian>;
	using t_geopoly = geo::model::polygon<t_geovertex, true /*cw*/, false /*closed*/>;
	using t_geoline = geo::model::linestring<t_geovertex>;
	using t_geosvg = geo::svg_mapper<t_geovertex>;

	// the tree is a dag -> avoid writing the same pointer several times
	std::unordered_set<void*> cache;

	std::ofstream ofstr{file};
	t_geosvg svg{ofstr, 100, 100, "width = \"500px\" height = \"500px\""};

	// function to traverse the tree
	std::function<void(const std::shared_ptr<TrapezoidNode<t_vec>>&)> traverse;
	traverse = [&svg, &traverse, &cache]
	(const std::shared_ptr<TrapezoidNode<t_vec>>& node)
	{
		if(node->GetLeft())
			traverse(node->GetLeft());
		if(node->GetRight())
			traverse(node->GetRight());

		if(node->GetType() == TrapezoidNodeType::TRAPEZOID)
		{
			auto trnode = std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(node);
			auto trap = trnode->GetTrapezoid();
			if(cache.find(trap.get()) == cache.end())
			{
				cache.insert(trap.get());

				auto lineTop = trap->GetTopLine();
				auto lineBottom = trap->GetBottomLine();

				t_geopoly poly;
				poly.outer().push_back(t_geovertex{std::get<0>(lineTop)[0], std::get<0>(lineTop)[1]});
				poly.outer().push_back(t_geovertex{std::get<0>(lineBottom)[0], std::get<0>(lineBottom)[1]});
				poly.outer().push_back(t_geovertex{std::get<1>(lineBottom)[0], std::get<1>(lineBottom)[1]});
				poly.outer().push_back(t_geovertex{std::get<1>(lineTop)[0], std::get<1>(lineTop)[1]});

				svg.add(poly);
				svg.map(poly, "stroke: #000000; stroke-width: 1px; fill: none;", 1.);
			}
		}
	};

	traverse(node);

	if(lines)
	{
		for(const auto& line : *lines)
		{
			t_geoline theline;
			theline.push_back(t_geovertex{std::get<0>(line)[0], std::get<0>(line)[1]});
			theline.push_back(t_geovertex{std::get<1>(line)[0], std::get<1>(line)[1]});

			svg.add(theline);
			svg.map(theline, "stroke: #ff0000; stroke-width: 2px; fill: none;", 1.);
		}
	}

	ofstr.flush();
}


/**
 * create polygons from the trapezoid tree
 */
template<class t_vec, class t_line = std::pair<t_vec, t_vec>,
	class t_real = typename t_vec::value_type>
requires tl2::is_vec<t_vec>
std::vector<std::vector<t_vec>>
get_trapezoids(const std::shared_ptr<TrapezoidNode<t_vec>>& node)
{
	std::vector<std::vector<t_vec>> polys;

	// the tree is a dag -> avoid accessing the same trapezoid pointer several times
	std::unordered_set<void*> cache;

	// function to traverse the tree
	std::function<void(const std::shared_ptr<TrapezoidNode<t_vec>>&)> traverse;
	traverse = [&polys, &traverse, &cache]
	(const std::shared_ptr<TrapezoidNode<t_vec>>& node)
	{
		if(node->GetLeft())
			traverse(node->GetLeft());
		if(node->GetRight())
			traverse(node->GetRight());

		if(node->GetType() == TrapezoidNodeType::TRAPEZOID)
		{
			auto trnode = std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(node);
			auto trap = trnode->GetTrapezoid();
			if(cache.find(trap.get()) == cache.end())
			{
				cache.insert(trap.get());

				auto lineTop = trap->GetTopLine();
				auto lineBottom = trap->GetBottomLine();

				std::vector<t_vec> poly;
				poly.push_back(std::get<0>(lineBottom));
				poly.push_back(std::get<1>(lineBottom));
				poly.push_back(std::get<1>(lineTop));
				poly.push_back(std::get<0>(lineTop));

				polys.emplace_back(std::move(poly));
			}
		}
	};

	traverse(node);
	return polys;
}


/**
 * check if the line segments share the same x coordinates
 */
template<class t_vec, class t_line = std::pair<t_vec, t_vec>,
	class t_real = typename t_vec::value_type>
requires tl2::is_vec<t_vec>
bool check_line_equal_x(const std::vector<t_line>& lines,
	bool exclude_duplicates = true,  // exclude coinciding points
	t_real eps=std::numeric_limits<t_real>::epsilon())
{
	struct CompareReals
	{
		CompareReals() = delete;
		CompareReals(t_real eps) : m_eps{eps}
		{}

		bool operator()(t_real x, t_real y) const
		{
			if(tl2::equals<t_real>(x, y, m_eps))
				return false;
			return x < y;
		}

		t_real m_eps = std::numeric_limits<t_real>::epsilon();
	};

	std::vector<t_vec> alreadyseen0, alreadyseen1;
	std::set<t_real, CompareReals> cache_x{CompareReals{eps}};

	for(const t_line& line : lines)
	{
		const t_vec& pt0 = std::get<0>(line);
		const t_vec& pt1 = std::get<1>(line);

		if(exclude_duplicates)
		{
			if(std::find_if(alreadyseen0.begin(), alreadyseen0.end(),
				[&pt0, eps](const t_vec& pt) -> bool
				{
					return tl2::equals<t_vec>(pt0, pt, eps);
				}) != alreadyseen0.end())
				continue;

			if(std::find_if(alreadyseen1.begin(), alreadyseen1.end(),
				[&pt1, eps](const t_vec& pt) -> bool
				{
					return tl2::equals<t_vec>(pt1, pt, eps);
				}) != alreadyseen1.end())
				continue;
		}

		t_real x0 = pt0[0];
		if(cache_x.find(x0) != cache_x.end())
			return true;
		cache_x.insert(x0);

		t_real x1 = pt1[0];
		if(cache_x.find(x1) != cache_x.end())
			return true;
		cache_x.insert(x1);

		if(exclude_duplicates)
		{
			alreadyseen0.push_back(pt0);
			alreadyseen1.push_back(pt1);
		}
	}

	return false;
}


/**
 * try to unite two adjacent trapezoids
 */
template<class t_vec, class t_real = typename t_vec::value_type>
requires tl2::is_vec<t_vec>
bool try_unite_trapezoids(
	std::shared_ptr<Trapezoid<t_vec>>* trap1, // left trapezoid
	std::shared_ptr<Trapezoid<t_vec>>* trap2, // right trapezoid
	t_real eps=std::numeric_limits<t_real>::epsilon())
{
	if(!*trap1 || !*trap2)
		return false;
	if(*trap1 == *trap2)
		return false;

	if((*trap1)->GetLeftPoint()[0] > (*trap2)->GetLeftPoint()[0])
		std::swap(trap1, trap2);

	if(!is_line_equal<t_vec>((*trap1)->GetTopLine(), (*trap2)->GetTopLine(), eps))
		return false;
	if(!is_line_equal<t_vec>((*trap1)->GetBottomLine(), (*trap2)->GetBottomLine(), eps))
		return false;

	if(!tl2::equals<t_real>((*trap1)->GetRightPoint()[0], (*trap2)->GetLeftPoint()[0], eps))
		return false;

	(*trap1)->SetRightPoint((*trap2)->GetRightPoint());
	*trap2 = *trap1;

	return true;
}


/**
 * remove empty nodes
 * @return can node be deleted?
 */
template<class t_vec, class t_real = typename t_vec::value_type>
requires tl2::is_vec<t_vec>
bool clean_trapezoid_tree(std::shared_ptr<TrapezoidNode<t_vec>> node)
{
	if(!node)
		return true;

	// don't remove trapezoid nodes
	if(node->GetType() == TrapezoidNodeType::TRAPEZOID)
		return false;

	// delete point or line nodes with no children
	if(!node->GetLeft() && !node->GetRight())
		return true;

	if(clean_trapezoid_tree<t_vec>(node->GetLeft()))
		node->SetLeft(nullptr);
	if(clean_trapezoid_tree<t_vec>(node->GetRight()))
		node->SetRight(nullptr);

	return !node->GetLeft() && !node->GetRight();
}


/**
 * create a trapezoid tree
 * @see (Berg 2008), pp. 128-133 and pp. 137-139
 */
template<class t_vec, class t_line = std::pair<t_vec, t_vec>, class t_real = typename t_vec::value_type>
requires tl2::is_vec<t_vec>
std::shared_ptr<TrapezoidNode<t_vec>>
create_trapezoid_tree(const std::vector<t_line>& _lines,
	bool randomise=true, bool shear=true,
	t_real padding=1., t_real eps=1e-5)
{
	using t_mat = tl2::mat<t_real, std::vector>;
	std::vector<t_line> lines = _lines;

	// order line vertices by x
	for(t_line& line : lines)
	{
		if(std::get<0>(line)[0] > std::get<1>(line)[0])
			std::swap(std::get<0>(line), std::get<1>(line));
	}


	t_real shear_eps = eps * 10.;
	if(shear)
	{
		// shear until no more x coordinates coincide
		t_real shear_mult = 1;
		while(check_line_equal_x<t_vec>(lines, true, eps))
		{
			t_mat shear = tl2::shear<t_mat>(2, 2, 0, 1, shear_eps);

			for(auto& line : lines)
			{
				std::get<0>(line) = shear*std::get<0>(line);
				std::get<1>(line) = shear*std::get<1>(line);
			}

			++shear_mult;
		}
		shear_eps *= shear_mult;
	}


	auto box = std::make_shared<Trapezoid<t_vec>>();
	box->SetBoundingBox(lines, padding);

	std::shared_ptr<TrapezoidNode<t_vec>> root =
		std::make_shared<TrapezoidNodeTrapezoid<t_vec>>(box);

	if(randomise)
		std::shuffle(lines.begin(), lines.end(), std::mt19937{std::random_device{}()});


	// add lines to the tree
	for(std::size_t lineidx=0; lineidx<lines.size(); ++lineidx)
	{
		const auto& line = lines[lineidx];
		std::vector<std::shared_ptr<TrapezoidNodeTrapezoid<t_vec>>> intersecting_trapezoids;

		const t_vec& leftpt = std::get<0>(line);
		const t_vec& rightpt = std::get<1>(line);

		if(auto trap_node = find_trapezoid<t_vec>(root, leftpt); trap_node)
			intersecting_trapezoids.push_back(trap_node);

		if(intersecting_trapezoids.size() == 0)
			continue;

		auto cur_trap = intersecting_trapezoids[0];
		while(cur_trap)
		{
			const auto& trap_rightpt = cur_trap->GetTrapezoid()->GetRightPoint();
			if(rightpt[0] <= trap_rightpt[0])
				break;

			if(side_of_line<t_vec, t_real>(leftpt, rightpt, trap_rightpt) >= t_real(0))
				cur_trap = find_neighbour_trapezoid<t_vec>(root, cur_trap->GetTrapezoid(), 0, 0, eps);
			else
				cur_trap = find_neighbour_trapezoid<t_vec>(root, cur_trap->GetTrapezoid(), 0, 1, eps);

			if(cur_trap)
				intersecting_trapezoids.push_back(cur_trap);
		}


		auto create_trapnode = [](std::shared_ptr<Trapezoid<t_vec>> trap)
			-> std::shared_ptr<TrapezoidNodeTrapezoid<t_vec>>
		{
			return std::make_shared<TrapezoidNodeTrapezoid<t_vec>>(trap);
		};

		if(intersecting_trapezoids.size() == 0)
		{
			// no intersections
			break;
		}
		else if(intersecting_trapezoids.size() == 1)
		{
			auto cur_trap_node = intersecting_trapezoids[0];
			auto cur_trap = cur_trap_node->GetTrapezoid();

			auto trap_left = std::make_shared<Trapezoid<t_vec>>();
			trap_left->SetLeftPoint(cur_trap->GetLeftPoint());
			trap_left->SetRightPoint(leftpt);
			trap_left->SetTopLine(cur_trap->GetTopLine());
			trap_left->SetBottomLine(cur_trap->GetBottomLine());

			auto trap_right = std::make_shared<Trapezoid<t_vec>>();
			trap_right->SetLeftPoint(rightpt);
			trap_right->SetRightPoint(cur_trap->GetRightPoint());
			trap_right->SetTopLine(cur_trap->GetTopLine());
			trap_right->SetBottomLine(cur_trap->GetBottomLine());

			auto trap_top = std::make_shared<Trapezoid<t_vec>>();
			trap_top->SetLeftPoint(leftpt);
			trap_top->SetRightPoint(rightpt);
			trap_top->SetTopLine(cur_trap->GetTopLine());
			trap_top->SetBottomLine(line);

			auto trap_bottom = std::make_shared<Trapezoid<t_vec>>();
			trap_bottom->SetLeftPoint(leftpt);
			trap_bottom->SetRightPoint(rightpt);
			trap_bottom->SetTopLine(line);
			trap_bottom->SetBottomLine(cur_trap->GetBottomLine());

			auto trap_left_node = create_trapnode(trap_left);
			auto trap_right_node = create_trapnode(trap_right);
			auto trap_top_node = create_trapnode(trap_top);
			auto trap_bottom_node = create_trapnode(trap_bottom);

			auto line_node = std::make_shared<TrapezoidNodeLine<t_vec>>(line);
			if(!trap_top->IsEmpty(eps))
				line_node->SetLeft(trap_top_node);
			if(!trap_bottom->IsEmpty(eps))
				line_node->SetRight(trap_bottom_node);

			auto rightpt_node = std::make_shared<TrapezoidNodePoint<t_vec>>(rightpt);
			rightpt_node->SetLeft(line_node);
			if(!trap_right->IsEmpty(eps))
			rightpt_node->SetRight(trap_right_node);

			auto leftpt_node = std::make_shared<TrapezoidNodePoint<t_vec>>(leftpt);
			leftpt_node->SetLeft(trap_left_node);
			leftpt_node->SetRight(rightpt_node);

			fit_trapezoid_lines<t_vec>(leftpt_node);

			if(root->GetType() == TrapezoidNodeType::TRAPEZOID &&
				cur_trap == std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(root)->GetTrapezoid())
			{
				// replace root node
				root = leftpt_node;
			}
			else
			{
				// replace child node pointers
				replace_trapezoid_ptr<t_vec>(root, cur_trap_node, leftpt_node);
			}
		}
		else if(intersecting_trapezoids.size() > 1)
		{
			// first trapezoid
			auto first_trap_node = *intersecting_trapezoids.begin();
			auto first_trap = first_trap_node->GetTrapezoid();

			auto first_left = std::make_shared<Trapezoid<t_vec>>();
			first_left->SetLeftPoint(first_trap->GetLeftPoint());
			first_left->SetRightPoint(leftpt);
			first_left->SetTopLine(first_trap->GetTopLine());
			first_left->SetBottomLine(first_trap->GetBottomLine());

			auto first_top = std::make_shared<Trapezoid<t_vec>>();
			first_top->SetLeftPoint(leftpt);
			first_top->SetRightPoint(first_trap->GetRightPoint());
			first_top->SetTopLine(first_trap->GetTopLine());
			first_top->SetBottomLine(line);

			auto first_bottom = std::make_shared<Trapezoid<t_vec>>();
			first_bottom->SetLeftPoint(leftpt);
			first_bottom->SetRightPoint(first_trap->GetRightPoint());
			first_bottom->SetTopLine(line);
			first_bottom->SetBottomLine(first_trap->GetBottomLine());


			// last trapezoid
			auto last_trap_node = *intersecting_trapezoids.rbegin();
			auto last_trap = last_trap_node->GetTrapezoid();

			auto last_right = std::make_shared<Trapezoid<t_vec>>();
			last_right->SetLeftPoint(rightpt);
			last_right->SetRightPoint(last_trap->GetRightPoint());
			last_right->SetTopLine(last_trap->GetTopLine());
			last_right->SetBottomLine(last_trap->GetBottomLine());

			auto last_top = std::make_shared<Trapezoid<t_vec>>();
			last_top->SetLeftPoint(last_trap->GetLeftPoint());
			last_top->SetRightPoint(rightpt);
			last_top->SetTopLine(last_trap->GetTopLine());
			last_top->SetBottomLine(line);

			auto last_bottom = std::make_shared<Trapezoid<t_vec>>();
			last_bottom->SetLeftPoint(last_trap->GetLeftPoint());
			last_bottom->SetRightPoint(rightpt);
			last_bottom->SetTopLine(line);
			last_bottom->SetBottomLine(last_trap->GetBottomLine());


			// mid trapezoids
			std::vector<std::shared_ptr<Trapezoid<t_vec>>> mid_tops, mid_bottoms;
			std::vector<std::shared_ptr<TrapezoidNodeTrapezoid<t_vec>>> mid_trap_nodes;

			for(std::size_t isect_idx=1; isect_idx<intersecting_trapezoids.size()-1; ++isect_idx)
			{
				auto mid_trap_node = intersecting_trapezoids[isect_idx];
				auto mid_trap = mid_trap_node->GetTrapezoid();

				auto mid_top = std::make_shared<Trapezoid<t_vec>>();
				mid_top->SetLeftPoint(mid_trap->GetLeftPoint());
				mid_top->SetRightPoint(mid_trap->GetRightPoint());
				mid_top->SetTopLine(mid_trap->GetTopLine());
				mid_top->SetBottomLine(line);

				auto mid_bottom = std::make_shared<Trapezoid<t_vec>>();
				mid_bottom->SetLeftPoint(mid_trap->GetLeftPoint());
				mid_bottom->SetRightPoint(mid_trap->GetRightPoint());
				mid_bottom->SetTopLine(line);
				mid_bottom->SetBottomLine(mid_trap->GetBottomLine());

				if(isect_idx > 1)
				{
					try_unite_trapezoids(&*mid_tops.rbegin(), &mid_top, eps);
					try_unite_trapezoids(&*mid_bottoms.rbegin(), &mid_bottom , eps);
					try_unite_trapezoids(&*mid_tops.rbegin(), &mid_bottom, eps);
					try_unite_trapezoids(&*mid_bottoms.rbegin(), &mid_top , eps);
				}
				if(isect_idx == 1)
				{
					try_unite_trapezoids(&first_top, &mid_top, eps);
					try_unite_trapezoids(&first_bottom, &mid_bottom, eps);
					try_unite_trapezoids(&first_top, &mid_bottom, eps);
					try_unite_trapezoids(&first_bottom, &mid_top, eps);
				}
				if(isect_idx == intersecting_trapezoids.size()-2)
				{
					try_unite_trapezoids(&last_top, &mid_top, eps);
					try_unite_trapezoids(&last_bottom, &mid_bottom, eps);
					try_unite_trapezoids(&last_top, &mid_bottom, eps);
					try_unite_trapezoids(&last_bottom, &mid_top, eps);
				}

				mid_tops.push_back(mid_top);
				mid_bottoms.push_back(mid_bottom);
				mid_trap_nodes.push_back(mid_trap_node);
			}

			try_unite_trapezoids(&first_top, &last_top, eps);
			try_unite_trapezoids(&first_bottom, &last_bottom, eps);
			try_unite_trapezoids(&first_top, &last_bottom, eps);
			try_unite_trapezoids(&first_bottom, &last_top, eps);


			// first trapezoid
			auto first_left_node = create_trapnode(first_left);
			auto first_top_node = create_trapnode(first_top);
			auto first_bottom_node = create_trapnode(first_bottom);

			auto first_line_node = std::make_shared<TrapezoidNodeLine<t_vec>>(line);
			if(!first_top->IsEmpty(eps))
				first_line_node->SetLeft(first_top_node);
			if(!first_bottom->IsEmpty(eps))
				first_line_node->SetRight(first_bottom_node);

			auto first_leftpt_node = std::make_shared<TrapezoidNodePoint<t_vec>>(leftpt);
			if(!first_left->IsEmpty(eps))
				first_leftpt_node->SetLeft(first_left_node);
			first_leftpt_node->SetRight(first_line_node);

			fit_trapezoid_lines<t_vec>(first_leftpt_node);

			if(root->GetType() == TrapezoidNodeType::TRAPEZOID &&
				first_trap == std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(root)->GetTrapezoid())
			{
				// replace root node
				root = first_leftpt_node;
			}
			else
			{
				// replace child node pointers
				replace_trapezoid_ptr<t_vec>(root, first_trap_node, first_leftpt_node);
			}


			// mid trapezoids
			for(std::size_t i=0; i<mid_trap_nodes.size(); ++i)
			{
				auto mid_top = mid_tops[i];
				auto mid_bottom = mid_bottoms[i];
				auto mid_trap_node = mid_trap_nodes[i];
				auto mid_trap = mid_trap_node->GetTrapezoid();

				auto mid_top_node = create_trapnode(mid_top);
				auto mid_bottom_node = create_trapnode(mid_bottom);

				auto mid_line_node = std::make_shared<TrapezoidNodeLine<t_vec>>(line);
				if(!mid_top->IsEmpty(eps))
					mid_line_node->SetLeft(mid_top_node);
				if(!mid_bottom->IsEmpty(eps))
					mid_line_node->SetRight(mid_bottom_node);

				fit_trapezoid_lines<t_vec>(mid_line_node);

				if(root->GetType() == TrapezoidNodeType::TRAPEZOID &&
					mid_trap == std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(root)->GetTrapezoid())
				{
					// replace root node
					root = mid_line_node;
				}
				else
				{
					// replace child node pointers
					replace_trapezoid_ptr<t_vec>(root, mid_trap_node, mid_line_node);
				}
			}


			// last trapezoid
			auto last_right_node = create_trapnode(last_right);
			auto last_top_node = create_trapnode(last_top);
			auto last_bottom_node = create_trapnode(last_bottom);

			auto last_line_node = std::make_shared<TrapezoidNodeLine<t_vec>>(line);
			if(!last_top->IsEmpty(eps))
				last_line_node->SetLeft(last_top_node);
			if(!last_bottom->IsEmpty(eps))
				last_line_node->SetRight(last_bottom_node);

			auto last_rightpt_node = std::make_shared<TrapezoidNodePoint<t_vec>>(rightpt);
			last_rightpt_node->SetLeft(last_line_node);
			if(!last_right->IsEmpty(eps))
				last_rightpt_node->SetRight(last_right_node);

			fit_trapezoid_lines<t_vec>(last_rightpt_node);

			if(root->GetType() == TrapezoidNodeType::TRAPEZOID &&
				last_trap == std::dynamic_pointer_cast<TrapezoidNodeTrapezoid<t_vec>>(root)->GetTrapezoid())
			{
				// replace root node
				root = last_rightpt_node;
			}
			else
			{
				// replace child node pointers
				replace_trapezoid_ptr<t_vec>(root, last_trap_node, last_rightpt_node);
			}
		}
	}

	if(shear)
	{
		// TODO: remove tilt
		t_mat shear_inv = tl2::shear<t_mat>(2, 2, 0, 1, -shear_eps);
		trafo_trapezoid_tree<t_vec, t_mat>(root, shear_inv);
	}

	clean_trapezoid_tree<t_vec>(root);
	return root;
}
// ----------------------------------------------------------------------------

}
#endif
