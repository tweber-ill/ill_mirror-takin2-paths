/**
 * split a concave polygon into convex regions
 * @author Tobias Weber <tweber@ill.fr>
 * @date 26-jun-21
 * @license GPLv3, see 'LICENSE' file
 * @note Forked on 27-jun-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 *
 * clang++ -I.. -I../externals -std=c++20 -Wall -Wextra -o convex convex.cpp
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

#include "../src/libs/hull.h"
using namespace tl2_ops;


void tst1()
{
	using t_real = double;
	using t_vec = tl2::vec<t_real, std::vector>;

	std::vector<t_vec> poly
	{{
		tl2::create<t_vec>({0., 0.}),
		tl2::create<t_vec>({2., 2.}),
		tl2::create<t_vec>({8., 2.}),
		tl2::create<t_vec>({10., 0.}),
		tl2::create<t_vec>({10., 10.}),
		tl2::create<t_vec>({8., 8.}),
		tl2::create<t_vec>({2., 8.}),
		tl2::create<t_vec>({0., 10.}),
	}};

	auto split = geo::convex_split<t_vec, t_real>(poly);
	if(split.size() == 0)
		std::cout << "already convex" << std::endl;

	for(std::size_t idx=0; idx<split.size(); ++idx)
	{
		const auto& polysplit = split[idx];
		std::cout << "split polygon " << idx << std::endl;

		for(const auto& vec : polysplit)
			std::cout << vec << std::endl;
	}
	std::cout << std::endl;
}


void tst2()
{
	using t_real = double;
	using t_vec = tl2::vec<t_real, std::vector>;

	std::vector<t_vec> poly
	{{
		tl2::create<t_vec>({0., 0.}),
		tl2::create<t_vec>({3., 3.}),
		tl2::create<t_vec>({5., 1.}),
		tl2::create<t_vec>({5., 7.}),
		tl2::create<t_vec>({0., 7.}),
		tl2::create<t_vec>({0., 5.}),
		tl2::create<t_vec>({-7., 5.}),
		tl2::create<t_vec>({-2., 2.}),
	}};

	auto split = geo::convex_split<t_vec, t_real>(poly);
	if(split.size() == 0)
		std::cout << "already convex" << std::endl;

	for(std::size_t idx=0; idx<split.size(); ++idx)
	{
		const auto& polysplit = split[idx];
		std::cout << "split polygon " << idx << std::endl;

		for(const auto& vec : polysplit)
			std::cout << vec << std::endl;
	}
	std::cout << std::endl;
}


void tst3()
{
	using t_real = double;
	using t_vec = tl2::vec<int, std::vector>;

	// test points in wrong, non-ccw order
	std::vector<t_vec> poly
	{{
		tl2::create<t_vec>({6, 0}),
		tl2::create<t_vec>({5, -1}),
		tl2::create<t_vec>({4, 0}),
		tl2::create<t_vec>({5, 1}),
	}};

	auto split = geo::convex_split<t_vec, t_real>(poly);
	if(split.size() == 0)
		std::cout << "already convex" << std::endl;

	for(std::size_t idx=0; idx<split.size(); ++idx)
	{
		const auto& polysplit = split[idx];
		std::cout << "split polygon " << idx << std::endl;

		for(const auto& vec : polysplit)
			std::cout << vec << std::endl;
	}
	std::cout << std::endl;
}


int main()
{
	std::cout << "Test 1" << std::endl;
	tst1();

	std::cout << "\nTest 2" << std::endl;
	tst2();

	std::cout << "\nTest 3" << std::endl;
	tst3();

	return 0;
}
