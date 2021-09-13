/**
 * testing index trees
 * @author Tobias Weber <tweber@ill.fr>
 * @date sep-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * g++ -I.. -Wall -Wextra -Weffc++ -std=c++20 -o index_trees index_trees.cpp
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                     Grenoble, France).
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

#define BOOST_TEST_MODULE test_indextrees

#include <boost/test/included/unit_test.hpp>
namespace test = boost::unit_test;

#include <boost/iterator/function_output_iterator.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
namespace bgeo = boost::geometry;
namespace geoidx = bgeo::index;

#include <boost/type_index.hpp>
namespace ty = boost::typeindex;

#include <tuple>
#include <vector>
#include <random>
#include <iostream>

#include "src/libs/trees.h"


BOOST_AUTO_TEST_CASE_TEMPLATE(trees, t_scalar,
	decltype(std::tuple<float, double, long double>{}))
{
	// show the scalar type
	std::cout << "\nTesting for "
		<< ty::type_id_with_cvr<t_scalar>().pretty_name()
		<< " type." << std::endl;


	// vector and r tree types
	using t_vec = tl2::vec<t_scalar, std::vector>;
	using t_vertex = bgeo::model::point<t_scalar, 2, bgeo::cs::cartesian>;
	using t_rtree = geoidx::rtree<t_vertex, geoidx::dynamic_rstar>;

	// number of points and epsilon value
	constexpr const std::size_t NUM_POINTS = 5000;
	constexpr const t_scalar eps = std::numeric_limits<t_scalar>::epsilon();


	// create random points
	std::mt19937 rng{std::random_device{}()};

	std::vector<t_vec> points;
	points.reserve(NUM_POINTS);
	for(std::size_t i=0; i<NUM_POINTS; ++i)
	{
		t_vec vec = tl2::create<t_vec>({
			std::uniform_real_distribution<t_scalar>{0., 100.}(rng),
			std::uniform_real_distribution<t_scalar>{0., 100.}(rng)
		});

		points.emplace_back(std::move(vec));
	}


	// a random vector to query
	t_vec query = tl2::create<t_vec>({
		std::uniform_real_distribution<t_scalar>{0., 100.}(rng),
		std::uniform_real_distribution<t_scalar>{0., 100.}(rng)
	});


	// create a two-dimensional k-d tree
	geo::KdTree<t_vec> kd(2);
	kd.create(points);

	// get point closest to query point
	t_vec closest_kd;
	if(const auto* node = kd.get_closest(query); node)
		closest_kd = *node->vec;


	// create a two-dimensional R* tree
	t_rtree rt(typename t_rtree::parameters_type{8});
	for(const t_vec& pt : points)
		rt.insert(t_vertex{pt[0], pt[1]});

	// get point closest to query point
	std::vector<t_vec> closest_rt;
	rt.query(boost::geometry::index::nearest(
		t_vertex{query[0], query[1]}, 1),
		boost::make_function_output_iterator([&closest_rt](const auto& point)
		{
			t_vec vec = tl2::create<t_vec>({
				point.template get<0>(), point.template get<1>()});
			closest_rt.emplace_back(std::move(vec));
		}));


	// test if the two results are equal
	if(closest_kd.size() && closest_rt.size())
	{
		using namespace tl2_ops;
		std::cout << "kd result: " << closest_kd << std::endl;
		std::cout << "rt result: " << closest_rt[0] << std::endl;

		BOOST_TEST((tl2::equals<t_vec>(closest_kd, closest_rt[0], eps)));
	}
}
