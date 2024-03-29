/**
 * intersection tests
 * @author Tobias Weber <tweber@ill.fr>
 * @date 24-apr-2021
 * @note Forked on 24-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *  * http://www.boost.org/doc/libs/1_76_0/libs/geometry/doc/html/index.html
 *  * https://www.boost.org/doc/libs/1_76_0/libs/geometry/doc/html/geometry/reference/algorithms/buffer/buffer_7_with_strategies.html
 *  * https://github.com/boostorg/geometry/tree/develop/example
 *  * https://www.boost.org/doc/libs/1_76_0/libs/test/doc/html/index.html
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

#define BOOST_TEST_MODULE test_intersections_circle

#include <iostream>
#include <fstream>
#include <tuple>

#include <boost/geometry.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/type_index.hpp>
namespace bgeo = boost::geometry;
namespace strat = bgeo::strategy::buffer;
namespace test = boost::unit_test;
namespace ty = boost::typeindex;

#include "src/libs/lines.h"

template<class T> using t_vec = tl2::vec<T, std::vector>;
template<class T> using t_vertex = bgeo::model::point<T, 2, bgeo::cs::cartesian>;
template<class T> using t_poly = bgeo::model::polygon<t_vertex<T>, true /*cw*/, false /*closed*/>;
template<class T> using t_polys = bgeo::model::multi_polygon<t_poly<T>, std::vector>;


BOOST_AUTO_TEST_CASE_TEMPLATE(inters_circle, t_real,
	decltype(std::tuple</*float,*/ double, long double>{}))
{
	std::cout << "Testing with " << ty::type_id_with_cvr<t_real>().pretty_name()
		<< " type." << std::endl;

	constexpr const std::size_t NUM_TESTS = 1000;
	const t_real eps = std::sqrt(std::numeric_limits<t_real>::epsilon());
	const t_real cmp_eps = 1e-2;

	t_real rad_min = 0.5;
	t_real rad_max = 5.;
	t_real x_min = -5.;
	t_real x_max = 5.;
	t_real y_min = -5.;
	t_real y_max = 5.;

	for(std::size_t i=0; i<NUM_TESTS; ++i)
	{
		t_real rad1 = tl2::get_rand<t_real>(rad_min, rad_max);
		t_real rad2 = tl2::get_rand<t_real>(rad_min, rad_max);
		t_real x1 = tl2::get_rand<t_real>(x_min, x_max);
		t_real x2 = tl2::get_rand<t_real>(x_min, x_max);
		t_real y1 = tl2::get_rand<t_real>(y_min, y_max);
		t_real y2 = tl2::get_rand<t_real>(y_min, y_max);


		// circles
		t_polys<t_real> circle1;
		bgeo::buffer(t_vertex<t_real>{x1, y1},
			circle1,
			strat::distance_symmetric<t_real>{rad1},
			strat::side_straight{},
			strat::join_round{128},
			strat::end_round{128},
			strat::point_circle{512});

		t_polys<t_real> circle2;
		bgeo::buffer(t_vertex<t_real>{x2, y2},
			circle2,
			strat::distance_symmetric<t_real>{rad2},
			strat::side_straight{},
			strat::join_round{128},
			strat::end_round{128},
			strat::point_circle{512});


		// intersections
		std::vector<t_vertex<t_real>> inters_circle_circle;
		bgeo::intersection(circle1, circle2, inters_circle_circle);


		// custom intersection calculation
		auto custom_inters = geo::intersect_circle_circle<t_vec<t_real>>(
			tl2::create<t_vec<t_real>>({x1, y1}), rad1,
			tl2::create<t_vec<t_real>>({x2, y2}), rad2,
			eps);


		// collision calculation
		bool collide = geo::collide_circle_circle<t_vec<t_real>>(
			tl2::create<t_vec<t_real>>({x1, y1}), rad1,
			tl2::create<t_vec<t_real>>({x2, y2}), rad2);


		auto print_circles = [&x1, &x2, &y1, &y2, &rad1, &rad2, &custom_inters, &inters_circle_circle]()
		{
			std::cout << "--------------------------------------------------------------------------------" << std::endl;
			std::cout << "circle 1: mid = (" << x1 << ", " << y1 << "), rad = " << rad1 << std::endl;
			std::cout << "circle 2: mid = (" << x2 << ", " << y2 << "), rad = " << rad2 << std::endl;
			std::cout << std::endl;

			std::cout << "custom circle-circle intersection points:" << std::endl;
			for(const t_vec<t_real>& pt : custom_inters)
			{
				using namespace tl2_ops;
				std::cout << "\t" << pt << std::endl;
			}
			std::cout << std::endl;

			std::cout << "boost::geo circle-circle intersection points:" << std::endl;
			for(const auto& vert : inters_circle_circle)
			{
				std::cout << "\t" << bgeo::get<0>(vert) << "; " << bgeo::get<1>(vert) << std::endl;
			}
			std::cout << "--------------------------------------------------------------------------------" << std::endl;
			std::cout << std::endl;
		};


		if(custom_inters.size() > 0)
			BOOST_TEST((collide));


		bool sizes_equal = (inters_circle_circle.size() == custom_inters.size());
		BOOST_TEST((sizes_equal));
		if(!sizes_equal)
		{
			print_circles();
			continue;
		}

		if(inters_circle_circle.size() == custom_inters.size() && custom_inters.size() == 1)
		{
			bool equals = tl2::equals<t_real>(custom_inters[0][0], bgeo::get<0>(inters_circle_circle[0]), cmp_eps) &&
				tl2::equals<t_real>(custom_inters[0][1], bgeo::get<1>(inters_circle_circle[0]), cmp_eps);

			BOOST_TEST((equals));
			if(!equals)
				print_circles();
		}
		else if(inters_circle_circle.size() == custom_inters.size() && custom_inters.size() == 2)
		{
			auto pos1 = std::vector<std::tuple<t_real, t_real>>
			{{
				std::make_tuple(custom_inters[0][0], custom_inters[0][1]),
				std::make_tuple(custom_inters[1][0], custom_inters[1][1]),
			}};

			auto pos2 = std::vector<std::tuple<t_real, t_real>>
			{{
				std::make_tuple(bgeo::get<0>(inters_circle_circle[0]), bgeo::get<1>(inters_circle_circle[0])),
				std::make_tuple(bgeo::get<0>(inters_circle_circle[1]), bgeo::get<1>(inters_circle_circle[1])),
			}};


			/*std::sort(pos1.begin(), pos1.end(),
				[](const auto& tup1, const auto& tup2) -> bool
				{
					return std::get<0>(tup1) < std::get<0>(tup2);
				});*/

			std::sort(pos2.begin(), pos2.end(),
				[](const auto& tup1, const auto& tup2) -> bool
				{
					return std::get<0>(tup1) < std::get<0>(tup2);
				});


			bool equals = (tl2::equals<t_real>(std::get<0>(pos1[0]), std::get<0>(pos2[0]), cmp_eps) &&
				tl2::equals<t_real>(std::get<1>(pos1[0]), std::get<1>(pos2[0]), cmp_eps) &&
				tl2::equals<t_real>(std::get<0>(pos1[1]), std::get<0>(pos2[1]), cmp_eps) &&
				tl2::equals<t_real>(std::get<1>(pos1[1]), std::get<1>(pos2[1]), cmp_eps));

			BOOST_TEST((equals));
			if(!equals)
				print_circles();
		}
	}
}
