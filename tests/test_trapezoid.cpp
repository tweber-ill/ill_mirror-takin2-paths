/**
 * @author Tobias Weber (orcid: 0000-0002-7230-1932)
 * @date nov-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 *
 * g++ -std=c++20 -I.. -o test_trapezoid test_trapezoid.cpp
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
	};

	bool randomise = true;
	bool shear = true;
	auto node = geo::create_trapezoid_tree<t_vec>(lines, randomise, shear, 1.);
	std::cout << std::make_pair(node, 0) << std::endl;
	save_trapezoid_svg(node, "tst.svg", &lines);

	return 0;
}
