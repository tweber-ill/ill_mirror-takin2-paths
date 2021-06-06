/**
 * @author Tobias Weber <tweber@ill.fr>
 * @date jun-2021
 * @license see 'LICENSE' file
 *
 * g++ -std=c++20 -I.. -o lines lines.cpp
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
