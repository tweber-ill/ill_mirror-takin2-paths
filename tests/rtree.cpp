/**
 * testing rtree internal
 * @author Tobias Weber <tweber@ill.fr>
 * @date sep-2021
 * @license GPLv3, see 'LICENSE' file
 * @note Forked on 10-sep-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * References:
 *  * http://www.boost.org/doc/libs/1_65_1/libs/geometry/doc/html/index.html
 *  * http://www.boost.org/doc/libs/1_65_1/libs/geometry/doc/html/geometry/spatial_indexes/rtree_examples.html
 *  * https://www.boost.org/doc/libs/1_76_0/libs/geometry/doc/html/geometry/reference/algorithms/buffer/buffer_7_with_strategies.html
 *  * https://github.com/boostorg/geometry/tree/develop/example
 *  * https://github.com/boostorg/geometry/blob/develop/include/boost/geometry/index/detail/rtree/visitors/iterator.hpp
 *  * https://github.com/boostorg/geometry/blob/develop/include/boost/geometry/index/detail/rtree/node/weak_visitor.hpp
 *
 * g++ -Wall -Wextra -Weffc++ -std=c++20 -o rtree rtree.cpp
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                     Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2021  Tobias WEBER (privately developed).
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
#include <string>
#include <tuple>
#include <vector>
#include <random>

// rtree with access to private members
#include "rtree.hpp"
//#include <boost/geometry/index/rtree.hpp>

#include <boost/function_output_iterator.hpp>
#include <boost/geometry.hpp>
#include <boost/type_index.hpp>

namespace geo = boost::geometry;
namespace trafo = geo::strategy::transform;
namespace geoidx = geo::index;
namespace ty = boost::typeindex;


using t_real = double;
using t_vertex = geo::model::point<t_real, 2, geo::cs::cartesian>;
using t_box = geo::model::box<t_vertex>;
using t_svg = geo::svg_mapper<t_vertex>;


template<class t_tree>
struct Visitor
{
	using t_bounds = typename t_tree::bounds_type;

	template<class T> void operator()(const T& node_or_leaf)
	{
		if(!m_all_bounds)
		{
			std::cerr << "Error: No bounds vector!" << std::endl;;
			return;
		}

		/*std::cout << "In visitor with type: "
			<< ty::type_id_with_cvr<T>().pretty_name()
			<< std::endl;*/

		const auto& elements = geoidx::detail::rtree::elements(node_or_leaf);
		constexpr bool is_node = !std::is_same_v<
			std::decay_t<decltype(*elements.begin())>, typename t_tree::value_type>;

		// are we at node level (not at the leaves)?
		if constexpr(is_node)
		{
			std::cout << "Node level " << m_level << ": " 
				<< elements.size() << " child nodes" << std::endl;

			const t_bounds& bounds = geoidx::detail::rtree::elements_box<t_bounds>(
				elements.begin(), elements.end(), m_tree->translator(), 
				geoidx::detail::get_strategy(m_tree->parameters()));
			m_all_bounds->push_back(std::make_tuple(bounds, m_level));

			for(const auto& element : elements)
			{
				Visitor<t_tree> nextvisitor;
				nextvisitor.m_tree = this->m_tree;
				nextvisitor.m_level = this->m_level + 1;
				nextvisitor.m_all_bounds = this->m_all_bounds;
				geoidx::detail::rtree::apply_visitor(nextvisitor, *element.second);
			}
		}
		else
		{
			std::cout << "Leaf level " << m_level << ": " 
				<< elements.size() << " elements" << std::endl;

			const t_bounds& bounds = geoidx::detail::rtree::elements_box<t_bounds>(
				elements.begin(), elements.end(), m_tree->translator(), 
				geoidx::detail::get_strategy(m_tree->parameters()));
			m_all_bounds->push_back(std::make_tuple(bounds, m_level));
		}
	}


	std::size_t m_level{0};
	const t_tree* m_tree{};

	// bounding box and tree level
	std::shared_ptr<std::vector<std::tuple<t_bounds, std::size_t>>> m_all_bounds{};
};


template<class t_rtree, std::size_t max_elems = 8>
void test_rtree(const std::vector<t_vertex>& points, const char* outfile)
{
	using t_rtreebox = typename t_rtree::bounds_type;

	std::cout << "r-tree bounding box type: "
		<< ty::type_id_with_cvr<t_rtreebox>().pretty_name()
		<< std::endl;
	std::cout << "r-tree value type: "
		<< ty::type_id_with_cvr<typename t_rtree::value_type>().pretty_name()
		<< std::endl;

	std::cout << "r-tree internal node type: "
		<< ty::type_id_with_cvr<typename t_rtree::node>().pretty_name()
		<< std::endl;
	std::cout << "r-tree internal leaf type: "
		<< ty::type_id_with_cvr<typename t_rtree::leaf>().pretty_name()
		<< std::endl;


	// spatial indices
	t_rtree rt1(typename t_rtree::parameters_type{max_elems});

	// insert points
	for(std::size_t idx=0; idx<points.size(); ++idx)
		rt1.insert(std::make_tuple(points[idx], idx));


	std::size_t level = rt1.m_members.leafs_level;
	std::cout << "Number of levels: " << level << std::endl;

	Visitor<t_rtree> visitor;
	visitor.m_all_bounds = std::make_shared<std::vector<std::tuple<t_rtreebox, std::size_t>>>();
	visitor.m_tree = &rt1;

	// see: https://github.com/boostorg/geometry/blob/develop/include/boost/geometry/index/detail/rtree/node/weak_visitor.hpp
	geoidx::detail::rtree::apply_visitor(visitor, *rt1.m_members.root);
	
	std::cout << "Total number of bounding boxes: "
		<< visitor.m_all_bounds->size() << std::endl;


	// write svg
	{
		const std::vector<std::string> colours =
		{{
			"#000000",
			"#ff0000",
			"#0000ff",
			"#00aa00",
			"#aaaa00",
			"#00aaaa",
			"#aa00aa",
		}};

		std::ofstream ofstr(outfile);
		t_svg svg1(ofstr, 500, 500);

		// global bounding box
		t_rtreebox globalbounds = rt1.bounds();
		svg1.add(globalbounds);
		svg1.map(globalbounds, "stroke:#000000; stroke-width:3px; fill:none; stroke-linecap:round; stroke-linejoin:round;", 2.);

		// sort bounding boxes by level
		std::stable_sort(visitor.m_all_bounds->begin(), visitor.m_all_bounds->end(),
			[](const auto& tup1, const auto& tup2)->bool
			{
				return std::get<1>(tup1) >= std::get<1>(tup2);
			});

		// bounding boxes
		for(const auto& [bounds, level] : *visitor.m_all_bounds)
		{
			std::size_t linewidth = level + 3;

			svg1.add(bounds);
			std::string props = "stroke:" + colours[level % colours.size()] + 
				"; stroke-width:" + std::to_string(linewidth) + 
				"px; fill:none; stroke-linecap:round; stroke-linejoin:round;";
			svg1.map(bounds, props, 2.);
		}

		// points
		for(const t_vertex& pt : points)
		{
			svg1.add(pt);
			std::string props = "fill; stroke:#000000; stroke-width:1px; fill:#000000;";
			svg1.map(pt, props, 3);
		}
	}
}


int main()
{
	constexpr const std::size_t NUM_POINTS = 500;

	// random points
	std::mt19937 rng{std::random_device{}()};

	std::vector<t_vertex> points;
	points.reserve(NUM_POINTS);
	for(std::size_t i=0; i<NUM_POINTS; ++i)
	{
		points.emplace_back(
			t_vertex{ std::uniform_real_distribution<t_real>{0., 100.}(rng),
				std::uniform_real_distribution<t_real>{0., 100.}(rng) });
	}


	std::cout << "R-tree, 16 elements" << std::endl;
	using t_rtree = geoidx::rtree<std::tuple<t_vertex, std::size_t>, geoidx::dynamic_linear>;
	test_rtree<t_rtree, 16>(points, "rtree16.svg");

	std::cout << "\n\nR-tree, 8 elements" << std::endl;
	using t_rtree = geoidx::rtree<std::tuple<t_vertex, std::size_t>, geoidx::dynamic_linear>;
	test_rtree<t_rtree, 8>(points, "rtree8.svg");

	std::cout << "\n\nR*-tree, 16 elements" << std::endl;
	using t_rstartree = geoidx::rtree<std::tuple<t_vertex, std::size_t>, geoidx::dynamic_rstar>;
	test_rtree<t_rstartree, 16>(points, "rstartree16.svg");

	std::cout << "\n\nR*-tree, 8 elements" << std::endl;
	using t_rstartree = geoidx::rtree<std::tuple<t_vertex, std::size_t>, geoidx::dynamic_rstar>;
	test_rtree<t_rstartree, 8>(points, "rstartree8.svg");

	return 0;
}
