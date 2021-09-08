/**
 * closest pair
 * @author Tobias Weber <tweber@ill.fr>
 * @date Oct/Nov-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license GPLv3, see 'LICENSE' file
 *
 * References for the algorithms:
 *   - (Klein 2005) "Algorithmische Geometrie" (2005), ISBN: 978-3540209560 (http://dx.doi.org/10.1007/3-540-27619-X).
 *   - (FUH 2020) "Algorithmische Geometrie" (2020), Kurs 1840, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/1840).
 */

#ifndef __GEO_ALGOS_PAIR_H__
#define __GEO_ALGOS_PAIR_H__

#include <vector>
#include <tuple>
#include <algorithm>
#include <iostream>

#include "tlibs2/libs/maths.h"

#include <boost/intrusive/avltree.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>


namespace geo {

// ----------------------------------------------------------------------------
// closest pair
// @see (Klein 2005), ch. 2.2.2, pp. 53f; ch. 2.3.1, pp. 57f; ch. 2.4.1, pp. 93f
// @see (FUH 2020), ch. 2.2.2, pp. 58-69; ch. 2.3.1, pp. 62-69; ch. 2.4.1, pp. 95-96
// ----------------------------------------------------------------------------

template<class t_hook, class T>
struct ClosestPairTreeLeaf
{
	const T* vec = nullptr;
	t_hook _h{};

	friend std::ostream& operator<<(
		std::ostream& ostr, const ClosestPairTreeLeaf<t_hook, T>& e)
	{
		ostr << *e.vec;
		return ostr;
	}

	friend bool operator<(
		const ClosestPairTreeLeaf<t_hook, T>& e1,
		const ClosestPairTreeLeaf<t_hook, T>& e2)
	{
		// compare by y
		return (*e1.vec)[1] < (*e2.vec)[1];
	}
};


/**
 * closest pair (2d)
 *  - @see (Klein 2005), ch 2.3.1, p. 57
 *  - @see (FUH 2020), ch. 2.3.1, pp. 62-69
 *	- @see https://en.wikipedia.org/wiki/Closest_pair_of_points_problem
 */
template<class t_vec, class t_real = typename t_vec::value_type>
std::tuple<t_vec, t_vec, t_real>
closest_pair_sweep(const std::vector<t_vec>& points)
requires tl2::is_vec<t_vec>
{
	namespace intr = boost::intrusive;

	using t_leaf = ClosestPairTreeLeaf<
		intr::avl_set_member_hook<
			intr::link_mode<
				intr::normal_link>>,
		t_vec>;
	using t_tree = intr::avltree<
		t_leaf, intr::member_hook<
			t_leaf, decltype(t_leaf::_h), &t_leaf::_h>>;


	std::vector<t_leaf> leaves;
	for(const t_vec& pt : points)
		leaves.emplace_back(t_leaf{.vec = &pt});

	std::stable_sort(leaves.begin(), leaves.end(),
	[](const t_leaf& leaf1, const t_leaf& leaf2) -> bool
	{
		// sort by x
		return (*leaf1.vec)[0] <= (*leaf2.vec)[0];
	});


	t_leaf* leaf1 = &leaves[0];
	t_leaf* leaf2 = &leaves[1];
	t_real dist = tl2::norm<t_vec>(*leaf1->vec - *leaf2->vec);

	t_tree status;
	status.insert_equal(*leaf1);
	status.insert_equal(*leaf2);


	auto iter1 = leaves.begin();
	for(auto iter2 = std::next(leaves.begin(), 2); iter2 != leaves.end(); )
	{
		if((*iter1->vec)[0] <= (*iter2->vec)[0]-dist)
		{
			// remove dead elements
			status.erase(*iter1);
			std::advance(iter1, 1);
		}
		else
		{
			// insert newly active elements
			t_vec vec1 = *iter2->vec; vec1[1] -= dist;
			t_vec vec2 = *iter2->vec; vec2[1] += dist;
			auto [iterrange1, iterrange2] =
				status.bounded_range(t_leaf{.vec=&vec1}, t_leaf{.vec=&vec2}, 1, 1);

			for(auto iter=iterrange1; iter!=iterrange2; std::advance(iter,1))
			{
				t_real newdist = tl2::norm<t_vec>(*iter->vec - *iter2->vec);
				if(newdist < dist)
				{
					dist = newdist;
					leaf1 = &*iter;
					leaf2 = &*iter2;
				}
			}

			status.insert_equal(*iter2);
			std::advance(iter2, 1);
		}
	}

	return std::make_tuple(*leaf1->vec, *leaf2->vec, dist);
}


template<class t_vertex, class t_vec, std::size_t ...indices>
constexpr void _geo_vertex_assign(
	t_vertex& vert, const t_vec& vec,
	const std::index_sequence<indices...>&)
{
	namespace geo = boost::geometry;
	(geo::set<indices>(vert, vec[indices]), ...);
}


/**
 * closest pair (r-tree)
 */
template<std::size_t dim, class t_vec, class t_real = typename t_vec::value_type>
std::tuple<t_vec, t_vec, t_real>
closest_pair_rtree(const std::vector<t_vec>& _points)
requires tl2::is_vec<t_vec>
{
	std::vector<t_vec> points = _points;
	std::stable_sort(points.begin(), points.end(),
	[](const t_vec& pt1, const t_vec& pt2) -> bool
	{
		// sort by x
		return pt1[0] <= pt2[0];
	});


	namespace geo = boost::geometry;
	namespace geoidx = geo::index;

	using t_vertex = geo::model::point<t_real, dim, geo::cs::cartesian>;
	using t_box = geo::model::box<t_vertex>;
	using t_rtree_leaf = std::tuple<t_vertex, std::size_t>;
	using t_rtree = geoidx::rtree<t_rtree_leaf, geoidx::dynamic_linear>;

	t_rtree rt(typename t_rtree::parameters_type(points.size()));
	for(std::size_t ptidx=0; ptidx<points.size(); ++ptidx)
	{
		t_vertex vert;
		_geo_vertex_assign<t_vertex, t_vec>(
			vert, points[ptidx], std::make_index_sequence<dim>());

		rt.insert(std::make_tuple(vert, ptidx));
	}


	std::size_t idx1 = 0;
	std::size_t idx2 = 1;
	t_vec query1 = tl2::create<t_vec>(dim);
	t_vec query2 = tl2::create<t_vec>(dim);

	t_real dist = tl2::norm<t_vec>(points[idx2] - points[idx1]);
	for(std::size_t ptidx=1; ptidx<points.size(); ++ptidx)
	{
		query1[0] = points[ptidx][0] - dist;
		query2[0] = points[ptidx][0];

		for(std::size_t i=1; i<dim; ++i)
		{
			query1[i] = points[ptidx][i] - dist;
			query2[i] = points[ptidx][i] + dist;
		}

		t_vertex vert1, vert2;
		_geo_vertex_assign<t_vertex, t_vec>(vert1, query1, std::make_index_sequence<dim>());
		_geo_vertex_assign<t_vertex, t_vec>(vert2, query2, std::make_index_sequence<dim>());

		t_box query_obj(vert1, vert2);

		std::vector<t_rtree_leaf> query_answer;
		rt.query(geoidx::within(query_obj), std::back_inserter(query_answer));

		for(const auto& answ : query_answer)
		{
			std::size_t answidx = std::get<1>(answ);
			t_real newdist = tl2::norm<t_vec>(points[answidx] - points[ptidx]);
			if(newdist < dist)
			{
				dist = newdist;
				idx1 = answidx;
				idx2 = ptidx;
			}
		}
	}

	return std::make_tuple(points[idx1], points[idx2], dist);
}


/**
 * closest pair (range tree)
 * @see (FUH 2020), ch. 2.4.1, pp. 95-96; ch. 4.2.5, pp. 193-194
 */
template<std::size_t dim, class t_vec, class t_real = typename t_vec::value_type>
std::tuple<t_vec, t_vec, t_real>
closest_pair_rangetree(const std::vector<t_vec>& _points)
requires tl2::is_vec<t_vec>
{
	RangeTree<t_vec> tree;
	tree.insert(_points);

	// get x-sorted points
	std::vector<std::shared_ptr<const t_vec>> points;
	RangeTree<t_vec>::t_node::get_vecs(tree.get_root(), points);

	const t_vec* pt1 = points[0].get();
	const t_vec* pt2 = points[1].get();
	t_vec query1 = tl2::create<t_vec>(dim);
	t_vec query2 = tl2::create<t_vec>(dim);

	t_real dist = tl2::norm<t_vec>(*pt2 - *pt1);
	for(std::size_t ptidx=1; ptidx<points.size(); ++ptidx)
	{
		const t_vec& curpt = *points[ptidx];

		query1[0] = curpt[0] - dist;
		query2[0] = curpt[0];

		for(std::size_t i=1; i<dim; ++i)
		{
			query1[i] = curpt[i] - dist;
			query2[i] = curpt[i] + dist;
		}

		auto query_answer = tree.query_range(query1, query2);

		for(const auto& answ : query_answer)
		{
			t_real newdist = tl2::norm<t_vec>(*answ - curpt);
			if(answ.get() != &curpt && newdist < dist)
			{
				dist = newdist;
				pt1 = answ.get();
				pt2 = &curpt;
			}
		}
	}

	return std::make_tuple(*pt1, *pt2, dist);
}

// ----------------------------------------------------------------------------

}
#endif
