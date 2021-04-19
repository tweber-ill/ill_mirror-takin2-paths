/**
 * spanning tree
 * @author Tobias Weber <tweber@ill.fr>
 * @date Oct/Nov-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 *
 * References for the algorithms:
 *   - (Klein 2005) "Algorithmische Geometrie" (2005), ISBN: 978-3540209560 (http://dx.doi.org/10.1007/3-540-27619-X).
 *   - (FUH 2020) "Algorithmische Geometrie" (2020), Kurs 1840, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/1840).
 *   - (Berg 2008) "Computational Geometry" (2008), ISBN: 978-3-642-09681-5 (http://dx.doi.org/10.1007/978-3-540-77974-2).
 */

#ifndef __GEO_ALGOS_SPAN_H__
#define __GEO_ALGOS_SPAN_H__

#include <vector>
#include <stack>
#include <set>
#include <algorithm>

#include "tlibs2/libs/maths.h"

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/kruskal_min_spanning_tree.hpp>


namespace geo {
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
		t_vec dir1 = verts[edge1.first]-verts[edge1.second];
		t_vec dir2 = verts[edge2.first]-verts[edge2.second];

		t_real len1sq = tl2::inner(dir1, dir1);
		t_real len2sq = tl2::inner(dir2, dir2);

		return len1sq >= len2sq;
	});


	std::vector<t_edge> span;

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
 * minimum spanning tree
 */
template<class t_vec, class t_edge = std::pair<std::size_t, std::size_t>>
std::vector<t_edge> calc_min_spantree_boost(
	const std::vector<t_vec>& verts)
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
