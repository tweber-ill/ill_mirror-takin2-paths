/**
 * convex hull
 * @author Tobias Weber <tweber@ill.fr>
 * @date October/November 2020
 * @note Forked on 19-apr-2021 and 3-jun-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license GPLv3, see 'LICENSE' file
 *
 * References for the algorithms:
 *   - (Klein 2005) "Algorithmische Geometrie" (2005), ISBN: 978-3540209560 (http://dx.doi.org/10.1007/3-540-27619-X).
 *   - (FUH 2020) "Algorithmische Geometrie" (2020), Kurs 1840, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/1840).
 *   - (Berg 2008) "Computational Geometry" (2008), ISBN: 978-3-642-09681-5 (http://dx.doi.org/10.1007/978-3-540-77974-2).
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021       Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "geo" project
 * Copyright (C) 2020-2021  Tobias WEBER (privately developed).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#ifndef __GEO_ALGOS_HULL_H__
#define __GEO_ALGOS_HULL_H__

#include <vector>
#include <list>
#include <set>
#include <unordered_map>
#include <tuple>
#include <algorithm>
#include <limits>
#include <iostream>

#include <boost/intrusive/bstree.hpp>

#include "tlibs2/libs/maths.h"
#include "lines.h"
#include "circular_iterator.h"


namespace geo {


// ----------------------------------------------------------------------------
// convex hull algorithms
// @see (Klein 2005), ch. 4.1, pp. 155f
// @see (FUH 2020), ch. 3, pp. 113-160
// ----------------------------------------------------------------------------

/**
 * recursive calculation of convex hull
 * @see (FUH 2020), ch. 3.1.4, pp. 123-125
 */
template<class t_vec, class t_real = typename t_vec::value_type>
std::vector<t_vec> _calc_hull_recursive_sorted(
	const std::vector<t_vec>& verts, t_real eps = 1e-5)
requires tl2::is_vec<t_vec>
{
	using namespace tl2_ops;

	// trivial cases to end recursion
	if(verts.size() <= 3)
	{
		std::vector<t_vec> hullverts;
		hullverts.reserve(verts.size());

		for(std::size_t vertidx=0; vertidx<verts.size(); ++vertidx)
			hullverts.push_back(verts[vertidx]);

		return std::get<0>(sort_vertices_by_angle<t_vec>(hullverts));
	}

	// divide
	std::size_t div = verts.size()/2;
	if(tl2::equals<t_real>(verts[div-1][0], verts[div][0], eps))
		++div;
	std::vector<t_vec> vertsLeft(verts.begin(), std::next(verts.begin(), div));
	std::vector<t_vec> vertsRight(std::next(verts.begin(), div), verts.end());

	// recurse
	std::vector<t_vec> hullLeft = _calc_hull_recursive_sorted(vertsLeft);
	std::vector<t_vec> hullRight = _calc_hull_recursive_sorted(vertsRight);


	// merge
	// upper part
	bool leftIsOnMax=false, rightIsOnMin=false;
	{
		auto _iterLeftMax = std::max_element(hullLeft.begin(), hullLeft.end(),
			[](const t_vec& vec1, const t_vec& vec2)->bool
			{ return vec1[0] < vec2[0]; });
		auto _iterRightMin = std::min_element(hullRight.begin(), hullRight.end(),
			[](const t_vec& vec1, const t_vec& vec2)->bool
			{ return vec1[0] < vec2[0]; });

		circular_wrapper circhullLeft(hullLeft);
		circular_wrapper circhullRight(hullRight);
		auto iterLeftMax = circhullLeft.begin() + (_iterLeftMax-hullLeft.begin());
		auto iterRightMin = circhullRight.begin() + (_iterRightMin-hullRight.begin());

		auto iterLeft = iterLeftMax;
		auto iterRight = iterRightMin;

		while(true)
		{
			bool leftChanged = false;
			bool rightChanged = false;

			while(side_of_line<t_vec>(*iterLeft, *iterRight, *(iterLeft+1)) > 0.)
			{
				++iterLeft;
				leftChanged = true;
			}
			while(side_of_line<t_vec>(*iterLeft, *iterRight, *(iterRight-1)) > 0.)
			{
				--iterRight;
				rightChanged = true;
			}

			// no more changes
			if(!leftChanged && !rightChanged)
				break;
		}

		if(iterLeft == iterLeftMax)
			leftIsOnMax = true;
		if(iterRight == iterRightMin)
			rightIsOnMin = true;

		circhullLeft.erase(iterLeftMax+1, iterLeft);
		circhullRight.erase(iterRight+1, iterRightMin);
	}

	// lower part
	{
		auto _iterLeftMax = std::max_element(hullLeft.begin(), hullLeft.end(),
			[](const t_vec& vec1, const t_vec& vec2)->bool
			{ return vec1[0] < vec2[0]; });
		auto _iterRightMin = std::min_element(hullRight.begin(), hullRight.end(),
			[](const t_vec& vec1, const t_vec& vec2)->bool
			{ return vec1[0] < vec2[0]; });

		circular_wrapper circhullLeft(hullLeft);
		circular_wrapper circhullRight(hullRight);
		auto iterLeftMax = circhullLeft.begin() + (_iterLeftMax-hullLeft.begin());
		auto iterRightMin = circhullRight.begin() + (_iterRightMin-hullRight.begin());

		auto iterLeft = iterLeftMax;
		auto iterRight = iterRightMin;

		while(true)
		{
			bool leftChanged = false;
			bool rightChanged = false;

			while(side_of_line<t_vec>(*iterLeft, *iterRight, *(iterLeft-1)) < 0.)
			{
				--iterLeft;
				leftChanged = true;
			}
			while(side_of_line<t_vec>(*iterLeft, *iterRight, *(iterRight+1)) < 0.)
			{
				++iterRight;
				rightChanged = true;
			}

			// no more changes
			if(!leftChanged && !rightChanged)
				break;
		}

		circhullLeft.erase(iterLeft+1, leftIsOnMax ? iterLeftMax : iterLeftMax+1);
		circhullRight.erase(rightIsOnMin ? iterRightMin+1 : iterRightMin, iterRight);
	}

	hullLeft.insert(hullLeft.end(), hullRight.begin(), hullRight.end());
	return std::get<0>(sort_vertices_by_angle<t_vec>(hullLeft));
}



template<class t_vec, class t_real = typename t_vec::value_type>
std::vector<t_vec> calc_hull_recursive(
	const std::vector<t_vec>& _verts, t_real eps = 1e-5)
requires tl2::is_vec<t_vec>
{
	std::vector<t_vec> verts = _sort_vertices<t_vec>(_verts, eps);

	return _calc_hull_recursive_sorted<t_vec>(verts);
}

// ----------------------------------------------------------------------------


/**
 * tests if the vertex is in the hull
 */
template<class t_vec> requires tl2::is_vec<t_vec>
std::tuple<bool, std::size_t, std::size_t> is_vert_in_hull(
	const std::vector<t_vec>& hull,
	const t_vec& newvert,
	const t_vec *vert_in_hull = nullptr)
{
	using t_real = typename t_vec::value_type;

	// get a point inside the hull if none given
	t_vec mean;
	if(!vert_in_hull)
	{
		mean = std::accumulate(hull.begin(), hull.end(), tl2::zero<t_vec>(2));
		mean /= t_real(hull.size());
		vert_in_hull = &mean;
	}

	for(std::size_t hullvertidx1=0; hullvertidx1<hull.size(); ++hullvertidx1)
	{
		std::size_t hullvertidx2 = hullvertidx1+1;
		if(hullvertidx2 >= hull.size())
			hullvertidx2 = 0;

		const t_vec& hullvert1 = hull[hullvertidx1];
		const t_vec& hullvert2 = hull[hullvertidx2];

		// new vertex is between these two points
		if(side_of_line<t_vec>(*vert_in_hull, hullvert1, newvert) > 0. &&
			side_of_line<t_vec>(*vert_in_hull, hullvert2, newvert) <= 0.)
		{
			// outside hull?
			if(side_of_line<t_vec>(hullvert1, hullvert2, newvert) < 0.)
				return std::make_tuple(false, hullvertidx1, hullvertidx2);
		}
	}
	return std::make_tuple(true, 0, 0);
};


/**
 * iterative calculation of convex hull
 * @see (FUH 2020), ch. 3.1.3, pp. 117-123
 */
template<class t_vec, class t_real = typename t_vec::value_type>
std::vector<t_vec> calc_hull_iterative(
	const std::vector<t_vec>& _verts, t_real eps = 1e-5)
requires tl2::is_vec<t_vec>
{
	using namespace tl2_ops;

	std::vector<t_vec> verts = _remove_duplicates<t_vec>(_verts, eps);

	if(verts.size() <= 3)
		return verts;

	std::vector<t_vec> hull = {{ verts[0], verts[1], verts[2] }};
	t_vec vert_in_hull = tl2::zero<t_vec>(2);
	std::tie(hull, vert_in_hull) = sort_vertices_by_angle<t_vec>(hull);


	// insert new vertex into hull
	for(std::size_t vertidx=3; vertidx<verts.size(); ++vertidx)
	{
		const t_vec& newvert = verts[vertidx];

		// is the vertex already in the hull?
		auto [already_in_hull, hullvertidx1, hullvertidx2] =
			is_vert_in_hull<t_vec>(hull, newvert, &vert_in_hull);
		if(already_in_hull)
			continue;

		circular_wrapper circularverts(hull);
		auto iterLower = circularverts.begin() + hullvertidx1;
		auto iterUpper = circularverts.begin() + hullvertidx2;

		// correct cycles
		if(hullvertidx1 > hullvertidx2 && iterLower.GetRound()==iterUpper.GetRound())
			iterUpper.SetRound(iterLower.GetRound()+1);

		for(; iterLower.GetRound()>=-2; --iterLower)
		{
			if(side_of_line<t_vec>(*iterLower, newvert, *(iterLower-1)) >= 0.)
				break;
		}

		for(; iterUpper.GetRound()<=2; ++iterUpper)
		{
			if(side_of_line<t_vec>(*iterUpper, newvert, *(iterUpper+1)) <= 0.)
				break;
		}

		auto iter = iterUpper;
		if(iterLower+1 < iterUpper)
			iter = circularverts.erase(iterLower+1, iterUpper);
		hull.insert(iter.GetIter(), newvert);
	}

	return hull;
}


/**
 * iterative calculation of convex hull
 * @see (FUH 2020), ch. 3.1.3, pp. 117-123
 */
template<class t_vec, class t_real = typename t_vec::value_type>
std::vector<t_vec> calc_hull_iterative_bintree(
	const std::vector<t_vec>& _verts, t_real eps = 1e-5)
requires tl2::is_vec<t_vec>
{
	using namespace tl2_ops;
	namespace intr = boost::intrusive;

	std::vector<t_vec> verts = _remove_duplicates<t_vec>(_verts, eps);

	if(verts.size() <= 3)
		return verts;

	std::vector<t_vec> starthull = {{ verts[0], verts[1], verts[2] }};
	t_vec vert_in_hull = std::accumulate(starthull.begin(), starthull.end(), tl2::zero<t_vec>(2));
	vert_in_hull /= t_real(starthull.size());


	using t_hook = intr::bs_set_member_hook<intr::link_mode<intr::normal_link>>;

	struct t_node
	{
		t_vec vert;
		t_real angle{};
		t_hook _h{};

		t_node(const t_vec& center, const t_vec& vert) : vert{vert}, angle{line_angle<t_vec>(center, vert)}
		{}

		bool operator<(const t_node& e2) const
		{
			return this->angle < e2.angle;
		}
	};

	using t_tree = intr::bstree<t_node,
		intr::member_hook<t_node, decltype(t_node::_h), &t_node::_h>>;
	t_tree hull;
	std::vector<t_node*> node_mem {{
		new t_node(vert_in_hull, verts[0]),
		new t_node(vert_in_hull, verts[1]),
		new t_node(vert_in_hull, verts[2]),
	}};

	for(t_node* node : node_mem)
		hull.insert_equal(*node);


	// test if the vertex is already in the hull
	auto is_in_hull = [&vert_in_hull, &hull](const t_vec& newvert)
		-> std::tuple<bool, std::size_t, std::size_t>
	{
		t_node tosearch(vert_in_hull, newvert);
		auto iter2 = hull.upper_bound(tosearch);
		// wrap around
		if(iter2 == hull.end())
			iter2 = hull.begin();

		auto iter1 = (iter2==hull.begin()
			? std::next(hull.rbegin(),1).base()
			: std::prev(iter2,1));

		const t_vec& vert1 = iter1->vert;
		const t_vec& vert2 = iter2->vert;

		// outside hull?
		if(side_of_line<t_vec>(vert1, vert2, newvert) < 0.)
		{
			std::size_t vertidx1 = std::distance(hull.begin(), iter1);
			std::size_t vertidx2 = std::distance(hull.begin(), iter2);

			return std::make_tuple(false, vertidx1, vertidx2);
		}

		return std::make_tuple(true, 0, 0);
	};


	// insert new vertex into hull
	for(std::size_t vertidx=3; vertidx<verts.size(); ++vertidx)
	{
		const t_vec& newvert = verts[vertidx];
		auto [already_in_hull, hullvertidx1, hullvertidx2] = is_in_hull(newvert);
		if(already_in_hull)
			continue;

		circular_wrapper circularverts(hull);
		auto iterLower = circularverts.begin()+hullvertidx1;
		auto iterUpper = circularverts.begin()+hullvertidx2;

		// correct cycles
		if(hullvertidx1 > hullvertidx2 && iterLower.GetRound()==iterUpper.GetRound())
			iterUpper.SetRound(iterLower.GetRound()+1);

		for(; iterLower.GetRound()>=-2; --iterLower)
		{
			if(side_of_line<t_vec>(iterLower->vert, newvert, (iterLower-1)->vert) >= 0.)
				break;
		}

		for(; iterUpper.GetRound()<=2; ++iterUpper)
		{
			if(side_of_line<t_vec>(iterUpper->vert, newvert, (iterUpper+1)->vert) <= 0.)
				break;
		}

		auto iter = iterUpper;
		if(std::distance(iterLower+1, iterUpper) > 0)
			iter = circularverts.erase(iterLower+1, iterUpper);

		t_node* newnode = new t_node(vert_in_hull, newvert);
		node_mem.push_back(newnode);
		hull.insert_equal(iter.GetIter(), *newnode);
	}


	// cleanups
	std::vector<t_vec> finalhull;
	finalhull.reserve(hull.size());

	for(auto iter = hull.begin(); iter != hull.end();)
	{
		finalhull.push_back(iter->vert);
		iter = hull.erase(iter);
	}

	for(t_node* node : node_mem)
		delete node;

	return finalhull;
}

// ----------------------------------------------------------------------------


/**
 * calculation of convex hull by contour polygon
 * @see (FUH 2020), ch. 3.1.5, pp. 125-128
 */
template<class t_vec, class t_real = typename t_vec::value_type>
std::vector<t_vec> calc_hull_contour(
	const std::vector<t_vec>& _verts, t_real eps = 1e-5)
requires tl2::is_vec<t_vec>
{
	using namespace tl2_ops;
	std::vector<t_vec> verts = _sort_vertices<t_vec>(_verts, eps);


	// contour determination
	{
		std::list<t_vec> contour_left_top, contour_left_bottom;

		std::pair<t_real, t_real> minmax_y_left
			= std::make_pair(std::numeric_limits<t_real>::max(), std::numeric_limits<t_real>::lowest());

		for(const t_vec& vec : verts)
		{
			if(vec[1] > std::get<1>(minmax_y_left))
			{
				std::get<1>(minmax_y_left) = vec[1];
				contour_left_top.push_back(vec);
			}
			if(vec[1] < std::get<0>(minmax_y_left))
			{
				std::get<0>(minmax_y_left) = vec[1];
				contour_left_bottom.push_front(vec);
			}
		}


		std::list<t_vec> contour_right_top, contour_right_bottom;
		std::pair<t_real, t_real> minmax_y_right
			= std::make_pair(std::numeric_limits<t_real>::max(), std::numeric_limits<t_real>::lowest());

		for(auto iter = verts.rbegin(); iter != verts.rend(); std::advance(iter, 1))
		{
			const t_vec& vec = *iter;
			if(vec[1] > std::get<1>(minmax_y_right))
			{
				std::get<1>(minmax_y_right) = vec[1];
				contour_right_top.push_front(vec);
			}
			if(vec[1] < std::get<0>(minmax_y_right))
			{
				std::get<0>(minmax_y_right) = vec[1];
				contour_right_bottom.push_back(vec);
			}
		}

		// convert to vector, only insert vertex if it's different than the last one
		verts.clear();
		verts.reserve(contour_left_top.size() + contour_right_top.size() +
			contour_left_bottom.size() + contour_right_bottom.size());
		for(const t_vec& vec : contour_left_top)
			if(!tl2::equals<t_vec>(*verts.rbegin(), vec, eps))
				verts.push_back(vec);
		for(const t_vec& vec : contour_right_top)
			if(!tl2::equals<t_vec>(*verts.rbegin(), vec, eps))
				verts.push_back(vec);
		for(const t_vec& vec : contour_right_bottom)
			if(!tl2::equals<t_vec>(*verts.rbegin(), vec, eps))
				verts.push_back(vec);
		for(const t_vec& vec : contour_left_bottom)
			if(!tl2::equals<t_vec>(*verts.rbegin(), vec, eps))
				verts.push_back(vec);

		if(verts.size() >= 2 && tl2::equals<t_vec>(*verts.begin(), *verts.rbegin(), eps))
			verts.erase(std::prev(verts.end(),1));
	}


	// hull calculation
	circular_wrapper circularverts(verts);
	for(std::size_t curidx = 1; curidx < verts.size()*2-1;)
	{
		if(curidx < 1)
			break;
		bool removed_points = false;

		// test convexity
		if(side_of_line<t_vec>(circularverts[curidx-1], circularverts[curidx+1], circularverts[curidx]) < 0.)
		{
			for(std::size_t lastgood = curidx; lastgood >= 1; --lastgood)
			{
				if(side_of_line<t_vec>(circularverts[lastgood-1],
					circularverts[lastgood], circularverts[curidx+1]) <= 0.)
				{
					if(lastgood+1 > curidx+1)
						continue;

					circularverts.erase(std::next(circularverts.begin(),
						lastgood+1), std::next(circularverts.begin(), curidx+1));
					curidx = lastgood;
					removed_points = true;
					break;
				}
			}
		}

		if(!removed_points)
			++curidx;
	}

	return verts;
}


/**
 * simplify a closed contour line
 */
template<class t_vec, class t_real = typename t_vec::value_type>
void simplify_contour(
	std::vector<t_vec>& contour,
	t_real min_dist = 0.01,
	t_real angular_eps = 0.01/180.*tl2::pi<t_real>,
	t_real eps = 1e-6)
requires tl2::is_vec<t_vec>
{
	// ------------------------------------------------------------------------
	// helper functions
	// ------------------------------------------------------------------------

	// check if removing a vertex creates intersecting lines in the contour
	auto can_remove_vertex = [&contour, eps](
		const t_vec& vertPrev, const t_vec& vert, const t_vec& vertNext) -> bool
	{
		// ensure real vectors as t_vec can be an interger vector
		using t_vec_real = tl2::vec<t_real, std::vector>;

		t_vec_real vert1 = tl2::create<t_vec_real>({ t_real(vertPrev[0]), t_real(vertPrev[1]) });
		t_vec_real vert2 = tl2::create<t_vec_real>({ t_real(vert[0]), t_real(vert[1]) });
		t_vec_real vert3 = tl2::create<t_vec_real>({ t_real(vertNext[0]), t_real(vertNext[1]) });

		// iterate all line segments on the contour and check for intersection with new [vert1, vert3] line segment
		for(std::size_t vert1idx=0; vert1idx<contour.size(); ++vert1idx)
		{
			std::size_t vert2idx = (vert1idx+1) % contour.size();

			const t_vec_real cont_vert1 = tl2::create<t_vec_real>({
				t_real(contour[vert1idx][0]), t_real(contour[vert1idx][1]) });
			const t_vec_real cont_vert2 = tl2::create<t_vec_real>({
				t_real(contour[vert2idx][0]), t_real(contour[vert2idx][1]) });

			// don't check self-intersections of the new line segment
			if(tl2::equals<t_vec_real>(cont_vert1, vert1, eps) ||
				tl2::equals<t_vec_real>(cont_vert1, vert2, eps) ||
				tl2::equals<t_vec_real>(cont_vert2, vert2, eps) ||
				tl2::equals<t_vec_real>(cont_vert2, vert3, eps))
				continue;

			auto [intersects, intersection] =
				intersect_lines<t_vec_real, t_real>(vert1, vert3, cont_vert1, cont_vert2, true, eps, false);
			if(intersects)
			{
				//std::cout << "intersection" << std::endl;
				return false;
			}
		}

		// no intersection found
		return true;
	};

	// ------------------------------------------------------------------------


	// circular iteration of the contour line
	circular_wrapper circularverts(contour);


	// remove contour vertices that are too close together
	for(std::size_t curidx = 0; curidx < contour.size()+1; ++curidx)
	{
		const t_vec& vert1 = circularverts[curidx];
		const t_vec& vert2 = circularverts[curidx+1];

		if(tl2::equals<t_vec, t_real>(vert1, vert2, eps))
		{
			circularverts.erase(circularverts.begin() + curidx);
			--curidx;
		}
	}


	// remove "staircase" artefacts from the contour line
	for(std::size_t curidx = 0; curidx < contour.size()+1; ++curidx)
	{
		const t_vec& vert1 = circularverts[curidx];
		const t_vec& vert2 = circularverts[curidx+1];
		const t_vec& vert3 = circularverts[curidx+2];
		const t_vec& vert4 = circularverts[curidx+3];

		if(tl2::norm<t_vec, t_real>(vert4 - vert1) > min_dist)
			continue;

		// check for horizontal or vertical line between vert2 and vert3
		t_real angle = line_angle<t_vec, t_real>(vert2, vert3);
		angle = tl2::mod_pos(angle, t_real{2}*tl2::pi<t_real>);
		//std::cout << angle/tl2::pi<t_real>*180. << std::endl;

		// line horizontal or vertical?
		if(tl2::equals_0<t_real>(angle, angular_eps)
			|| tl2::equals<t_real>(angle, tl2::pi<t_real>, angular_eps)
			|| tl2::equals<t_real>(angle, tl2::pi<t_real>/t_real(2), angular_eps)
			|| tl2::equals<t_real>(angle, tl2::pi<t_real>/t_real(3./2.), angular_eps))
		{
			t_real angle1 = line_angle<t_vec, t_real>(vert1, vert2);
			t_real angle2 = line_angle<t_vec, t_real>(vert3, vert4);

			angle1 = tl2::mod_pos(angle1, t_real{2}*tl2::pi<t_real>);
			angle2 = tl2::mod_pos(angle2, t_real{2}*tl2::pi<t_real>);

			// line angles before and after horizontal or vertical line equal?
			//std::cout << angle1/tl2::pi<t_real>*180. << ", ";
			//std::cout << angle2/tl2::pi<t_real>*180. << std::endl;
			if(tl2::equals<t_real>(angle1, angle2, angular_eps))
			{
				circularverts.erase(circularverts.begin() + curidx+3);
				circularverts.erase(circularverts.begin() + curidx+2);
			}
		}
	}


	// remove vertices along almost straight lines
	// at corners with large angles this can create crossing contour lines!
	// TODO: split into convex sub-contours and calculate the hull of each
	for(std::size_t curidx = 1; curidx < contour.size()*2-1; ++curidx)
	{
		const t_vec& vert1 = circularverts[curidx-1];
		const t_vec& vert2 = circularverts[curidx];
		const t_vec& vert3 = circularverts[curidx+1];

		t_real angle = line_angle<t_vec, t_real>(vert1, vert2, vert2, vert3);
		angle = tl2::mod_pos(angle, t_real{2}*tl2::pi<t_real>);
		if(angle > tl2::pi<t_real>)
			angle -= t_real(2)*tl2::pi<t_real>;

		//using namespace tl2_ops;
		//std::cout << "angle between " << vert1 << " ... " << vert2 << " ... " << vert3 << ": "
		//	<< angle/tl2::pi<t_real> * 180. << std::endl;

		if((std::abs(angle) < angular_eps	// staight line
			|| tl2::equals<t_real>(std::abs(angle), tl2::pi<t_real>, angular_eps)) // moving backwards
			&& can_remove_vertex(vert1, vert2, vert3))
		{
			circularverts.erase(circularverts.begin() + curidx);
			--curidx;
		}
	}
}
// ----------------------------------------------------------------------------

}
#endif
