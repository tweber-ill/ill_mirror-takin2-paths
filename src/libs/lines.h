/**
 * geometric calculations, line segment intersections
 * @author Tobias Weber <tweber@ill.fr>
 * @date Oct/Nov-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 *
 * References for the algorithms:
 *   - (Klein 2005) "Algorithmische Geometrie" (2005), ISBN: 978-3540209560 (http://dx.doi.org/10.1007/3-540-27619-X).
 *   - (FUH 2020) "Algorithmische Geometrie" (2020), Kurs 1840, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/1840).
 *   - (Berg 2008) "Computational Geometry" (2008), ISBN: 978-3-642-09681-5 (http://dx.doi.org/10.1007/978-3-540-77974-2).
 */

#ifndef __GEO_ALGOS_LINES_H__
#define __GEO_ALGOS_LINES_H__

#include <vector>
#include <queue>
#include <tuple>
#include <algorithm>
#include <limits>
#include <iostream>

#include <boost/intrusive/avltree.hpp>

#include "tlibs2/libs/maths.h"


namespace geo {

// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

/**
 * calculate circumcentre
 * @see https://de.wikipedia.org/wiki/Umkreis
 */
template<class t_vec> requires tl2::is_vec<t_vec>
t_vec calc_circumcentre(const std::vector<t_vec>& triag)
{
	using namespace tl2_ops;
	using t_real = typename t_vec::value_type;

	if(triag.size() < 3)
		return t_vec{};

	const t_vec& v0 = triag[0];
	const t_vec& v1 = triag[1];
	const t_vec& v2 = triag[2];

	// formula, see: https://de.wikipedia.org/wiki/Umkreis
	const t_real x =
		(v0[0]*v0[0]+v0[1]*v0[1]) * (v1[1]-v2[1]) +
		(v1[0]*v1[0]+v1[1]*v1[1]) * (v2[1]-v0[1]) +
		(v2[0]*v2[0]+v2[1]*v2[1]) * (v0[1]-v1[1]);

	const t_real y =
		(v0[0]*v0[0]+v0[1]*v0[1]) * (v2[0]-v1[0]) +
		(v1[0]*v1[0]+v1[1]*v1[1]) * (v0[0]-v2[0]) +
		(v2[0]*v2[0]+v2[1]*v2[1]) * (v1[0]-v0[0]);

	const t_real n =
		t_real{2}*v0[0] * (v1[1]-v2[1]) +
		t_real{2}*v1[0] * (v2[1]-v0[1]) +
		t_real{2}*v2[0] * (v0[1]-v1[1]);

	return tl2::create<t_vec>({x/n, y/n});
}


template<class t_vec, class t_real=typename t_vec::value_type>
t_real line_angle(const t_vec& pt1, const t_vec& pt2)
requires tl2::is_vec<t_vec>
{
	t_vec dir = pt2 - pt1;
	return std::atan2(dir[1], dir[0]);
}


template<class t_vec, class t_real=typename t_vec::value_type>
t_real line_angle(const t_vec& line1vert1, const t_vec& line1vert2,
	const t_vec& line2vert1, const t_vec& line2vert2)
requires tl2::is_vec<t_vec>
{
	return line_angle<t_vec, t_real>(line2vert1, line2vert2)
		- line_angle<t_vec, t_real>(line1vert1, line1vert2);
}


template<class t_vec> requires tl2::is_vec<t_vec>
std::pair<bool, t_vec> intersect_lines(
	const t_vec& pos1a, const t_vec& pos1b,
	const t_vec& pos2a, const t_vec& pos2b, bool only_segments=true)
{
	t_vec dir1 = pos1b - pos1a;
	t_vec dir2 = pos2b - pos2a;

	auto[pt1, pt2, valid, dist, param1, param2] =
		tl2::intersect_line_line(pos1a, dir1, pos2a, dir2);

	if(!valid)
		return std::make_pair(false, tl2::create<t_vec>({}));

	if(only_segments && (param1<0. || param1>1. || param2<0. || param2>1.))
		return std::make_pair(false, tl2::create<t_vec>({}));

	return std::make_pair(true, pt1);
}


template<class t_vec, class t_line=std::pair<t_vec, t_vec>, 
	class t_real=typename t_vec::value_type>
requires tl2::is_vec<t_vec>
std::pair<t_real, t_real> get_line_slope_offs(const t_line& line)
{
	t_real slope = (std::get<1>(line)[1] - std::get<0>(line)[1])
		/ (std::get<1>(line)[0] - std::get<0>(line)[0]);
	t_real offs = std::get<0>(line)[1] - std::get<0>(line)[0]*slope;

	return std::make_pair(slope, offs);
}


template<class t_vec, class t_line=std::pair<t_vec, t_vec>, 
	class t_real=typename t_vec::value_type>
requires tl2::is_vec<t_vec>
t_real get_line_y(const t_line& line, t_real x)
{
	auto [slope, offs] = get_line_slope_offs<t_vec>(line);
	return slope*x + offs;
}


/**
 * are two lines equal?
 */
template<class t_vec, class t_line=std::pair<t_vec, t_vec>, 
	class t_real=typename t_vec::value_type>
requires tl2::is_vec<t_vec>
bool is_line_equal(const t_line& line1, const t_line& line2,
	t_real eps=std::numeric_limits<t_real>::epsilon())
{
	auto [slope1, offs1] = get_line_slope_offs<t_vec>(line1);
	auto [slope2, offs2] = get_line_slope_offs<t_vec>(line2);

	return tl2::equals<t_real>(slope1, slope2, eps) 
		&& tl2::equals<t_real>(offs1, offs2, eps);
}


template<class t_line /*= std::pair<t_vec, t_vec>*/>
std::tuple<bool, typename t_line::first_type> intersect_lines(
	const t_line& line1, const t_line& line2)
requires tl2::is_vec<typename t_line::first_type> 
	&& tl2::is_vec<typename t_line::second_type>
{
	return intersect_lines<typename t_line::first_type>(
		line1.first, line1.second, line2.first, line2.second, true);
}


/**
 * returns > 0 if point is on the left-hand side of line
 */
template<class t_vec, class t_real = typename t_vec::value_type>
t_real side_of_line(const t_vec& vec1a, const t_vec& vec1b, const t_vec& pt)
requires tl2::is_vec<t_vec>
{
	using namespace tl2_ops;

	t_vec dir1 = vec1b - vec1a;
	t_vec dir2 = pt - vec1a;

	return dir1[0]*dir2[1] - dir1[1]*dir2[0];
}


template<class t_vec> requires tl2::is_vec<t_vec>
bool pt_inside_hull(const std::vector<t_vec>& hull, const t_vec& pt)
{
	// iterate vertices
	for(std::size_t idx1=0; idx1<hull.size(); ++idx1)
	{
		std::size_t idx2 = idx1+1;
		if(idx2 >= hull.size())
			idx2 = 0;

		const t_vec& vert1 = hull[idx1];
		const t_vec& vert2 = hull[idx2];

		// outside?
		if(side_of_line<t_vec>(vert1, vert2, pt) < 0.)
			return false;
	}

	return true;
}


/**
 * get barycentric coordinates of a point
 * see https://en.wikipedia.org/wiki/Barycentric_coordinate_system
 */
template<class t_vec> requires tl2::is_vec<t_vec>
std::optional<t_vec> get_barycentric(
	const t_vec& tri1, const t_vec& tri2, const t_vec& tri3, const t_vec& pt)
{
	using t_real = typename t_vec::value_type;
	using t_mat = tl2::mat<t_real, std::vector>;

	t_mat trafo = tl2::create<t_mat, t_vec>({tri1-tri3, tri2-tri3});
	auto [inv_trafo, ok] = tl2::inv<t_mat>(trafo);
	if(!ok)
		return std::nullopt;

	t_vec vecBary = inv_trafo * (pt-tri3);
	return vecBary;
}


template<class t_vec> requires tl2::is_vec<t_vec>
bool pt_inside_triag(
	const t_vec& tri1, const t_vec& tri2, const t_vec& tri3, const t_vec& pt)
{
	using t_real = typename t_vec::value_type;
	auto vecBary = get_barycentric<t_vec>(tri1, tri2, tri3, pt);
	if(!vecBary)
		return false;

	t_real x = (*vecBary)[0];
	t_real y = (*vecBary)[1];
	t_real z = t_real{1} - x - y;

	return x>=t_real{0} && x<t_real{1} &&
		y>=t_real{0} && y<t_real{1} &&
		z>=t_real{0} && z<t_real{1};
}


/**
 * get triangle containing point pt
 */
template<class t_vec> requires tl2::is_vec<t_vec>
std::optional<std::size_t> get_containing_triag(
	const std::vector<std::vector<t_vec>>& triags, const t_vec& pt)
{
	for(std::size_t idx=0; idx<triags.size(); ++idx)
	{
		const auto& triag = triags[idx];
		if(pt_inside_triag<t_vec>(triag[0], triag[1], triag[2], pt))
			return idx;
	}

	return std::nullopt;
}


template<class t_vec, class t_real = typename t_vec::value_type>
std::vector<t_vec> _remove_duplicates(
	const std::vector<t_vec>& _verts, 
	t_real eps=std::numeric_limits<t_real>::epsilon())
requires tl2::is_vec<t_vec>
{
	std::vector<t_vec> verts = _verts;

	// remove duplicate points
	verts.erase(std::unique(verts.begin(), verts.end(),
		[eps](const t_vec& vec1, const t_vec& vec2) -> bool
		{ return tl2::equals<t_vec>(vec1, vec2, eps); }
		), verts.end());

	return verts;
}


template<class t_vec, class t_real = typename t_vec::value_type>
std::vector<t_vec> _sort_vertices(
	const std::vector<t_vec>& _verts, 
	t_real eps=std::numeric_limits<t_real>::epsilon())
requires tl2::is_vec<t_vec>
{
	std::vector<t_vec> verts = _verts;

	std::stable_sort(verts.begin(), verts.end(), 
		[eps](const t_vec& vert1, const t_vec& vert2) -> bool
		{
			if(tl2::equals<t_real>(vert1[0], vert2[0], eps))
				return vert1[1] < vert2[1];
			return vert1[0] < vert2[0];
		});

	// remove unnecessary points
	auto iterCurX = verts.begin();
	for(auto iter = iterCurX; iter != verts.end(); std::advance(iter, 1))
	{
		// next x?
		if(!tl2::equals<t_real>((*iter)[0], (*iterCurX)[0], eps))
		{
			std::size_t num_same_x = iter - iterCurX;
			if(num_same_x >= 3)
				iter = verts.erase(std::next(iterCurX, 1), std::prev(iter, 1));
			iterCurX = iter;
		}
	}

	return verts;
}


template<class t_vec, class t_real = typename t_vec::value_type>
std::tuple<std::vector<t_vec>, t_vec>
sort_vertices_by_angle(const std::vector<t_vec>& _verts)
requires tl2::is_vec<t_vec>
{
	std::vector<t_vec> verts = _verts;

	// sort by angle
	t_vec mean = std::accumulate(verts.begin(), verts.end(), tl2::zero<t_vec>(2));
	mean /= t_real(verts.size());
	std::stable_sort(verts.begin(), verts.end(), [&mean](const t_vec& vec1, const t_vec& vec2)->bool
	{
		return line_angle<t_vec>(mean, vec1) < line_angle<t_vec>(mean, vec2);
	});

	return std::make_tuple(verts, mean);
}


/**
 * test if point is on the circle border
 */
template<class t_vec, class t_real = typename t_vec::value_type>
requires tl2::is_vec<t_vec>
bool is_on_circle(
	const t_vec& org, t_real rad, 
	const t_vec& pos, 
	t_real eps = std::numeric_limits<typename t_vec::value_type>::epsilon())
{
	t_real val = tl2::inner<t_vec>(org-pos, org-pos);
	return tl2::equals<t_real>(val, rad*rad, eps);
};


/**
 * intersection of two circles
 * <x-mid_{1,2} | x-mid_{1,2}> = r_{1,2}^2
 *
 * circle 1:
 * trafo to mid_1 = (0,0)
 * x^2 + y^2 = r_1^2  ->  y = +-sqrt(r_1^2 - x^2)
 *
 * circle 2:
 * (x-m_1)^2 + (y-m_2)^2 = r_2^2
 * (x-m_1)^2 + (+-sqrt(r_1^2 - x^2)-m_2)^2 = r_2^2
 *
 * @see https://mathworld.wolfram.com/Circle-CircleIntersection.html
 */
template<class t_vec, template<class...> class t_cont = std::vector>
requires tl2::is_vec<t_vec>
t_cont<t_vec> intersect_circle_circle(
	const t_vec& org1, typename t_vec::value_type r1,
	const t_vec& org2, typename t_vec::value_type r2,
	typename t_vec::value_type eps = 
		std::sqrt(std::numeric_limits<typename t_vec::value_type>::epsilon()))
{
	using T = typename t_vec::value_type;

	T m1 = org2[0] - org1[0];
	T m2 = org2[1] - org1[1];

	T r1_2 = r1*r1;
	T r2_2 = r2*r2;
	T m1_2 = m1*m1;
	T m2_2 = m2*m2;
	T m2_4 = m2_2*m2_2;

	T rt = T(2)*m2_2 * (r1_2*r2_2 + m1_2*(r1_2 + r2_2) + m2_2*(r1_2 + r2_2))
		- m2_2 * (r1_2*r1_2 + r2_2*r2_2)
		- (T(2)*m1_2*m2_4 + m1_2*m1_2*m2_2 + m2_4*m2_2);

	t_cont<t_vec> inters;
	if(rt < T(0))
		return inters;

	rt = std::sqrt(rt);
	T factors = m1*(r1_2 - r2_2) + m1*m1_2 + m1*m2_2;
	T div = T(2)*(m1_2 + m2_2);

	// first intersection
	T x1 = (factors - rt) / div;
	T y1a = std::sqrt(r1_2 - x1*x1);
	T y1b = -std::sqrt(r1_2 - x1*x1);

	t_vec pos1a = tl2::create<t_vec>({x1, y1a}) + org1;
	t_vec pos1b = tl2::create<t_vec>({x1, y1b}) + org1;

	if(is_on_circle<t_vec>(org1, r1, pos1a, eps) 
		&& is_on_circle<t_vec>(org2, r2, pos1a, eps))
	{
		inters.emplace_back(std::move(pos1a));
	}

	if(!tl2::equals<t_vec>(pos1a, pos1b, eps) 
		&& is_on_circle<t_vec>(org1, r1, pos1b, eps) 
		&& is_on_circle<t_vec>(org2, r2, pos1b, eps))
	{
		inters.emplace_back(std::move(pos1b));
	}

	// second intersection
	if(!tl2::equals<T>(rt, T(0), eps))
	{
		T x2 = (factors + rt) / div;
		T y2a = std::sqrt(r1_2 - x2*x2);
		T y2b = -std::sqrt(r1_2 - x2*x2);

		t_vec pos2a = tl2::create<t_vec>({x2, y2a}) + org1;
		t_vec pos2b = tl2::create<t_vec>({x2, y2b}) + org1;

		if(is_on_circle<t_vec>(org1, r1, pos2a, eps) 
			&& is_on_circle<t_vec>(org2, r2, pos2a, eps))
		{
			inters.emplace_back(std::move(pos2a));
		}

		if(!tl2::equals<t_vec>(pos2a, pos2b, eps) 
			&& is_on_circle<t_vec>(org1, r1, pos2b, eps) 
			&& is_on_circle<t_vec>(org2, r2, pos2b, eps))
		{
			inters.emplace_back(std::move(pos2b));
		}
	}

	// sort intersections by x
	std::sort(inters.begin(), inters.end(), 
	[](const t_vec& vec1, const t_vec& vec2) -> bool
	{
		return vec1[0] < vec2[0];
	});

	return inters;
}


/**
 * check circles for collision
 */
template<class t_vec> requires tl2::is_vec<t_vec>
bool collide_circle_circle(
	const t_vec& org1, typename t_vec::value_type r1,
	const t_vec& org2, typename t_vec::value_type r2)
{
	using t_real = typename t_vec::value_type;

	t_real dot = tl2::inner<t_vec>(org2-org1, org2-org1);
	return (dot < (r2+r1) * (r2+r1));
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// line segment intersections
// @see (Klein 2005), ch. 2.3.2, pp. 64f
// @see (FUH 2020), ch. 2.3.2, pp. 69-80
// ----------------------------------------------------------------------------

template<class t_vec, class t_line = std::pair<t_vec, t_vec>>
std::vector<std::tuple<std::size_t, std::size_t, t_vec>>
intersect_ineff(const std::vector<t_line>& lines)
requires tl2::is_vec<t_vec>
{
	std::vector<std::tuple<std::size_t, std::size_t, t_vec>> intersections;

	for(std::size_t i=0; i<lines.size(); ++i)
	{
		for(std::size_t j=i+1; j<lines.size(); ++j)
		{
			const t_line& line1 = lines[i];
			const t_line& line2 = lines[j];

			if(auto [intersects, pt] = intersect_lines<t_line>(line1, line2); intersects)
				intersections.emplace_back(std::make_tuple(i, j, pt));
		}
	}

	return intersections;
}


template<class t_hook, class t_vec, class t_line = std::pair<t_vec, t_vec>>
requires tl2::is_vec<t_vec>
struct IntersTreeLeaf
{
	using t_real = typename t_vec::value_type;

	const t_real *curX{nullptr};
	const std::vector<t_line> *lines{nullptr};
	std::size_t line_idx{0};

	t_hook _h{};

	friend std::ostream& operator<<(std::ostream& ostr, const IntersTreeLeaf<t_hook, t_vec, t_line>& e)
	{
		ostr << std::get<0>((*e.lines)[e.line_idx]) << ", " << std::get<1>((*e.lines)[e.line_idx]);
		return ostr;
	}

	friend bool operator<(const IntersTreeLeaf<t_hook, t_vec, t_line>& e1, const IntersTreeLeaf<t_hook, t_vec, t_line>& e2)
	{
		t_real line1_y = get_line_y<t_vec>((*e1.lines)[e1.line_idx], *e1.curX);
		t_real line2_y = get_line_y<t_vec>((*e2.lines)[e2.line_idx], *e2.curX);

		// compare by y
		return line1_y < line2_y;
	}
};


/**
 * line segment intersection via sweep
 * @see (FUH 2020), ch. 2.3.2, pp. 69-80
 */
template<class t_vec, 
	class t_line = std::pair<t_vec, t_vec>, 
	class t_real = typename t_vec::value_type>
std::vector<std::tuple<std::size_t, std::size_t, t_vec>>
intersect_sweep(const std::vector<t_line>& _lines, t_real eps = 1e-6)
requires tl2::is_vec<t_vec>
{
	using t_mat = tl2::mat<t_real, std::vector>;

	// look for vertical lines
	bool has_vert_line = 0;
	t_real min_angle_to_y{std::numeric_limits<t_real>::max()};
	std::vector<t_line> lines = _lines;

	for(const t_line& line : lines)
	{
		if(tl2::equals<t_real>(line.first[0], line.second[0], eps))
		{
			has_vert_line = 1;
		}
		else
		{
			// get angles relative to y axis
			t_real angle_to_y = line_angle<t_vec>(line.first, line.second) + tl2::pi<t_real>/t_real(2);
			angle_to_y = tl2::mod_pos<t_real>(angle_to_y, t_real(2)*tl2::pi<t_real>);
			if(angle_to_y > tl2::pi<t_real>/t_real(2))
				angle_to_y -= tl2::pi<t_real>;

			if(std::abs(angle_to_y) < std::abs(min_angle_to_y))
				min_angle_to_y = angle_to_y;
		}
	}

	// rotate all lines
	std::optional<t_mat> rotmat;
	if(has_vert_line)
	{
		rotmat = tl2::rotation_2d<t_mat>(-min_angle_to_y * t_real(0.5));

		for(t_line& line : lines)
		{
			line.first = *rotmat * line.first;
			line.second = *rotmat * line.second;
		}
	}


	enum class SweepEventType { LEFT_VERTEX, RIGHT_VERTEX, INTERSECTION };

	struct SweepEvent
	{
		t_real x;
		SweepEventType ty{SweepEventType::LEFT_VERTEX};

		std::size_t line_idx{};
		std::optional<std::size_t> lower_idx{}, upper_idx{};
		std::optional<t_vec> intersection{};

		void print(std::ostream& ostr)
		{
			std::string strty;
			if(ty == SweepEventType::LEFT_VERTEX)
				strty = "left_vertex";
			else if(ty == SweepEventType::RIGHT_VERTEX)
				strty = "right_vertex";
			else if(ty == SweepEventType::INTERSECTION)
				strty = "intersection";

			ostr << "x=" << std::setw(6) << x << ", type=" << std::setw(12)
				<< strty << ", line " << line_idx;
			if(lower_idx)
				ostr << ", lower=" << *lower_idx;
			if(upper_idx)
				ostr << ", upper=" << *upper_idx;

			if(intersection)
			{
				//using namespace tl2_ops;
				//ostr << ", intersection=" << *intersection;
				ostr << ", intersection=(" << (*intersection)[0] << "," << (*intersection)[1] << ")";
			}
		}
	};


	namespace intr = boost::intrusive;

	using t_leaf = IntersTreeLeaf<
		intr::avl_set_member_hook<
			intr::link_mode<
				intr::normal_link>>,
		t_vec, t_line>;

	using t_tree = intr::avltree<
		t_leaf, intr::member_hook<
			t_leaf, decltype(t_leaf::_h), &t_leaf::_h>>;


	// events
	auto events_comp = [](const SweepEvent& evt1, const SweepEvent& evt2) -> bool
	{
		return evt1.x > evt2.x;
	};

	std::priority_queue<SweepEvent, std::vector<SweepEvent>, decltype(events_comp)> events(events_comp);

	for(std::size_t line_idx=0; line_idx<lines.size(); ++line_idx)
	{
		const t_line& line = lines[line_idx];

		SweepEvent evtLeft{.x = std::get<0>(line)[0], .ty=SweepEventType::LEFT_VERTEX, .line_idx=line_idx};
		SweepEvent evtRight{.x = std::get<1>(line)[0], .ty=SweepEventType::RIGHT_VERTEX, .line_idx=line_idx};

		// wrong order of vertices?
		if(evtLeft.x > evtRight.x)
		{
			std::swap(evtLeft.ty, evtRight.ty);

			events.emplace(std::move(evtRight));
			events.emplace(std::move(evtLeft));
		}
		else
		{
			events.emplace(std::move(evtLeft));
			events.emplace(std::move(evtRight));
		}
	}


	// status
	t_tree status;

	// results
	std::vector<std::tuple<std::size_t, std::size_t, t_vec>> intersections;

	t_real curX = 0.;
	while(events.size())
	{
		SweepEvent evt{std::move(events.top())};
		events.pop();

		curX = evt.x;

		switch(evt.ty)
		{
			case SweepEventType::LEFT_VERTEX:
			{
				// activate line
				t_leaf *leaf = new t_leaf{.curX=&curX, .lines=&lines, .line_idx=evt.line_idx};
				auto iter = status.insert_equal(*leaf);

				auto iterPrev = (iter == status.begin() ? status.end() : std::prev(iter, 1));
				auto iterNext = std::next(iter, 1);

				// add possible intersection event
				if(iterPrev != iter && iterPrev != status.end())
				{
					const t_line& line = lines[evt.line_idx];
					const t_line& linePrev = lines[iterPrev->line_idx];

					if(auto [intersects, pt] = intersect_lines<t_line>(line, linePrev);
					   intersects && !tl2::equals<t_real>(curX, pt[0], eps))
					{
						SweepEvent evtPrev{.x = pt[0], .ty=SweepEventType::INTERSECTION,
							.lower_idx=iterPrev->line_idx, .upper_idx=evt.line_idx,
							.intersection=pt};
						events.emplace(std::move(evtPrev));
					}
				}

				// add possible intersection event
				if(iterNext != iter && iterNext != status.end())
				{
					const t_line& line = lines[evt.line_idx];
					const t_line& lineNext = lines[iterNext->line_idx];

					if(auto [intersects, pt] = intersect_lines<t_line>(line, lineNext);
					   intersects && !tl2::equals<t_real>(curX, pt[0], eps))
					{
						SweepEvent evtNext{.x = pt[0], .ty=SweepEventType::INTERSECTION,
							.lower_idx=evt.line_idx, .upper_idx=iterNext->line_idx,
							.intersection=pt};
						events.emplace(std::move(evtNext));
					}
				}

				break;
			}
			case SweepEventType::RIGHT_VERTEX:
			{
				// find current line
				auto iter = std::find_if(status.begin(), status.end(),
					[&evt](const auto& leaf) -> bool
					{ return leaf.line_idx == evt.line_idx; });

				if(iter == status.end())
					continue;

				auto iterPrev = (iter == status.begin() ? status.end() : std::prev(iter, 1));
				auto iterNext = std::next(iter, 1);

				// inactivate current line
				delete &*iter;
				iter = status.erase(iter);


				// add possible intersection event
				if(iterPrev != iterNext && iterPrev != status.end() && iterNext != status.end())
				{
					const t_line& linePrev = lines[iterPrev->line_idx];
					const t_line& lineNext = lines[iterNext->line_idx];

					if(auto [intersects, pt] = intersect_lines<t_line>(linePrev, lineNext);
					   intersects && !tl2::equals<t_real>(curX, pt[0], eps))
					{
						SweepEvent evt{.x = pt[0], .ty=SweepEventType::INTERSECTION,
							.lower_idx=iterPrev->line_idx, .upper_idx=iterNext->line_idx,
							.intersection=pt};
						events.emplace(std::move(evt));
					}
				}

				break;
			}
			case SweepEventType::INTERSECTION:
			{
				if(std::find_if(intersections.begin(), intersections.end(),
					[&evt, eps](const auto& inters) -> bool
						{ return tl2::equals<t_vec>(std::get<2>(inters), *evt.intersection, eps); })
					== intersections.end())
				{
					// report an intersection
					intersections.emplace_back(std::make_tuple(*evt.lower_idx, *evt.upper_idx, *evt.intersection));
				}
				else
				{
					// intersection already reported
					continue;
				}

				// find upper line
				auto iterUpper = std::find_if(status.begin(), status.end(),
					[&evt](const auto& leaf) -> bool
					{ return leaf.line_idx == evt.upper_idx; });

				// find lower line
				auto iterLower = std::find_if(status.begin(), status.end(),
					[&evt](const auto& leaf) -> bool
					{ return leaf.line_idx == evt.lower_idx; });


				std::swap(iterUpper->line_idx, iterLower->line_idx);
				std::swap(iterUpper, iterLower);


				auto iterPrev = (iterUpper == status.begin() ? status.end() : std::prev(iterUpper, 1));
				auto iterNext = std::next(iterLower, 1);


				// add possible intersection event
				if(iterPrev != iterUpper && iterPrev != status.end() && iterUpper != status.end())
				{
					const t_line& linePrev = lines[iterPrev->line_idx];
					const t_line& lineUpper = lines[iterUpper->line_idx];

					if(auto [intersects, pt] = intersect_lines<t_line>(linePrev, lineUpper);
					   intersects && !tl2::equals<t_real>(curX, pt[0], eps))
					{
						SweepEvent evtPrev{.x = pt[0], .ty=SweepEventType::INTERSECTION,
							.lower_idx=iterPrev->line_idx, .upper_idx=iterUpper->line_idx,
							.intersection=pt};
						events.emplace(std::move(evtPrev));
					}
				}

				// add possible intersection event
				if(iterNext != iterLower && iterNext != status.end() && iterLower != status.end())
				{
					const t_line& lineNext = lines[iterNext->line_idx];
					const t_line& lineLower = lines[iterLower->line_idx];

					if(auto [intersects, pt] = intersect_lines<t_line>(lineNext, lineLower);
					   intersects && !tl2::equals<t_real>(curX, pt[0], eps))
					{
						SweepEvent evtNext{.x = pt[0], .ty=SweepEventType::INTERSECTION,
							.lower_idx=iterLower->line_idx, .upper_idx=iterNext->line_idx,
							.intersection=pt};
						events.emplace(std::move(evtNext));
					}
				}

				break;
			}
		}
	}


	// rotate intersection points back
	if(rotmat)
	{
		*rotmat = tl2::trans<t_mat>(*rotmat);

		for(auto& inters : intersections)
			std::get<2>(inters) = *rotmat * std::get<2>(inters);
	}

	return intersections;
}

// ----------------------------------------------------------------------------

}
#endif
