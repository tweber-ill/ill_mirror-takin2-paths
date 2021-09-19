/**
 * test voronoi diagram calculations
 * @author Tobias Weber <tweber@ill.fr>
 * @date 19-sep-2021
 * @license GPLv3, see 'LICENSE' file
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

#define BOOST_TEST_MODULE test_voronoi

#include <iostream>
#include <vector>
#include <tuple>

#include <boost/test/included/unit_test.hpp>
#include <boost/type_index.hpp>
namespace test = boost::unit_test;
namespace ty = boost::typeindex;

#define USE_CGAL
#include "src/libs/voronoi_lines.h"

template<class T> using t_vec = tl2::vec<T, std::vector>;
template<class T> using t_mat = tl2::mat<T, std::vector>;
template<class T> using t_line = std::pair<t_vec<T>, t_vec<T>>;


BOOST_AUTO_TEST_CASE_TEMPLATE(voronoi_lineseg, t_real,
	decltype(std::tuple<float, double, long double>{}))
{
	std::cout << "Testing with " << ty::type_id_with_cvr<t_real>().pretty_name()
		<< " type." << std::endl;

	const std::size_t num_lines = 50;

	// create non-intersecting line segments
	std::vector<t_line<t_real>> lines = 
		geo::random_nonintersecting_lines
			<t_line<t_real>, t_vec<t_real>, t_mat<t_real>, t_real>
			(num_lines, 1e4, 1., 100., true);

	std::vector<std::pair<std::size_t, std::size_t>> line_groups{};

	// calculate the voronoi diagrams
	auto res_boost = geo::calc_voro<t_vec<t_real>, t_line<t_real>>
		(lines, line_groups, false, false);
	auto res_cgal = geo::calc_voro_cgal<t_vec<t_real>, t_line<t_real>>
		(lines, line_groups, false, false);

	// same number of voronoi vertices found?
	BOOST_TEST((res_boost.GetVoronoiVertices().size() ==
		res_cgal.GetVoronoiVertices().size()));
}
