/**
 * testing kdtrees
 * @author Tobias Weber <tweber@ill.fr>
 * @date sep-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * g++ -Wall -Wextra -Weffc++ -std=c++20 -o kdtree kdtree.cpp
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

#include <iostream>
#include <fstream>
#include <vector>
#include <random>

#include "src/libs/trees.h"


using t_real = double;
using t_vec = tl2::vec<t_real, std::vector>;


int main()
{
	constexpr const std::size_t NUM_POINTS = 50;

	// random points
	std::mt19937 rng{std::random_device{}()};

	std::vector<t_vec> points;
	points.reserve(NUM_POINTS);
	for(std::size_t i=0; i<NUM_POINTS; ++i)
	{
		t_vec vec = tl2::create<t_vec>({
			std::uniform_real_distribution<t_real>{0., 100.}(rng),
			std::uniform_real_distribution<t_real>{0., 100.}(rng)
		});

		points.emplace_back(std::move(vec));
	}

	geo::KdTree<t_vec> kd;
	kd.create(points);
	std::cout << kd << std::endl;

	std::ofstream ofstr{"kdtree.svg"};
	geo::write_graph(ofstr, kd.get_root());

	return 0;
}
