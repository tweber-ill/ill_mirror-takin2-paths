/**
 * split a concave polygon into convex regions
 * @author Tobias Weber <tweber@ill.fr>
 * @date 26-jun-21
 * @license see 'LICENSE' file
 * @note Forked on 27-jun-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 *
 * clang++ -I.. -I../externals -std=c++20 -Wall -Wextra -o convex convex.cpp
 */

#include "../src/libs/hull.h"
using namespace tl2_ops;


using t_real = double;
using t_vec = tl2::vec<t_real, std::vector>;
using t_line = std::pair<t_vec, t_vec>;


int main()
{
	std::vector<t_vec> poly
	{{
		/*0*/ tl2::create<t_vec>({0., 0.}),
		/*1*/ tl2::create<t_vec>({2., 2.}),
		/*2*/ tl2::create<t_vec>({8., 2.}),
		/*3*/ tl2::create<t_vec>({10., 0.}),
		/*4*/ tl2::create<t_vec>({10., 10.}),
		/*5*/ tl2::create<t_vec>({8., 8.}),
		/*6*/ tl2::create<t_vec>({2., 8.}),
		/*7*/ tl2::create<t_vec>({0., 10.}),
	}};

	auto split = geo::convex_split<t_vec>(poly);
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

	return 0;
}
