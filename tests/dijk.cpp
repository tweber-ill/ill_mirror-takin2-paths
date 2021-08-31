/**
 * dijkstra tests
 * @author Tobias Weber <tobias.weber@tum.de>
 * @date aug-2021
 * @license see 'LICENSE' file
 * @note Forked on 5-jun-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * g++-10 -std=c++20 -I.. -o dijk dijk.cpp
 */

#define DIJK_DEBUG
#include "../src/libs/graphs.h"


template<class t_graph> requires geo::is_graph<t_graph>
void tst()
{
	using namespace tl2_ops;
	t_graph graph;

	graph.AddVertex("v1");
	graph.AddVertex("v2");
	graph.AddVertex("v3");
	graph.AddVertex("v4");
	graph.AddVertex("v5");

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

	print_graph<t_graph>(graph, std::cout);

	std::cout << "\n\ndijkstra:" << std::endl;
	auto predecessors = dijk<t_graph>(graph, "v1");

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


	std::cout << "\n\ndijkstra (modified):" << std::endl;
	auto predecessors_mod = dijk_mod<t_graph>(graph, "v1");

	for(std::size_t i=0; i<graph.GetNumVertices(); ++i)
	{
		const auto& _predidx = predecessors_mod[i];
		if(!_predidx)
			continue;

		std::size_t predidx = *_predidx;
		const std::string& vert = graph.GetVertexIdent(i);
		const std::string& pred = graph.GetVertexIdent(predidx);

		std::cout << "predecessor of " << vert << ": " << pred << "." << std::endl;
	}


	/*auto [distvecs_bellman, predecessors_bellman] = bellman<t_graph>(graph, "v1");
	std::cout << "\n\nbellman:" << std::endl;
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

	auto distvecs_floyd = floyd<t_graph>(graph);
	std::cout << "\nfloyd:" << std::endl;
	std::cout << distvecs_floyd << std::endl;*/
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
