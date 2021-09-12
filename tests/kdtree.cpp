/**
 * testing kd trees
 * @author Tobias Weber <tweber@ill.fr>
 * @date sep-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * g++ -I.. -Wall -Wextra -Weffc++ -std=c++20 -o kdtree kdtree.cpp
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


void test_kd(const std::vector<t_vec>& points, const t_vec& query,
	const char* outfile)
{
	geo::KdTree<t_vec> kd(2);
	kd.create(points);
	std::cout << kd << std::endl;

	std::ofstream ofstr{outfile};
	geo::write_graph(ofstr, kd.get_root());

	if(const auto* node = kd.get_closest(query); node)
	{
		using namespace tl2_ops;
		std::cout << "closest: " << *node->vec << std::endl;
	}


	// testing move operator
	//geo::KdTree<t_vec> kd2 = std::move(kd);
}


int main()
{
	const t_vec query = tl2::create<t_vec>({50., 50.});

	// fixed points
	{
		std::vector<t_vec> points{{
			tl2::create<t_vec>({ 14., 11. }),
			tl2::create<t_vec>({ 10., 5. }),
			tl2::create<t_vec>({ 12., 19. }),
			tl2::create<t_vec>({ 8., 15. }),
			tl2::create<t_vec>({ 15., 18. }),
			tl2::create<t_vec>({ 7., 10. }),
		}};

		test_kd(points, query, "kdtree.dot");
	}

	std::cout << "\n\n--------------------------------------------------------------------------------" << std::endl;

	// random points
	{
		constexpr const std::size_t NUM_POINTS = 50;
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

		test_kd(points, query, "kdtree_rnd.dot");
	}

	return 0;
}
