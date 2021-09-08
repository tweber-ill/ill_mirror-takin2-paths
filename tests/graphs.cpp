/**
 * graph tests
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date may-2021
 * @license GPLv3, see 'LICENSE' file
 * @note Forked on 5-jun-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * g++-10 -std=c++20 -I.. -o graphs graphs.cpp
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021       Tobias WEBER (Institut Laue-Langevin (ILL), 
 *                          Grenoble, France).
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

#include "../src/libs/graphs.h"


template<class t_graph> requires geo::is_graph<t_graph>
void tst()
{
	using namespace tl2_ops;
	t_graph graph;

	graph.AddVertex("A");
	graph.AddVertex("B");
	graph.AddVertex("C");
	graph.AddVertex("D");
	graph.AddVertex("E");
	graph.AddVertex("F");
	graph.AddVertex("G");

	graph.AddEdge("A", "B", 5);
	graph.AddEdge("A", "C", 2);
	graph.AddEdge("A", "F", 4);
	graph.AddEdge("A", "G", 50);
	graph.AddEdge("B", "A", 1);
	graph.AddEdge("B", "D", 3);
	graph.AddEdge("D", "E", 7);
	graph.AddEdge("C", "E", 3);
	graph.AddEdge("E", "G", 2);

	print_graph<t_graph>(graph, std::cout);
	auto predecessors = dijk<t_graph>(graph, "A");
	auto [distvecs_bellman, predecessors_bellman] = bellman<t_graph>(graph, "A");
	auto distvecs_floyd = floyd<t_graph>(graph);

	std::cout << "\ndijkstra:" << std::endl;
	for(std::size_t i=0; i<graph.GetNumVertices(); ++i)
	{
		const auto& _predidx = predecessors[i];
		if(!_predidx)
			continue;

		std::size_t predidx = *_predidx;
		const std::string& vert = graph.GetVertexIdent(i);
		const std::string& pred = graph.GetVertexIdent(predidx);

		std::cout << "predecessor of " << vert << ": " << pred << "." << std::endl;
	}

	std::cout << "\nbellman:" << std::endl;
	std::cout << distvecs_bellman << std::endl;
	for(std::size_t i=0; i<graph.GetNumVertices(); ++i)
	{
		const auto& _predidx = predecessors_bellman[i];
		if(!_predidx)
			continue;

		std::size_t predidx = *_predidx;
		const std::string& vert = graph.GetVertexIdent(i);
		const std::string& pred = graph.GetVertexIdent(predidx);

		std::cout << "predecessor of " << vert << ": " << pred << "." << std::endl;
	}

	std::cout << "\nfloyd:" << std::endl;
	std::cout << distvecs_floyd << std::endl;

}


int main()
{
	{
		std::cout << "using adjacency matrix" << std::endl;
		using t_graph = geo::AdjacencyMatrix<unsigned int>;
		tst<t_graph>();
	}

	std::cout << "\n--------------------------------------------------------------------------------" << std::endl;

	{
		std::cout << "\nusing adjacency list" << std::endl;
		using t_graph = geo::AdjacencyList<unsigned int>;
		tst<t_graph>();
	}

	return 0;
}
