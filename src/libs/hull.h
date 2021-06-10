/**
 * convex hull / delaunay triangulation / voronoi diagrams
 * @author Tobias Weber <tweber@ill.fr>
 * @date Oct/Nov-2020
 * @note Forked on 19-apr-2021 and 3-jun-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 *
 * References for the algorithms:
 *   - (Klein 2005) "Algorithmische Geometrie" (2005), ISBN: 978-3540209560 (http://dx.doi.org/10.1007/3-540-27619-X).
 *   - (FUH 2020) "Algorithmische Geometrie" (2020), Kurs 1840, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/1840).
 *   - (Berg 2008) "Computational Geometry" (2008), ISBN: 978-3-642-09681-5 (http://dx.doi.org/10.1007/978-3-540-77974-2).
 */

#ifndef __GEO_ALGOS_HULL_H__
#define __GEO_ALGOS_HULL_H__

#include <vector>
#include <list>
#include <set>
#include <tuple>
#include <algorithm>
#include <limits>
#include <iostream>

#include <boost/intrusive/bstree.hpp>
#include <boost/polygon/polygon.hpp>
#include <boost/polygon/voronoi.hpp>
#include <voronoi_visual_utils.hpp>

#include <libqhullcpp/Qhull.h>
#include <libqhullcpp/QhullFacet.h>
#include <libqhullcpp/QhullRidge.h>
#include <libqhullcpp/QhullFacetList.h>
#include <libqhullcpp/QhullFacetSet.h>
#include <libqhullcpp/QhullVertexSet.h>

#include "tlibs2/libs/maths.h"

#include "lines.h"
#include "graphs.h"
#include "circular_iterator.h"


// ----------------------------------------------------------------------------
// make point and line segment classes known for boost.polygon
// @see https://www.boost.org/doc/libs/1_75_0/libs/polygon/doc/gtl_custom_point.htm
// @see https://github.com/boostorg/polygon/blob/develop/example/voronoi_basic_tutorial.cpp
// ----------------------------------------------------------------------------
template<class t_vec> requires tl2::is_vec<t_vec>
struct boost::polygon::geometry_concept<t_vec>
{
	using type = boost::polygon::point_concept;
};


template<class t_vec> requires tl2::is_vec<t_vec>
struct boost::polygon::geometry_concept<std::pair<t_vec, t_vec>>
{
	using type = boost::polygon::segment_concept;
};


template<class t_vec> requires tl2::is_vec<t_vec>
struct boost::polygon::point_traits<t_vec>
{
	using coordinate_type = typename t_vec::value_type;

	static coordinate_type get(const t_vec& vec, boost::polygon::orientation_2d orientation)
	{
		return vec[orientation.to_int()];
	}
};


template<class t_vec> requires tl2::is_vec<t_vec>
struct boost::polygon::segment_traits<std::pair<t_vec, t_vec>>
{
	using coordinate_type = typename t_vec::value_type;
	using point_type = t_vec;
	using line_type = std::pair<t_vec, t_vec>; // for convenience, not part of interface

	static const point_type& get(const line_type& line, boost::polygon::direction_1d direction)
	{
		switch(direction.to_int())
		{
			case 1: return std::get<1>(line);
			case 0: default: return std::get<0>(line);
		}
	}
};
// ----------------------------------------------------------------------------


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
		auto _iterLeftMax = std::max_element(hullLeft.begin(), hullLeft.end(), [](const t_vec& vec1, const t_vec& vec2)->bool
		{ return vec1[0] < vec2[0]; });
		auto _iterRightMin = std::min_element(hullRight.begin(), hullRight.end(), [](const t_vec& vec1, const t_vec& vec2)->bool
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
		auto _iterLeftMax = std::max_element(hullLeft.begin(), hullLeft.end(), [](const t_vec& vec1, const t_vec& vec2)->bool
		{ return vec1[0] < vec2[0]; });
		auto _iterRightMin = std::min_element(hullRight.begin(), hullRight.end(), [](const t_vec& vec1, const t_vec& vec2)->bool
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
 * calculation of convex hull
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
			= std::make_pair(std::numeric_limits<t_real>::max(), -std::numeric_limits<t_real>::max());

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
			= std::make_pair(std::numeric_limits<t_real>::max(), -std::numeric_limits<t_real>::max());

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
				if(side_of_line<t_vec>(circularverts[lastgood-1], circularverts[lastgood], circularverts[curidx+1]) <= 0.)
				{
					if(lastgood+1 > curidx+1)
						continue;

					circularverts.erase(std::next(circularverts.begin(), lastgood+1), std::next(circularverts.begin(), curidx+1));
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
	std::vector<t_vec>& contour, t_real min_angle = 1./180.*tl2::pi<t_real>)
requires tl2::is_vec<t_vec>
{
	circular_wrapper circularverts(contour);

	for(std::size_t curidx = 1; curidx < contour.size()*2-1; ++curidx)
	{
		// remove vertices inside almost straight lines
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

		if(std::abs(angle) < min_angle)
		{
			circularverts.erase(circularverts.begin() + curidx);
			--curidx;
		}
	}
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// delaunay triangulation
// @see (Klein 2005), ch. 6, pp. 269f
// @see (FUH 2020), ch. 5.3, pp. 228-232
// ----------------------------------------------------------------------------

/**
 * delaunay triangulation and voronoi vertices
 * @returns [ voronoi vertices, triangles, neighbour triangle indices ]
 */
template<class t_vec> requires tl2::is_vec<t_vec>
std::tuple<std::vector<t_vec>, std::vector<std::vector<t_vec>>, std::vector<std::set<std::size_t>>>
calc_delaunay(int dim, const std::vector<t_vec>& verts, bool only_hull)
{
	using namespace tl2_ops;
	namespace qh = orgQhull;

	using t_real = typename t_vec::value_type;
	using t_real_qhull = coordT;

	std::vector<t_vec> voronoi;						// voronoi vertices
	std::vector<std::vector<t_vec>> triags;			// delaunay triangles
	std::vector<std::set<std::size_t>> neighbours;	// neighbour triangle indices

	try
	{
		std::vector<t_real_qhull> _verts;
		_verts.reserve(verts.size() * dim);
		for(const t_vec& vert : verts)
			for(int i=0; i<dim; ++i)
				_verts.push_back(t_real_qhull{vert[i]});

		qh::Qhull qh{"triag", dim, int(_verts.size()/dim), _verts.data(), 
			only_hull ? "Qt" : "v Qu QJ" };
		if(qh.hasQhullMessage())
			std::cout << qh.qhullMessage() << std::endl;


		qh::QhullFacetList facets{qh.facetList()};
		std::vector<void*> facetHandles{};

		facetHandles.reserve(facets.size());
		voronoi.reserve(facets.size());
		triags.reserve(facets.size());
		neighbours.reserve(facets.size());


		// get all triangles
		for(auto iterFacet=facets.begin(); iterFacet!=facets.end(); ++iterFacet)
		{
			if(iterFacet->isUpperDelaunay())
				continue;
			facetHandles.push_back(iterFacet->getBaseT());

			if(!only_hull)
			{
				qh::QhullPoint pt = iterFacet->voronoiVertex();

				t_vec vec = tl2::create<t_vec>(dim);
				for(int i=0; i<dim; ++i)
					vec[i] = t_real{pt[i]};

				voronoi.emplace_back(std::move(vec));
			}


			std::vector<t_vec> thetriag;
			qh::QhullVertexSet vertices = iterFacet->vertices();

			for(auto iterVertex=vertices.begin(); iterVertex!=vertices.end(); ++iterVertex)
			{
				qh::QhullPoint pt = (*iterVertex).point();

				t_vec vec = tl2::create<t_vec>(dim);
				for(int i=0; i<dim; ++i)
					vec[i] = t_real{pt[i]};

				thetriag.emplace_back(std::move(vec));
			}

			if(dim == 2)
			{
				std::tie(thetriag, std::ignore)
					= sort_vertices_by_angle<t_vec>(thetriag);
			}
			triags.emplace_back(std::move(thetriag));
		}


		// find neighbouring triangles
		if(!only_hull)
		{
			neighbours.resize(triags.size());

			std::size_t facetIdx = 0;
			for(auto iterFacet=facets.begin(); iterFacet!=facets.end(); ++iterFacet)
			{
				if(iterFacet->isUpperDelaunay())
					continue;

				qh::QhullFacetSet neighbourFacets{iterFacet->neighborFacets()};
				for(auto iterNeighbour=neighbourFacets.begin(); iterNeighbour!=neighbourFacets.end(); ++iterNeighbour)
				{
					void* handle = (*iterNeighbour).getBaseT();
					auto iterHandle = std::find(facetHandles.begin(), facetHandles.end(), handle);
					if(iterHandle != facetHandles.end())
					{
						std::size_t handleIdx = iterHandle - facetHandles.begin();
						neighbours[facetIdx].insert(handleIdx);
					}
				}

				if(++facetIdx >= triags.size())
					break;
			}
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return std::make_tuple(voronoi, triags, neighbours);
}


/**
 * @returns [triangle index, shared index 1, shared index 2, non-shared index]
 */
template<class t_vec, class t_real = typename t_vec::value_type>
std::optional<std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>>
get_triag_sharing_edge(
	std::vector<std::vector<t_vec>>& triags,
	const t_vec& vert1, const t_vec& vert2, 
	std::size_t curtriagidx, t_real eps = 1e-5)
requires tl2::is_vec<t_vec>
{
	for(std::size_t i=0; i<triags.size(); ++i)
	{
		if(i == curtriagidx)
			continue;

		const auto& triag = triags[i];

		// test all edge combinations
		if(tl2::equals<t_vec>(triag[0], vert1, eps) && tl2::equals<t_vec>(triag[1], vert2, eps))
			return std::make_tuple(i, 0, 1, 2);
		if(tl2::equals<t_vec>(triag[1], vert1, eps) && tl2::equals<t_vec>(triag[0], vert2, eps))
			return std::make_tuple(i, 1, 0, 2);
		if(tl2::equals<t_vec>(triag[0], vert1, eps) && tl2::equals<t_vec>(triag[2], vert2, eps))
			return std::make_tuple(i, 0, 2, 1);
		if(tl2::equals<t_vec>(triag[2], vert1, eps) && tl2::equals<t_vec>(triag[0], vert2, eps))
			return std::make_tuple(i, 2, 0, 1);
		if(tl2::equals<t_vec>(triag[1], vert1, eps) && tl2::equals<t_vec>(triag[2], vert2, eps))
			return std::make_tuple(i, 1, 2, 0);
		if(tl2::equals<t_vec>(triag[2], vert1, eps) && tl2::equals<t_vec>(triag[1], vert2, eps))
			return std::make_tuple(i, 2, 1, 0);
	}

	// no shared edge found
	return std::nullopt;
}


/**
 * does delaunay triangle conflict with point pt
 */
template<class t_vec> requires tl2::is_vec<t_vec>
bool is_conflicting_triag(const std::vector<t_vec>& triag, const t_vec& pt)
{
	using t_real = typename t_vec::value_type;

	// circumscribed circle radius
	t_vec center = calc_circumcentre<t_vec>(triag);
	t_real rad = tl2::norm<t_vec>(triag[0] - center);
	t_real dist = tl2::norm<t_vec>(pt - center);

	// point in circumscribed circle?
	return dist < rad;
}


template<class t_vec, class t_real = typename t_vec::value_type>
void flip_edge(std::vector<std::vector<t_vec>>& triags,
	std::size_t triagidx, std::size_t nonsharedidx, t_real eps = 1e-5)
requires tl2::is_vec<t_vec>
{
	std::size_t sharedidx1 = (nonsharedidx+1) % triags[triagidx].size();
	std::size_t sharedidx2 = (nonsharedidx+2) % triags[triagidx].size();

	// get triangle on other side of shared edge
	auto optother = get_triag_sharing_edge(
		triags, triags[triagidx][sharedidx1], triags[triagidx][sharedidx2], triagidx, eps);
	if(!optother)
		return;
	const auto [othertriagidx, othersharedidx1, othersharedidx2, othernonsharedidx] = *optother;

	if(is_conflicting_triag<t_vec>(triags[othertriagidx], triags[triagidx][nonsharedidx]))
	{
		triags[triagidx] = std::vector<t_vec>
		{{
			triags[triagidx][nonsharedidx],
			triags[othertriagidx][othernonsharedidx],
			triags[othertriagidx][othersharedidx1]
		}};

		triags[othertriagidx] = std::vector<t_vec>
		{{
			triags[triagidx][nonsharedidx],
			triags[othertriagidx][othernonsharedidx],
			triags[othertriagidx][othersharedidx2]
		}};

		// also check neighbours of newly created triangles for conflicts
		flip_edge(triags, othertriagidx, othernonsharedidx, eps);
		flip_edge(triags, othertriagidx, othersharedidx1, eps);
		flip_edge(triags, othertriagidx, othersharedidx2, eps);

		flip_edge(triags, triagidx, nonsharedidx, eps);
		flip_edge(triags, triagidx, sharedidx1, eps);
		flip_edge(triags, triagidx, sharedidx2, eps);
	}
}


/**
 * iterative delaunay triangulation
 * @see (FUH 2020), ch. 6.2, pp. 269-282
 */
template<class t_vec, class t_real = typename t_vec::value_type>
std::tuple<std::vector<t_vec>, std::vector<std::vector<t_vec>>, std::vector<std::set<std::size_t>>>
calc_delaunay_iterative(const std::vector<t_vec>& verts , t_real eps = 1e-5)
requires tl2::is_vec<t_vec>
{
	using namespace tl2_ops;

	std::vector<t_vec> voronoi;						// voronoi vertices
	std::vector<std::vector<t_vec>> triags;			// delaunay triangles
	std::vector<std::set<std::size_t>> neighbours;	// neighbour triangle indices

	if(verts.size() < 3)
		return std::make_tuple(voronoi, triags, neighbours);

	// first triangle
	triags.emplace_back(std::vector<t_vec>{{ verts[0], verts[1], verts[2] }});

	// currently inserted vertices
	std::vector<t_vec> curverts;
	curverts.reserve(verts.size());

	curverts.push_back(verts[0]);
	curverts.push_back(verts[1]);
	curverts.push_back(verts[2]);

	// insert vertices iteratively
	for(std::size_t newvertidx=3; newvertidx<verts.size(); ++newvertidx)
	{
		const t_vec& newvert = verts[newvertidx];
		//std::cout << "newvert " << newvertidx-3 << ": " << newvert << std::endl;

		// find triangle containing the new vertex
		if(auto optidx = get_containing_triag<t_vec>(triags, newvert); optidx)
		{
			//std::cout << "inside" << std::endl;

			auto conttriag = std::move(triags[*optidx]);
			triags.erase(triags.begin() + *optidx);

			// new delaunay edges connecting to newvert
			triags.emplace_back(std::vector<t_vec>{{ newvert, conttriag[0], conttriag[1] }});
			triags.emplace_back(std::vector<t_vec>{{ newvert, conttriag[0], conttriag[2] }});
			triags.emplace_back(std::vector<t_vec>{{ newvert, conttriag[1], conttriag[2] }});

			flip_edge(triags, triags.size()-3, 0, eps);
			flip_edge(triags, triags.size()-2, 0, eps);
			flip_edge(triags, triags.size()-1, 0, eps);
		}

		// new vertex is outside of any triangle
		else
		{
			auto hull = calc_hull_iterative_bintree<t_vec>(curverts, eps);
			std::tie(hull, std::ignore) = sort_vertices_by_angle<t_vec>(hull);

			// find the points in the hull visible from newvert
			std::vector<t_vec> visible;
			{
				// start indices
				auto [already_in_hull, hullvertidx1, hullvertidx2] =
					is_vert_in_hull<t_vec>(hull, newvert);
				if(already_in_hull)
					continue;

				// find visible vertices like in calc_hull_iterative
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

				for(auto iter=iterLower; iter<=iterUpper; ++iter)
					visible.push_back(*iter);
			}

			for(std::size_t visidx=0; visidx<visible.size()-1; ++visidx)
			{
				triags.emplace_back(std::vector<t_vec>{{ newvert, visible[visidx], visible[visidx+1] }});
				flip_edge(triags, triags.size()-1, 0, eps);
			}
		}

		curverts.push_back(newvert);
	}


	// find neighbouring triangles and voronoi vertices
	neighbours.resize(triags.size());
	voronoi.reserve(triags.size());

	for(std::size_t triagidx=0; triagidx<triags.size(); ++triagidx)
	{
		// sort vertices
		auto& triag = triags[triagidx];
		std::tie(triag, std::ignore) = sort_vertices_by_angle<t_vec>(triag);

		// voronoi vertices
		voronoi.emplace_back(calc_circumcentre<t_vec>(triag));

		// neighbouring triangle indices
		auto optother1 = get_triag_sharing_edge(triags, triags[triagidx][0], triags[triagidx][1], triagidx, eps);
		auto optother2 = get_triag_sharing_edge(triags, triags[triagidx][0], triags[triagidx][2], triagidx, eps);
		auto optother3 = get_triag_sharing_edge(triags, triags[triagidx][1], triags[triagidx][2], triagidx, eps);

		if(optother1) neighbours[triagidx].insert(std::get<0>(*optother1));
		if(optother2) neighbours[triagidx].insert(std::get<0>(*optother2));
		if(optother3) neighbours[triagidx].insert(std::get<0>(*optother3));
	}


	return std::make_tuple(voronoi, triags, neighbours);
}


/**
 * delaunay triangulation using parabolic trafo
 * @see (Berg 2008), pp. 254-256 and p. 168
 * @see (FUH 2020), ch. 6.5, pp. 298-300
 */
template<class t_vec> requires tl2::is_vec<t_vec>
std::tuple<std::vector<t_vec>, std::vector<std::vector<t_vec>>, std::vector<std::set<std::size_t>>>
calc_delaunay_parabolic(const std::vector<t_vec>& verts)
{
	using namespace tl2_ops;
	namespace qh = orgQhull;

	using t_real = typename t_vec::value_type;
	using t_real_qhull = coordT;

	const int dim = 2;

	std::vector<t_vec> voronoi;						// voronoi vertices
	std::vector<std::vector<t_vec>> triags;			// delaunay triangles
	std::vector<std::set<std::size_t>> neighbours;	// neighbour triangle indices

	try
	{
		std::vector<t_real_qhull> _verts;
		_verts.reserve(verts.size()*(dim+1));

		for(const t_vec& vert : verts)
		{
			_verts.push_back(t_real_qhull{vert[0]});
			_verts.push_back(t_real_qhull{vert[1]});
			_verts.push_back(t_real_qhull{vert[0]*vert[0] + vert[1]*vert[1]});
		}

		qh::Qhull qh{"triag", dim+1, int(_verts.size()/(dim+1)), _verts.data(), "Qt"};
		if(qh.hasQhullMessage())
			std::cout << qh.qhullMessage() << std::endl;


		qh::QhullFacetList facets{qh.facetList()};
		std::vector<void*> facetHandles{};

		facetHandles.reserve(facets.size());
		voronoi.reserve(facets.size());
		triags.reserve(facets.size());
		neighbours.reserve(facets.size());


		auto facetAllowed = [](auto iterFacet) -> bool
		{
			if(iterFacet->isUpperDelaunay())
				return false;

			// filter out non-visible part of hull
			qh::QhullHyperplane plane = iterFacet->hyperplane();
			t_vec normal = tl2::create<t_vec>(dim+1);
			for(int i=0; i<dim+1; ++i)
				normal[i] = t_real{plane[i]};
			// normal pointing upwards?
			if(normal[2] > 0.)
				return false;

			return true;
		};


		for(auto iterFacet=facets.begin(); iterFacet!=facets.end(); ++iterFacet)
		{
			if(!facetAllowed(iterFacet))
				continue;

			std::vector<t_vec> thetriag;
			qh::QhullVertexSet vertices = iterFacet->vertices();

			for(auto iterVertex=vertices.begin(); iterVertex!=vertices.end(); ++iterVertex)
			{
				qh::QhullPoint pt = (*iterVertex).point();

				t_vec vec = tl2::create<t_vec>(dim);
				for(int i=0; i<dim; ++i)
					vec[i] = t_real{pt[i]};

				thetriag.emplace_back(std::move(vec));
			}

			voronoi.emplace_back(calc_circumcentre<t_vec>(thetriag));
			std::tie(thetriag, std::ignore) = sort_vertices_by_angle<t_vec>(thetriag);
			triags.emplace_back(std::move(thetriag));
			facetHandles.push_back(iterFacet->getBaseT());
		}


		// find neighbouring triangles
		neighbours.resize(triags.size());

		std::size_t facetIdx = 0;
		for(auto iterFacet=facets.begin(); iterFacet!=facets.end(); ++iterFacet)
		{
			if(!facetAllowed(iterFacet))
				continue;

			qh::QhullFacetSet neighbourFacets{iterFacet->neighborFacets()};
			for(auto iterNeighbour=neighbourFacets.begin(); iterNeighbour!=neighbourFacets.end(); ++iterNeighbour)
			{
				void* handle = (*iterNeighbour).getBaseT();
				auto iterHandle = std::find(facetHandles.begin(), facetHandles.end(), handle);
				if(iterHandle != facetHandles.end())
				{
					std::size_t handleIdx = iterHandle - facetHandles.begin();
					neighbours[facetIdx].insert(handleIdx);
				}
			}

			if(++facetIdx >= triags.size())
				break;
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return std::make_tuple(voronoi, triags, neighbours);
}


/**
 * get all edges from a delaunay triangulation
 */
template<class t_vec,
	class t_edge = std::pair<std::size_t, std::size_t>,
	class t_real = typename t_vec::value_type>
std::vector<t_edge> get_edges(
	const std::vector<t_vec>& verts, 
	const std::vector<std::vector<t_vec>>& triags, 
	t_real eps)
{
	auto get_vert_idx = [&verts, eps](const t_vec& vert) -> std::optional<std::size_t>
	{
		for(std::size_t vertidx=0; vertidx<verts.size(); ++vertidx)
		{
			const t_vec& vert2 = verts[vertidx];
			if(tl2::equals<t_vec>(vert, vert2, eps))
				return vertidx;
		}

		return std::nullopt;
	};


	std::vector<t_edge> edges;
	edges.reserve(triags.size()*3*2);

	for(std::size_t vertidx=0; vertidx<verts.size(); ++vertidx)
	{
		const t_vec& vert = verts[vertidx];

		for(const auto& triag : triags)
		{
			for(std::size_t i=0; i<triag.size(); ++i)
			{
				const t_vec& triagvert = triag[i];

				if(tl2::equals<t_vec>(vert, triagvert, eps))
				{
					const t_vec& vert2 = triag[(i+1) % triag.size()];
					const t_vec& vert3 = triag[(i+2) % triag.size()];

					std::size_t vert2idx = *get_vert_idx(vert2);
					std::size_t vert3idx = *get_vert_idx(vert3);

					edges.push_back(std::make_pair(vertidx, vert2idx));
					edges.push_back(std::make_pair(vertidx, vert3idx));
				}
			}
		}
	}

	return edges;
}

// ----------------------------------------------------------------------------


/**
 * voronoi diagram for line segments
 * @see https://github.com/boostorg/polygon/blob/develop/example/voronoi_basic_tutorial.cpp
 * @see https://www.boost.org/doc/libs/1_76_0/libs/polygon/example/voronoi_advanced_tutorial.cpp
 * @see https://github.com/boostorg/polygon/blob/develop/example/voronoi_visual_utils.hpp
 * @see https://github.com/boostorg/polygon/blob/develop/example/voronoi_visualizer.cpp
 * @see https://www.boost.org/doc/libs/1_75_0/libs/polygon/doc/voronoi_diagram.htm
 */
template<class t_vec, class t_line=std::pair<t_vec, t_vec>,
	class t_graph=AdjacencyMatrix<typename t_vec::value_type>,
	class t_int = int>
std::tuple<std::vector<t_vec>, std::vector<t_line>, std::vector<std::vector<t_vec>>, t_graph>
calc_voro(const std::vector<t_line>& lines, 
	std::vector<std::pair<std::size_t, std::size_t>>& line_groups = {})
requires tl2::is_vec<t_vec> && is_graph<t_graph>
{
	using t_real = typename t_vec::value_type;
	namespace poly = boost::polygon;

	const t_real eps = 1e-3;
	const t_real parabola_eps = 1e-3;
	const t_real scale = 1000.; // internal scale for int-conversion

	// length of infinite edges
	t_real infline_len = 1.;
	for(const t_line& line : lines)
	{
		t_vec dir = std::get<1>(line) - std::get<0>(line);
		t_real len = tl2::norm(dir);
		infline_len = std::max(infline_len, len);
	}
	infline_len *= 10.;


	// type traits
	struct t_vorotraits
	{
		using coordinate_type = t_real;
		using vertex_type = poly::voronoi_vertex<coordinate_type>;
		using edge_type = poly::voronoi_edge<coordinate_type>;
		using cell_type = poly::voronoi_cell<coordinate_type>;

		struct vertex_equality_predicate_type
		{
			bool operator()(const vertex_type& vert1, const vertex_type& vert2) const
			{
				const t_real eps = 1e-3;
				return tl2::equals(vert1.x(), vert2.x(), eps)
					&& tl2::equals(vert1.y(), vert2.y(), eps);
			}
		};
	};


	poly::voronoi_builder<t_int> vorobuilder;
	for(const t_line& line : lines)
	{
		t_int x1 = t_int(std::get<0>(line)[0]*scale);
		t_int y1 = t_int(std::get<0>(line)[1]*scale);
		t_int x2 = t_int(std::get<1>(line)[0]*scale);
		t_int y2 = t_int(std::get<1>(line)[1]*scale);

		vorobuilder.insert_segment(x1, y1, x2, y2);
	}

	poly::voronoi_diagram<t_real, t_vorotraits> voro;
	vorobuilder.construct(&voro);


	// get line segment index
	auto get_segment_idx = 
		[](const typename t_vorotraits::edge_type& edge, bool twin) 
			-> std::optional<std::size_t>
	{
		const auto* cell = twin ? edge.twin()->cell() : edge.cell();
		if(!cell)
			return std::nullopt;

		return cell->source_index();
	};

	// get the group index of the line segment
	auto get_group_idx = [&line_groups, &lines, eps](std::size_t segidx)
		-> std::optional<std::size_t>
	{
		for(std::size_t grpidx=0; grpidx<line_groups.size(); ++grpidx)
		{
			auto [grp_beg, grp_end] = line_groups[grpidx];

			if(segidx >= grp_beg && segidx < grp_end)
				return grpidx;
		}

		// line is in neither region
		return std::nullopt;
	};


	// graph of voronoi vertices
	t_graph graph;

	// voronoi vertices
	std::vector<const typename t_vorotraits::vertex_type*> vorovertices;
	std::vector<t_vec> vertices;
	vorovertices.reserve(voro.vertices().size());
	vertices.reserve(voro.vertices().size());

	for(std::size_t vertidx=0; vertidx<voro.vertices().size(); ++vertidx)
	{
		const typename t_vorotraits::vertex_type* vert = &voro.vertices()[vertidx];
		t_vec vorovert = tl2::create<t_vec>({ vert->x()/scale, vert->y()/scale });

		vorovertices.push_back(vert);
		vertices.emplace_back(std::move(vorovert));
		graph.AddVertex(std::to_string(vertices.size()));
	}


	auto get_vertex_idx = 
		[&vorovertices](const typename t_vorotraits::vertex_type* vert) 
			-> std::optional<std::size_t>
	{
		// infinite edge?
		if(!vert)
			return std::nullopt;

		std::size_t idx = 0;
		for(const typename t_vorotraits::vertex_type* vertex : vorovertices)
		{
			if(vertex == vert)
				return idx;
			++idx;
		}

		return std::nullopt;
	};


	// edges
	std::vector<std::vector<t_vec>> all_parabolic_edges;
	std::vector<t_line> linear_edges;
	linear_edges.reserve(voro.edges().size());

	for(const auto& edge : voro.edges())
	{
		// only bisectors, no internal edges
		if(edge.is_secondary())
			continue;

		// add graph edges
		const auto* vert0 = edge.vertex0();
		const auto* vert1 = edge.vertex1();
		auto vert0idx = get_vertex_idx(vert0);
		auto vert1idx = get_vertex_idx(vert1);
		bool valid_vertices = vert0idx && vert1idx;

		// group lines?
		if(line_groups.size())
		{
			auto seg1idx = get_segment_idx(edge, false);
			auto seg2idx = get_segment_idx(edge, true);

			if(seg1idx && seg2idx)
			{
				auto region1 = get_group_idx(*seg1idx);
				auto region2 = get_group_idx(*seg2idx);

				// are the generating line segments part of the same group?
				if(region1 && region2 && *region1 == *region2)
					continue;
			}
		}

		if(valid_vertices)
		{
			// add to graoh, TODO: arc length of parabolic edges
			t_real len = tl2::norm(vertices[*vert1idx] - vertices[*vert0idx]);

			graph.AddEdge(*vert0idx, *vert1idx, len);
			graph.AddEdge(*vert1idx, *vert0idx, len);
		}

		if(edge.is_finite() && !valid_vertices)
			continue;


		// get line segment
		auto get_segment = [&get_segment_idx, &edge, &lines](bool twin) 
			-> const t_line*
		{
			auto idx = get_segment_idx(edge, twin);
			if(!idx)
				return nullptr;

			const t_line& line = lines[*idx];
			return &line;
		};


		// get line segment endpoint
		auto get_segment_point = [&edge, &get_segment](bool twin) 
			-> const t_vec*
		{
			const auto* cell = twin ? edge.twin()->cell() : edge.cell();
			if(!cell)
				return nullptr;

			const t_line* line = get_segment(twin);
			const t_vec* vec = nullptr;
			if(!line)
				return nullptr;

			switch(cell->source_category())
			{
				case poly::SOURCE_CATEGORY_SEGMENT_START_POINT:
					vec = &std::get<0>(*line);
					break;
				case poly::SOURCE_CATEGORY_SEGMENT_END_POINT:
					vec = &std::get<1>(*line);
					break;
				default:
					break;
			}

			return vec;
		};


		// converter functions
		auto to_point_data = [](const t_vec& vec) -> poly::point_data<t_real>
		{
			return poly::point_data<t_real>{vec[0], vec[1]};
		};

		auto vertex_to_point_data = [&scale](const typename t_vorotraits::vertex_type& vec) 
			-> poly::point_data<t_real>
		{
			return poly::point_data<t_real>{vec.x()/scale, vec.y()/scale};
		};

		auto to_vec = [](const poly::point_data<t_real>& pt) -> t_vec
		{
			return tl2::create<t_vec>({ pt.x(), pt.y() });
		};

		auto vertex_to_vec = [](const typename t_vorotraits::vertex_type& vec) 
			-> t_vec
		{
			return tl2::create<t_vec>({ vec.x(), vec.y() });
		};

		auto to_segment_data = [&to_point_data](const t_line& line) 
			-> poly::segment_data<t_real>
		{
			auto pt1 = to_point_data(std::get<0>(line));
			auto pt2 = to_point_data(std::get<1>(line));

			return poly::segment_data<t_real>{pt1, pt2};
		};


		// parabolic edge
		if(edge.is_curved() && edge.is_finite())
		{
			const t_line* seg = get_segment(edge.cell()->contains_point());
			const t_vec* pt = get_segment_point(!edge.cell()->contains_point());
			if(!seg || !pt)
				continue;

			std::vector<poly::point_data<t_real>> parabola{{ 
				vertex_to_point_data(*edge.vertex0()), 
				vertex_to_point_data(*edge.vertex1()) 
			}};

			poly::voronoi_visual_utils<t_real>::discretize(
				to_point_data(*pt), to_segment_data(*seg),
				parabola_eps, &parabola);

			if(parabola.size())
			{
				std::vector<t_vec> parabolic_edges;
				parabolic_edges.reserve(parabola.size());
				for(const auto& parabola_pt : parabola)
					parabolic_edges.emplace_back(to_vec(parabola_pt));
				all_parabolic_edges.emplace_back(std::move(parabolic_edges));
			}
		}

		// linear edge
		else
		{
			// finite edge
			if(edge.is_finite())
			{
				linear_edges.emplace_back(std::make_pair(
					vertex_to_vec(*edge.vertex0()) / scale, 
					vertex_to_vec(*edge.vertex1()) / scale));
			}

			// infinite edge
			else
			{
				t_vec lineorg;
				bool inverted = false;
				if(edge.vertex0())
				{
					lineorg = vertex_to_vec(*edge.vertex0());
					inverted = false;
				}
				else if(edge.vertex1())
				{
					lineorg = vertex_to_vec(*edge.vertex1());
					inverted = true;
				}
				else
				{
					continue;
				}

				lineorg /= scale;

				const t_vec* vec = get_segment_point(false);
				const t_vec* twinvec = get_segment_point(true);

				if(!vec || !twinvec)
					continue;

				t_vec perpdir = *vec - *twinvec;
				if(inverted)
					perpdir = -perpdir;
				t_vec linedir = tl2::create<t_vec>({ perpdir[1], -perpdir[0] });

				linedir /= tl2::norm(linedir);
				linedir *= infline_len;

				linear_edges.push_back(std::make_pair(lineorg, lineorg + linedir));
			}
		}
	}

	// remove vertices with no connection
	if(line_groups.size())
	{
		std::vector<std::string> verts;
		verts.reserve(graph.GetNumVertices());

		for(std::size_t vert=0; vert<graph.GetNumVertices(); ++vert)
		{
			const std::string& id = graph.GetVertexIdent(vert);
			verts.push_back(id);
		}

		for(const std::string& id : verts)
		{
			auto neighbours_outgoing = graph.GetNeighbours(id, 1);

			if(neighbours_outgoing.size()==0)
				graph.RemoveVertex(id);
		}
	}

	return std::make_tuple(vertices, linear_edges, all_parabolic_edges, graph);
}

}
#endif
