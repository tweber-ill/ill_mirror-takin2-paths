/**
 * dijkstra tests
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date aug-2021
 * @license GPLv3, see 'LICENSE' file
 * @note Forked on 5-jun-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * References:
 *  * http://www.boost.org/doc/libs/1_76_0/libs/geometry/doc/html/index.html
 *  * https://www.boost.org/doc/libs/1_76_0/libs/geometry/doc/html/geometry/reference/algorithms/buffer/buffer_7_with_strategies.html
 *  * https://github.com/boostorg/geometry/tree/develop/example
 *  * https://www.boost.org/doc/libs/1_76_0/libs/test/doc/html/index.html
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021       Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
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

#define BOOST_TEST_MODULE test_dijkstra

#include <tuple>

#include <boost/test/included/unit_test.hpp>
#include <boost/type_index.hpp>
namespace test = boost::unit_test;
namespace ty = boost::typeindex;

#include "../src/libs/graphs.h"


BOOST_AUTO_TEST_CASE_TEMPLATE(dijkstra, t_graph,
	decltype(std::tuple<                      // test dijkstra's algorithm using both an
		geo::AdjacencyMatrix<unsigned int>,   // adjacency matrix, and
		geo::AdjacencyList<unsigned int>>{})) // an adjacency list
{
	// create a graph
	t_graph graph;

	// graph vertices
	graph.AddVertex("v1");
	graph.AddVertex("v2");
	graph.AddVertex("v3");
	graph.AddVertex("v4");
	graph.AddVertex("v5");

	// graph edges
	graph.AddEdge("v1", "v2", 1);
	graph.AddEdge("v1", "v4", 9);
	graph.AddEdge("v1", "v5", 10);
	graph.AddEdge("v2", "v3", 3);
	graph.AddEdge("v2", "v4", 7);
	graph.AddEdge("v3", "v1", 10);
	graph.AddEdge("v3", "v4", 1);
	graph.AddEdge("v3", "v5", 2);
	graph.AddEdge("v4", "v2", 1);
	graph.AddEdge("v4", "v5", 2);

	//print_graph<t_graph>(graph, std::cout);

	// run two versions of dijkstra's algorithm
	auto predecessors = dijk<t_graph>(graph, "v1");
	auto predecessors_mod = dijk_mod<t_graph>(graph, "v1");
	BOOST_TEST((predecessors.size() == predecessors_mod.size()));

	// verify that both version give the same predecessors
	for(std::size_t i=0; i<graph.GetNumVertices(); ++i)
	{
		const auto& _predidx = predecessors[i];
		const auto& _predidx_mod = predecessors_mod[i];
		BOOST_TEST((_predidx.operator bool() == _predidx_mod.operator bool()));
		if(!_predidx || !_predidx_mod)
			continue;

		std::size_t predidx = *_predidx;
		std::size_t predidx_mod = *_predidx_mod;
		BOOST_TEST((predidx == predidx_mod));

		const std::string& vert = graph.GetVertexIdent(i);
		const std::string& pred = graph.GetVertexIdent(predidx);
		const std::string& pred_mod = graph.GetVertexIdent(predidx_mod);

		BOOST_TEST((pred == pred_mod));
		//std::cout << "predecessor of " << vert << ": " << pred << "." << std::endl;
	}

	// verify that the results match with the expected predecessor indices
	const std::vector<std::optional<std::size_t>> expected_predecessors
		{{ std::nullopt, 0, 1, 2, 2 }};
	BOOST_TEST((predecessors.size() == expected_predecessors.size()));

	for(std::size_t i=0; i<std::min(predecessors.size(), expected_predecessors.size()); ++i)
	{
		BOOST_TEST((predecessors[i] == expected_predecessors[i]));
		if(predecessors[i] && expected_predecessors[i])
			BOOST_TEST((*predecessors[i] == *expected_predecessors[i]));
	}
}
