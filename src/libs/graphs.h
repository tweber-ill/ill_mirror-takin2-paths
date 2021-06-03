/**
 * graph algorithms
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @note Forked on 19-apr-2021 and 3-jun-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 *
 * References for the algorithms:
 *   - (Klein 2005) "Algorithmische Geometrie" (2005), ISBN: 978-3540209560 (http://dx.doi.org/10.1007/3-540-27619-X).
 *   - (FUH 2020) "Algorithmische Geometrie" (2020), Kurs 1840, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/1840).
 *   - (FUH 2021) "Effiziente Algorithmen" (2021), Kurs 1684, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/01684).
 *   - (Berg 2008) "Computational Geometry" (2008), ISBN: 978-3-642-09681-5 (http://dx.doi.org/10.1007/978-3-540-77974-2).
 */

#ifndef __GRAPH_ALGOS_H__
#define __GRAPH_ALGOS_H__

#include <vector>
#include <queue>
#include <limits>
#include <stack>
#include <set>
#include <algorithm>
#include <iostream>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>

#include "tlibs2/libs/maths.h"
#include "containers.h"


namespace geo {

/**
 * export graph to the dot format
 * @see https://graphviz.org/doc/info/lang.html
 */
template<class t_graph> requires is_graph<t_graph>
void print_graph(const t_graph& graph, std::ostream& ostr = std::cout)
{
	const std::size_t N = graph.GetNumVertices();

	ostr << "digraph my_graph\n{\n";

	ostr << "\t// vertices\n";
	for(std::size_t i=0; i<N; ++i)
		ostr << "\t" << i << " [label=\"" << graph.GetVertexIdent(i) << "\"];\n";

	ostr << "\n";
	ostr << "\t// edges and weights\n";

	for(std::size_t i=0; i<N; ++i)
	{
		for(std::size_t j=0; j<N; ++j)
		{
			typename t_graph::t_weight w = graph.GetWeight(i, j);
			if(!w)
				continue;

			ostr << "\t" << i << " -> " << j << " [label=\"" << w << "\"];\n";
		}
	}

	ostr << "}\n";
}


/**
 * dijkstra algorithm
 * @see (FUH 2021), Kurseinheit 4, p. 17
 */
template<class t_graph> requires is_graph<t_graph>
std::vector<std::optional<std::size_t>>
dijk(const t_graph& graph, const std::string& startvert)
{
	// start index
	auto _startidx = graph.GetVertexIndex(startvert);
	if(!_startidx)
		return {};
	const std::size_t startidx = *_startidx;


	// distances
	const std::size_t N = graph.GetNumVertices();
	using t_weight = typename t_graph::t_weight;

	std::vector<t_weight> dists;
	std::vector<std::optional<std::size_t>> predecessors;
	dists.resize(N);
	predecessors.resize(N);

	// don't use the full maximum to prevent overflows when we're adding the weight afterwards
	const t_weight infinity = std::numeric_limits<t_weight>::max() / 2;
	for(std::size_t vertidx=0; vertidx<N; ++vertidx)
		dists[vertidx] = (vertidx==startidx ? 0 : infinity);


	// distance priority queue and comparator
	auto vert_cmp = [&dists](std::size_t idx1, std::size_t idx2) -> bool
	{
		return dists[idx1] > dists[idx2];
	};

	std::priority_queue<std::size_t, std::vector<std::size_t>, decltype(vert_cmp)>
		prio{vert_cmp};

	for(std::size_t vertidx=0; vertidx<N; ++vertidx)
		prio.push(vertidx);


	while(prio.size())
	{
		std::size_t vertidx = prio.top();
		prio.pop();

		std::vector<std::size_t> neighbours = graph.GetNeighbours(vertidx);
		for(std::size_t neighbouridx : neighbours)
		{
			t_weight w = graph.GetWeight(vertidx, neighbouridx);

			// is the path from startidx to neighbouridx over vertidx shorter than from startidx to neighbouridx?
			if(dists[vertidx] + w < dists[neighbouridx])
			{
				dists[neighbouridx] = dists[vertidx] + w;
				predecessors[neighbouridx] = vertidx;
			}
		}
	}

	return predecessors;
}


/**
 * bellman-ford algorithm
 * @see (FUH 2021), Kurseinheit 4, p. 13
 */
template<class t_graph, class t_mat=tl2::mat<typename t_graph::t_weight, std::vector>>
requires is_graph<t_graph> && tl2::is_mat<t_mat>
t_mat bellman(const t_graph& graph, const std::string& startvert)
{
	// start index
	auto _startidx = graph.GetVertexIndex(startvert);
	if(!_startidx)
		return t_mat{};
	const std::size_t startidx = *_startidx;


	// distances
	const std::size_t N = graph.GetNumVertices();
	using t_weight = typename t_graph::t_weight;
	t_mat dists = tl2::zero<t_mat>(N, N);

	// don't use the full maximum to prevent overflows when we're adding the weight afterwards
	const t_weight infinity = std::numeric_limits<t_weight>::max() / 2;

	for(std::size_t vertidx=0; vertidx<N; ++vertidx)
		dists(0, vertidx) = (vertidx==startidx ? 0 : infinity);


	// iterate vertices
	for(std::size_t i=1; i<N; ++i)
	{
		for(std::size_t vertidx=0; vertidx<N; ++vertidx)
		{
			dists(i, vertidx) = dists(i-1, vertidx);

			std::vector<std::size_t> neighbours = graph.GetNeighbours(vertidx, false);
			for(std::size_t neighbouridx : neighbours)
			{
				t_weight w = graph.GetWeight(neighbouridx, vertidx);

				if(dists(i-1, neighbouridx) + w < dists(i, vertidx))
				{
					dists(i, vertidx) = dists(i-1, neighbouridx) + w;
				}
			}
		}
	}

	return dists;
}


/**
 * floyd-warshall algorithm
 * @see (FUH 2021), Kurseinheit 4, p. 23
 */
template<class t_graph, class t_mat=tl2::mat<typename t_graph::t_weight, std::vector>>
requires is_graph<t_graph> && tl2::is_mat<t_mat>
t_mat floyd(const t_graph& graph)
{
	// distances
	const std::size_t N = graph.GetNumVertices();
	using t_weight = typename t_graph::t_weight;
	t_mat dists = tl2::zero<t_mat>(N, N);
	t_mat next_dists = tl2::zero<t_mat>(N, N);


	// don't use the full maximum to prevent overflows when we're adding the weight afterwards
	const t_weight infinity = std::numeric_limits<t_weight>::max() / 2;

	// initial weights
	for(std::size_t vertidx1=0; vertidx1<N; ++vertidx1)
	{
		std::vector<std::size_t> neighbours = graph.GetNeighbours(vertidx1);

		for(std::size_t vertidx2=0; vertidx2<N; ++vertidx2)
		{
			if(vertidx2 == vertidx1)
				continue;

			// is vertidx2 a direct neighbour of vertidx1?
			if(std::find(neighbours.begin(), neighbours.end(), vertidx2) != neighbours.end())
				dists(vertidx1, vertidx2) = graph.GetWeight(vertidx1, vertidx2);
			else
				dists(vertidx1, vertidx2) = infinity;
		}
	}


	// iterate vertices
	for(std::size_t i=1; i<N; ++i)
	{
		for(std::size_t vertidx1=0; vertidx1<N; ++vertidx1)
		{
			for(std::size_t vertidx2=0; vertidx2<N; ++vertidx2)
			{
				t_weight dist1 = dists(vertidx1, vertidx2);
				t_weight dist2 = dists(vertidx1, i) + dists(i, vertidx2);

				next_dists(vertidx1, vertidx2) = std::min(dist1, dist2);
			}
		}

		std::swap(dists, next_dists);
	}

	return dists;
}



// ----------------------------------------------------------------------------
// spanning tree
// ----------------------------------------------------------------------------

/**
 * finds loops in an undirected graph
 */
template<class t_edge = std::pair<std::size_t, std::size_t>>
bool has_loops(
	const std::vector<t_edge>& edges, 
	std::size_t start_from, std::size_t start_to)
{
	// [from, to]
	std::stack<t_edge> tovisit;
	tovisit.push(std::make_pair(start_from, start_to));

	std::set<std::size_t> visitedverts;
	visitedverts.insert(start_from);

	std::set<t_edge> visitededges;

	// visit connected vertices
	while(!tovisit.empty())
	{
		auto topedge = tovisit.top();
		auto [vertfrom, vertto] = topedge;
		tovisit.pop();

		if(visitededges.find(topedge) != visitededges.end())
			continue;

		visitededges.insert(std::make_pair(vertfrom, vertto));
		visitededges.insert(std::make_pair(vertto, vertfrom));

		// has this vertex already been visited? => loop in graph
		if(visitedverts.find(vertto) != visitedverts.end())
			return true;

		visitedverts.insert(vertto);

		// get all edges from current vertex
		for(auto iter=edges.begin(); iter!=edges.end(); ++iter)
		{
			// forward direction
			if(iter->first == vertto)
				tovisit.push(std::make_pair(iter->first, iter->second));
			// backward direction
			if(iter->second == vertto)
				tovisit.push(std::make_pair(iter->second, iter->first));
		}
	}

	return false;
}


/**
 * minimal spanning tree
 * @see (FUH 2020), ch. 5.2.3, pp. 221-224
 * @see https://de.wikipedia.org/wiki/Algorithmus_von_Kruskal
 */
template<class t_vec, class t_edge = std::pair<std::size_t, std::size_t>>
std::vector<t_edge> calc_min_spantree(
	const std::vector<t_vec>& verts, 
	const std::vector<t_edge>& _edges)
requires tl2::is_vec<t_vec>
{
	using t_real = typename t_vec::value_type;

	std::vector<t_edge> edges = _edges;

	std::stable_sort(edges.begin(), edges.end(), [&verts](const t_edge& edge1, const t_edge& edge2) -> bool
	{
		t_vec dir1 = verts[edge1.first] - verts[edge1.second];
		t_vec dir2 = verts[edge2.first] - verts[edge2.second];

		t_real len1sq = tl2::inner(dir1, dir1);
		t_real len2sq = tl2::inner(dir2, dir2);

		return len1sq >= len2sq;
	});


	std::vector<t_edge> span;
	span.reserve(edges.size());

	while(edges.size())
	{
		t_edge edge = std::move(edges.back());
		edges.pop_back();

		span.push_back(edge);
		if(has_loops<t_edge>(span, edge.first, edge.second))
			span.pop_back();
	}

	return span;
}


/**
 * minimum spanning tree using boost.graph for comparison
 */
template<class t_vec, class t_edge = std::pair<std::size_t, std::size_t>>
std::vector<t_edge> calc_min_spantree_boost(const std::vector<t_vec>& verts)
requires tl2::is_vec<t_vec>
{
	using t_real = typename t_vec::value_type;

	struct t_edge_weight
	{
		t_edge_weight() = default;
		t_edge_weight(t_real weight) : weight(weight) {}

		t_real weight{1};
	};

	using t_graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, t_vec, t_edge_weight>;
	using t_edge_descr = typename boost::graph_traits<t_graph>::edge_descriptor;

	t_graph graph;
	auto weight = boost::get(&t_edge_weight::weight, graph);

	for(const t_vec& vert : verts)
		boost::add_vertex(vert, graph);

	for(std::size_t i=0; i<verts.size(); ++i)
	{
		const t_vec& vert1 = verts[i];
		for(std::size_t j=i+1; j<verts.size(); ++j)
		{
			const t_vec& vert2 = verts[j];
			t_real dist = tl2::norm(vert2-vert1);
			boost::add_edge(boost::vertex(i, graph), boost::vertex(j, graph), t_edge_weight{dist}, graph);
		}
	}

	std::vector<t_edge_descr> spanning_edges;
	boost::kruskal_minimum_spanning_tree(graph, std::back_inserter(spanning_edges), boost::weight_map(weight));

	std::vector<t_edge> span;
	for(auto iter=spanning_edges.begin(); iter!=spanning_edges.end(); ++iter)
	{
		std::size_t idx1 = boost::source(*iter, graph);
		std::size_t idx2 = boost::target(*iter, graph);
		span.emplace_back(std::make_pair(idx1, idx2));
	}

	return span;
}

// ----------------------------------------------------------------------------


}
#endif
