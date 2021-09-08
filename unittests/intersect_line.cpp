/**
 * intersection tests
 * @author Tobias Weber <tweber@ill.fr>
 * @date 27-aug-2021
 * @note Forked on 27-aug-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
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

#define BOOST_TEST_MODULE test_intersections_line

#include <iostream>
#include <fstream>
#include <tuple>

#include <boost/test/included/unit_test.hpp>
#include <boost/type_index.hpp>
namespace test = boost::unit_test;
namespace ty = boost::typeindex;

#include "src/libs/lines.h"

template<class T> using t_vec = tl2::vec<T, std::vector>;


BOOST_AUTO_TEST_CASE_TEMPLATE(intersections_line, t_real,
	decltype(std::tuple</*float,*/ double, long double>{}))
{
	std::cout << "Testing with " << ty::type_id_with_cvr<t_real>().pretty_name()
		<< " type." << std::endl;

	constexpr const std::size_t NUM_TESTS = 5000;
	//const t_real eps = std::sqrt(std::numeric_limits<t_real>::epsilon());
	const t_real eps = 1e-4;

	t_real x_min = -1000.;
	t_real x_max = 1000.;
	t_real y_min = -1000.;
	t_real y_max = 1000.;

	for(std::size_t i=0; i<NUM_TESTS; ++i)
	{
		t_vec<t_real> pt1a = tl2::create<t_vec<t_real>>({tl2::get_rand(x_min, x_max), tl2::get_rand(y_min, y_max)});
		t_vec<t_real> pt1b = tl2::create<t_vec<t_real>>({tl2::get_rand(x_min, x_max), tl2::get_rand(y_min, y_max)});

		t_vec<t_real> pt2a = tl2::create<t_vec<t_real>>({tl2::get_rand(x_min, x_max), tl2::get_rand(y_min, y_max)});
		t_vec<t_real> pt2b = tl2::create<t_vec<t_real>>({tl2::get_rand(x_min, x_max), tl2::get_rand(y_min, y_max)});

		auto [ok, inters] = geo::intersect_lines<t_vec<t_real>>(pt1a, pt1b, pt2a, pt2b, true, eps);
		bool inters_check = geo::intersect_lines_check<t_vec<t_real>>(pt1a, pt1b, pt2a, pt2b);

		// check if both intersection tests agree
		BOOST_TEST((ok == inters_check));
	}
}
