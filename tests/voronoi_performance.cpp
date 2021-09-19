/**
 * test performance of line segment voronoi diagram calculation
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
 *
 * g++-11 -std=c++20 -O2 -march=native -I../externals -I.. -DUSE_CGAL -o voronoi_performance voronoi_performance.cpp
 */


#include <iostream>
#include <iomanip>
#include <vector>

#include "../src/libs/voronoi_lines.h"


using t_real = double;
using t_vec = tl2::vec<t_real, std::vector>;
using t_mat = tl2::mat<t_real, std::vector>;
using t_line = std::pair<t_vec, t_vec>;


std::vector<t_line> random_lines(std::size_t num_lines)
{
	const int MAX_RANGE = 5000;
	const t_real MAX_SEG_LEN = 10.;

	std::vector<t_line> lines;
	lines.reserve(num_lines);

	for(std::size_t i=0; i<num_lines; ++i)
	{
		t_real x = tl2::get_rand<t_real>(-MAX_RANGE, MAX_RANGE);
		t_real y = tl2::get_rand<t_real>(-MAX_RANGE, MAX_RANGE);
		t_vec pt1 = tl2::create<t_vec>({ x, y });

		t_real len = tl2::get_rand<t_real>(0., MAX_SEG_LEN);
		t_real angle = tl2::get_rand<t_real>(0., 2.*tl2::pi<t_real>);
		t_mat rot = tl2::rotation_2d<t_mat>(angle);
		t_vec pt2 = pt1 + rot * tl2::create<t_vec>({ len, 0. });

		lines.emplace_back(std::make_pair(std::move(pt1), std::move(pt2)));
	}

	return lines;
}


int main()
{
	// create non-intersecting line segments
	std::vector<t_line> lines;
	do
	{
		lines = random_lines(50);
	}
	while(geo::intersect_sweep<t_vec, t_line>(lines).size());


	// TODO

	return 0;
}
