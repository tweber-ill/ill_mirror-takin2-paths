/**
 * @author Tobias Weber <tweber@ill.fr>
 * @date nov-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license GPLv3, see 'LICENSE' file
 *
 * g++ -std=c++20 -I.. -o trapezoid trapezoid.cpp
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


#include <iostream>
#include <iomanip>
#include <vector>

#include "../src/libs/trapezoid.h"


using t_real = double;
using t_vec = tl2::vec<t_real, std::vector>;
//using t_mat = tl2::mat<t_real, std::vector>;


int main()
{
	std::vector<std::pair<t_vec, t_vec>> lines
	{
		// must not cross!
		{ tl2::create<t_vec>({-5, 5}), tl2::create<t_vec>({5, 1}) },
		{ tl2::create<t_vec>({-10, -5}), tl2::create<t_vec>({10, 0}) },
		{ tl2::create<t_vec>({-7, -8}), tl2::create<t_vec>({-1, -3}) },
		{ tl2::create<t_vec>({6, 7}), tl2::create<t_vec>({8, 9}) },
		{ tl2::create<t_vec>({6, 6}), tl2::create<t_vec>({8, 6}) },
		{ tl2::create<t_vec>({2, -6}), tl2::create<t_vec>({5, -6}) },
		{ tl2::create<t_vec>({-5, 7}), tl2::create<t_vec>({4, 6}) },
		{ tl2::create<t_vec>({-8, 3}), tl2::create<t_vec>({-7, 6}) },
	};

	/*std::vector<std::pair<t_vec, t_vec>> lines
	{
		{ tl2::create<t_vec>({-5, -5}), tl2::create<t_vec>({5, -5}) },
		{ tl2::create<t_vec>({-5, 5}), tl2::create<t_vec>({5, 5}) },
		{ tl2::create<t_vec>({5, -5.02}), tl2::create<t_vec>({5, 5.02}) },
		{ tl2::create<t_vec>({-5, -5.01}), tl2::create<t_vec>({-5, 5.01}) },
	};*/

	bool randomise = true;
	bool shear = true;
	auto node = geo::create_trapezoid_tree<t_vec>(lines, randomise, shear, 1., 1e-5);
	std::cout << std::make_pair(node, 0) << std::endl;
	save_trapezoid_svg(node, "tst.svg", &lines);

	return 0;
}
