/**
 * @author Tobias Weber <tweber@ill.fr>
 * @date jun-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * g++ -std=c++20 -I.. -o lines lines.cpp
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
#include <iomanip>
#include <vector>

#include "../src/libs/lines.h"


using t_real = double;
using t_vec = tl2::vec<t_real, std::vector>;
//using t_mat = tl2::mat<t_real, std::vector>;


int main()
{
	using namespace tl2_ops;

	std::vector<t_vec> poly
	{{
		tl2::create<t_vec>({ -5, -5 }),
		tl2::create<t_vec>({ 5, -5 }),
		tl2::create<t_vec>({ 5, 5 }),
		tl2::create<t_vec>({ -5, 5 }),
	}};

	t_vec pt1 = tl2::create<t_vec>({ 0, 0 });
	t_vec pt2 = tl2::create<t_vec>({ 10, 0 });
	t_vec pt3 = tl2::create<t_vec>({ 3, -2 });

	std::cout << pt1 << ": " << std::boolalpha <<
		geo::pt_inside_poly(poly, pt1) << std::endl;
	std::cout << pt2 << ": " << std::boolalpha <<
		geo::pt_inside_poly(poly, pt2) << std::endl;
	std::cout << pt3 << ": " << std::boolalpha <<
		geo::pt_inside_poly(poly, pt3) << std::endl;

	return 0;
}
