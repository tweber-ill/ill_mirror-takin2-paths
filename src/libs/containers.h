/**
 * geometric container data types (trees, graphs, etc.)
 * @author Tobias Weber <tweber@ill.fr>
 * @date November 2020 - June 2021
 * @note Forked on 19-apr-2021 and 3-jun-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 *
 * References:
 *   - (Klein 2005) "Algorithmische Geometrie" (2005), ISBN: 978-3540209560 (http://dx.doi.org/10.1007/3-540-27619-X).
 *   - (FUH 2020) "Algorithmische Geometrie" (2020), Kurs 1840, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/1840).
 *   - (FUH 2021) "Effiziente Algorithmen" (2021), Kurs 1684, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/01684).
 *   - (Berg 2008) "Computational Geometry" (2008), ISBN: 978-3-642-09681-5 (http://dx.doi.org/10.1007/978-3-540-77974-2).
 *   - https://www.boost.org/doc/libs/1_74_0/doc/html/intrusive/node_algorithms.html
 */

#ifndef __GEO_CONTAINERS_H__
#define __GEO_CONTAINERS_H__

#include <type_traits>
#include <concepts>
#include <string>
#include <sstream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <cstdint>
#include <iostream>

#include <boost/intrusive/bstree_algorithms.hpp>
#include <boost/intrusive/avltree_algorithms.hpp>
#include <boost/intrusive/rbtree_algorithms.hpp>
#include <boost/intrusive/splaytree_algorithms.hpp>
#include <boost/intrusive/treap_algorithms.hpp>

#include "tlibs2/libs/maths.h"


/**
 * range tree's underlying binary tree:
 * 	1: binary search tree
 * 	2: avl tree
 * 	3: red-black tree
 * 	4: splay tree
 */
#ifndef __RANGE_TREE_UNDERLYING_IMPL
	#define __RANGE_TREE_UNDERLYING_IMPL 2
#endif


namespace geo {

// ----------------------------------------------------------------------------
// concepts
// ----------------------------------------------------------------------------

// hack for standard libraries where this concept is not yet available
#define __GEO_NO_CONVERTIBLE__

#ifdef __GEO_NO_CONVERTIBLE__
	template<class t_1, class t_2>
	concept convertible_to = std::is_convertible<t_1, t_2>::value;
#else
	template<class t_1, class t_2>
	concept convertible_to = requires { std::convertible_to<t_1, t_2>; };
#endif


/**
 * requirements for a basic vector container like std::vector
 */
template<class T>
concept is_tree_node = requires(const T& a)
{
	// tree hierarchy
	a.parent;
	a.left;
	a.right;
};


/**
 * requirements for the graph container interface
 */
template<class t_graph>
concept is_graph = requires(t_graph& graph, std::size_t vertidx)
{
	{ graph.GetNumVertices() }
		-> convertible_to<std::size_t>;

	{ graph.GetVertexIdent(vertidx) }
		-> convertible_to<std::string>;

	{ graph.GetWeight(vertidx, vertidx) }
		-> convertible_to<typename t_graph::t_weight>;

	graph.GetNeighbours(vertidx);

	graph.AddVertex("123");
	graph.AddEdge(0, 1);
	graph.AddEdge("1", "2");
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// common classes / functions
// ----------------------------------------------------------------------------

/**
 * common node type
 */
template<class t_nodetype>
struct CommonTreeNode
{
	// tree hierarchy
	t_nodetype *parent = nullptr;
	t_nodetype *left = nullptr;
	t_nodetype *right = nullptr;


	CommonTreeNode() {}

	CommonTreeNode(const CommonTreeNode<t_nodetype>& other)
	{
		*this = operator=(other);
	}

	const CommonTreeNode<t_nodetype>&
	operator=(const CommonTreeNode<t_nodetype>& other)
	{
		this->parent = other.parent;
		this->left = other.left;
		this->right = other.right;
	}
};


/**
 * common node traits
 * @see https://www.boost.org/doc/libs/1_74_0/doc/html/intrusive/node_algorithms.html
 */
template<class t_tree_node>
requires is_tree_node<t_tree_node>
struct BasicNodeTraits
{
	using node = t_tree_node;
	using node_ptr = node*;
	using const_node_ptr = const node*;

	static node* get_parent(const node* thenode)
	{
		if(!thenode) return nullptr;
		return thenode->parent;
	}

	static node* get_left(const node* thenode)
	{
		if(!thenode) return nullptr;
		return thenode->left;
	}

	static node* get_right(const node* thenode)
	{
		if(!thenode) return nullptr;
		return thenode->right;
	}

	static void set_parent(node* thenode, node* parent)
	{
		if(!thenode) return;
		thenode->parent = parent;
	}

	static void set_left(node* thenode, node* left)
	{
		if(!thenode) return;
		thenode->left = left;
	}

	static void set_right(node* thenode, node* right)
	{
		if(!thenode) return;
		thenode->right = right;
	}
};


/**
 * node traits
 * @see https://www.boost.org/doc/libs/1_74_0/doc/html/intrusive/node_algorithms.html
 */
template<class t_tree_node>
requires is_tree_node<t_tree_node>
struct BinTreeNodeTraits : public BasicNodeTraits<t_tree_node>
{
	using node = typename BasicNodeTraits<t_tree_node>::node;
	using node_ptr = typename BasicNodeTraits<t_tree_node>::node_ptr;
	using const_node_ptr = typename BasicNodeTraits<t_tree_node>::const_node_ptr;

	using balance = typename node::t_balance;

	static balance positive() { return 1; }
	static balance negative() { return -1; }
	static balance zero() { return 0; }

	static balance get_balance(const node* thenode)
	{
		if(!thenode) return zero();
		return thenode->balance;
	}

	static void set_balance(node* thenode, balance bal)
	{
		if(!thenode) return;
		thenode->balance = bal;
	}
};


/**
 * node traits
 * @see https://www.boost.org/doc/libs/1_74_0/doc/html/intrusive/node_algorithms.html
 */
template<class t_tree_node>
requires is_tree_node<t_tree_node>
struct RbTreeNodeTraits : public BasicNodeTraits<t_tree_node>
{
	using node = typename BasicNodeTraits<t_tree_node>::node;
	using node_ptr = typename BasicNodeTraits<t_tree_node>::node_ptr;
	using const_node_ptr = typename BasicNodeTraits<t_tree_node>::const_node_ptr;

	using color = typename node::t_colour;

	static color red() { return 1; }
	static color black() { return 0; }

	static color get_color(const node* thenode)
	{
		if(!thenode) return black();
		return thenode->colour;
	}

	static void set_color(node* thenode, color col)
	{
		if(!thenode) return;
		thenode->colour = col;
	}
};


template<class t_node>
void write_graph(std::ostream& ostrStates, std::ostream& ostrTransitions,
	const std::unordered_map<const t_node*, std::size_t>& nodeMap, const t_node* node)
requires is_tree_node<t_node>
{
	using namespace tl2_ops;
	if(!node) return;

	std::size_t num = nodeMap.find(node)->second;
	ostrStates << "\t" << num << " [label=\"" << num << "\"];\n";

	if(node->left)
	{
		std::size_t numleft = nodeMap.find(node->left)->second;
		ostrTransitions << "\t" << num << ":sw -> " << numleft << ":n [label=\"l\"];\n";

		write_graph<t_node>(ostrStates, ostrTransitions, nodeMap, node->left);
	}

	if(node->right)
	{
		std::size_t numright = nodeMap.find(node->right)->second;
		ostrTransitions << "\t" << num << ":se -> " << numright << ":n [label=\"r\"];\n";

		write_graph<t_node>(ostrStates, ostrTransitions, nodeMap, node->right);
	}
}


template<class t_node>
void number_nodes(std::unordered_map<const t_node*, std::size_t>& map, const t_node* node, std::size_t &num)
requires is_tree_node<t_node>
{
	if(!node) return;

	if(auto iter = map.find(node); iter==map.end())
		map.emplace(std::make_pair(node, num++));

	number_nodes<t_node>(map, node->left, num);
	number_nodes<t_node>(map, node->right, num);
}


template<class t_node>
void write_graph(std::ostream& ostr, const t_node* node)
requires is_tree_node<t_node>
{
	std::ostringstream ostrStates;
	std::ostringstream ostrTransitions;

	ostrStates.precision(ostr.precision());
	ostrTransitions.precision(ostr.precision());

	std::unordered_map<const t_node*, std::size_t> nodeNumbers;
	std::size_t nodeNum = 0;
	number_nodes(nodeNumbers, node, nodeNum);

	write_graph<t_node>(ostrStates, ostrTransitions, nodeNumbers, node);

	ostr << "// directed graph\ndigraph tree\n{\n\t// states\n";
	ostr << ostrStates.str();
	ostr << "\n\t// transitions\n";
	ostr << ostrTransitions.str();
	ostr << "\n}\n";
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// range tree
// see (Klein 2005), ch. 3.3.2 pp. 135f.
// see (Berg 2008), pp. 105-110.
// ----------------------------------------------------------------------------

template<class t_vec> requires tl2::is_basic_vec<t_vec> class RangeTree;

/**
 * range tree node
 */
template<class t_vec>
requires tl2::is_basic_vec<t_vec>
struct RangeTreeNode : public CommonTreeNode<RangeTreeNode<t_vec>>
{
	using t_real = typename t_vec::value_type;

#if __RANGE_TREE_UNDERLYING_IMPL==1 || __RANGE_TREE_UNDERLYING_IMPL==2
	// bstree or avltree
	using t_balance = std::int64_t;
	t_balance balance = 0;
#elif __RANGE_TREE_UNDERLYING_IMPL==3
	// rbtree
	using t_colour = std::int8_t;
	t_colour colour = 0;
#endif

	// range tree for idx+1
	std::shared_ptr<RangeTree<t_vec>> nextidxtree{};

	// dimension of data and current index
	std::size_t dim = 0;
	std::size_t idx = 0;

	// range for current index
	t_real range[2] = { 0., 0. };

	// pointer to actual data
	std::shared_ptr<const t_vec> vec{};


	RangeTreeNode() {}

	RangeTreeNode(
		const std::shared_ptr<const t_vec>& vec,
		std::size_t dim, std::size_t idx=0)
			: CommonTreeNode<RangeTreeNode<t_vec>>{},
				dim{dim}, idx{idx}, vec{vec}
	{}


	/**
	 * get all node vectors in a linear fashion
	 */
	static void get_vecs(
		const RangeTreeNode<t_vec>* node,
		std::vector<std::shared_ptr<const t_vec>>& vecs,
		const t_vec* min=nullptr, const t_vec* max=nullptr)
	{
		auto is_in_range = [](
			const t_vec& vec,
			const t_vec& min, const t_vec& max,
			std::size_t dim) -> bool
		{
			for(std::size_t idx=0; idx<dim; ++idx)
			{
				if(vec[idx] < min[idx] || vec[idx] > max[idx])
					return false;
			}
			return true;
		};

		if(node->left)
			get_vecs(node->left, vecs, min, max);

		bool in_range = true;
		if(min && max)
			in_range = is_in_range(*node->vec, *min, *max, node->dim);
		if(in_range)
			vecs.push_back(node->vec);

		if(node->right)
			get_vecs(node->right, vecs, min, max);
	}


	void print(std::ostream& ostr, std::size_t indent=0) const
	{
		using namespace tl2_ops;
		const RangeTreeNode<t_vec> *node = this;

		ostr << "ptr: " << (void*)node;
		ostr << ", vec: ";
		if(node->vec)
			ostr << *node->vec;
		else
			ostr << "null";
		ostr << ", idx: " << node->idx;
		ostr << ", range: " << node->range[0] << ".." << node->range[1] << "\n";

		if(node->left || node->right)
		{
			for(std::size_t i=0; i<indent+1; ++i)
				ostr << "  ";
			ostr << "left: ";
			if(node->left)
				node->left->print(ostr, indent+1);
			else
				ostr << "nullptr\n";

			for(std::size_t i=0; i<indent+1; ++i)
				ostr << "  ";
			ostr << "right: ";
			if(node->right)
				node->right->print(ostr, indent+1);
			else
				ostr << "nullptr\n";
		}
	}


	friend std::ostream& operator<<(std::ostream& ostr, const RangeTreeNode<t_vec>& node)
	{
		node.print(ostr);
		return ostr;
	}


	friend bool operator<(const RangeTreeNode<t_vec>& e1, const RangeTreeNode<t_vec>& e2)
	{
		// compare by idx
		return (*e1.vec)[e1.idx] < (*e2.vec)[e2.idx];
	}
};


/**
 * k-dim range tree
 * @see (Klein 2005), ch. 3.3.2, pp. 135f
 * @see (Berg 2008), pp. 105-110
 */
template<class t_vec>
requires tl2::is_basic_vec<t_vec>
class RangeTree
{
public:
	using t_node = RangeTreeNode<t_vec>;

#if __RANGE_TREE_UNDERLYING_IMPL==1
	// bstree
	using t_nodetraits = BinTreeNodeTraits<t_node>;
	using t_treealgos = boost::intrusive::bstree_algorithms<t_nodetraits>;
#elif __RANGE_TREE_UNDERLYING_IMPL==2
	// avltree
	using t_nodetraits = BinTreeNodeTraits<t_node>;
	using t_treealgos = boost::intrusive::avltree_algorithms<t_nodetraits>;
#elif __RANGE_TREE_UNDERLYING_IMPL==3
	// rbtree
	using t_nodetraits = RbTreeNodeTraits<t_node>;
	using t_treealgos = boost::intrusive::rbtree_algorithms<t_nodetraits>;
#elif __RANGE_TREE_UNDERLYING_IMPL==4
	// sgtree
	using t_nodetraits = BasicNodeTraits<t_node>;
	using t_treealgos = boost::intrusive::splaytree_algorithms<t_nodetraits>;
#endif


public:
	RangeTree(std::size_t idx=0) : m_idx{idx}
	{
		t_treealgos::init_header(&m_root);
	}

	~RangeTree()
	{
		free_nodes(get_root());
	}


	/**
	 * query a rectangular range
	 */
	std::vector<std::shared_ptr<const t_vec>>
	query_range(const t_vec& _min, const t_vec& _max)
	{
		auto is_in_range =
		[](const t_node* node, const t_vec& min, const t_vec& max) -> bool
		{
			const std::size_t idx = node->idx;
			return node->range[0] <= min[idx] && node->range[1] >= max[idx];
		};

		const t_node* node = get_root();
		t_vec min = _min, max = _max;

		// iterate coordinate sub-trees
		while(true)
		{
			// fit query rectangle to range
			if(min[node->idx] < node->range[0])
				min[node->idx] = node->range[0];
			if(max[node->idx] > node->range[1])
				max[node->idx] = node->range[1];

			if(!is_in_range(node, min, max))
			{
				return {};
			}
			else
			{
				// descend tree to find the smallest fitting range
				while(1)
				{
					bool updated = false;
					if(node->left && is_in_range(node->left, min, max))
					{
						node = node->left;
						updated = true;
					}
					else if(node->right && is_in_range(node->right, min, max))
					{
						node = node->right;
						updated = true;
					}

					// no more updates
					if(!updated)
						break;
				}
			}

			if(!node->nextidxtree)
				break;

			node = node->nextidxtree->get_root();
			if(!node)
				break;
		}

		std::vector<std::shared_ptr<const t_vec>> vecs;
		t_node::get_vecs(node, vecs, &min, &max);
		return vecs;
	}


	/**
	 * insert a collection of vectors
	 */
	void insert(const std::vector<t_vec>& vecs)
	{
		for(const t_vec& vec : vecs)
			insert(vec);
		update();
	}

	/**
	 * insert a collection of vectors
	 */
	void insert(const std::vector<std::shared_ptr<const t_vec>>& vecs)
	{
		for(const std::shared_ptr<const t_vec>& vec : vecs)
			insert(vec);
		update();
	}

	/**
	 * insert a vector
	 */
	void insert(const t_vec& vec)
	{
		t_node* node = new t_node{std::make_shared<t_vec>(vec), vec.size(), m_idx};
		insert(node);
	}

	/**
	 * insert a vector
	 */
	void insert(const std::shared_ptr<const t_vec>& vec)
	{
		t_node* node = new t_node{vec, vec->size(), m_idx};
		insert(node);
	}


	const t_node* get_root() const
	{
		return t_treealgos::root_node(&m_root);
	}

	t_node* get_root()
	{
		return t_treealgos::root_node(&m_root);
	}


	void update()
	{
		update(get_root());
	}


	static void update(t_node* node)
	{
		if(!node) return;

		t_node* left = node->left;
		t_node* right = node->right;

		if(left) update(left);
		if(right) update(right);


		// --------------------------------------------------------------------
		// ranges
		// --------------------------------------------------------------------
		// leaf node
		if(!left && !right && node->vec)
			node->range[0] = node->range[1] = (*node->vec)[node->idx];

		if(left && !right)
		{
			node->range[0] = left->range[0];
			if(node->vec)
				node->range[1] = (*node->vec)[node->idx];
			else
				node->range[1] = left->range[1];
		}

		if(right && !left)
		{
			node->range[1] = right->range[1];
			if(node->vec)
				node->range[0] = (*node->vec)[node->idx];
			else
				node->range[0] = right->range[0];
		}

		if(left && right)
		{
			node->range[0] = left->range[0];
			node->range[1] = right->range[1];
		}
		// --------------------------------------------------------------------


		// --------------------------------------------------------------------
		// subtree for next index
		// --------------------------------------------------------------------
		if(node->idx+1 < node->dim)
		{
			node->nextidxtree = std::make_shared<RangeTree<t_vec>>(node->idx+1);

			std::vector<std::shared_ptr<const t_vec>> vecs;
			RangeTreeNode<t_vec>::get_vecs(node, vecs);

			node->nextidxtree->insert(vecs);
		}
		// --------------------------------------------------------------------
	}


	friend std::ostream& operator<<(std::ostream& ostr, const RangeTree<t_vec>& tree)
	{
		ostr << *tree.get_root();
		return ostr;
	}


protected:
	/**
	 * insert a node
	 */
	void insert(t_node* node)
	{
		t_treealgos::insert_equal(&m_root, get_root(), node,
			[](const t_node* node1, const t_node* node2) -> bool
			{
				return *node1 < *node2;
			});
	}


	static void free_nodes(t_node* node)
	{
		if(!node) return;

		free_nodes(node->left);
		free_nodes(node->right);

		delete node;
	}


private:
	t_node m_root{};
	std::size_t m_idx = 0;
};

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// treap
// ----------------------------------------------------------------------------

template<class t_vec>
requires tl2::is_basic_vec<t_vec>
struct TreapNode : public CommonTreeNode<TreapNode<t_vec>>
{
	// pointer to actual data
	std::shared_ptr<const t_vec> vec{};
};


template<class t_vec>
requires tl2::is_basic_vec<t_vec>
using TreapNodeTraits = BasicNodeTraits<TreapNode<t_vec>>;


/**
 * 2-dim treap: tree in first component, heap in second component
 * @see https://en.wikipedia.org/wiki/Treap
 * @see (Berg 2008), pp. 226-230
 * @see (FUH 2020), ch. 4.2.4, pp. 188-192
 */
template<class t_vec>
requires tl2::is_basic_vec<t_vec>
class Treap
{
public:
	using t_node = TreapNode<t_vec>;
	using t_nodetraits = TreapNodeTraits<t_vec>;
	using t_treealgos = boost::intrusive::treap_algorithms<t_nodetraits>;


public:
	Treap()
	{
		t_treealgos::init_header(&m_root);
	}

	~Treap()
	{
		free_nodes(get_root());
	}


	/**
	 * insert a collection of vectors
	 */
	void insert(const std::vector<t_vec>& vecs)
	{
		for(const t_vec& vec : vecs)
			insert(vec);
	}

	/**
	 * insert a vector
	 */
	void insert(const t_vec& vec)
	{
		t_node* node = new t_node;
		node->vec = std::make_shared<t_vec>(vec);
		insert(node);
	}


	const t_node* get_root() const
	{
		return t_treealgos::root_node(&m_root);
	}

	t_node* get_root()
	{
		return t_treealgos::root_node(&m_root);
	}


protected:
	/**
	 * insert a node
	 */
	void insert(t_node* node)
	{
		t_treealgos::insert_equal(&m_root, get_root(), node,
			[](const t_node* node1, const t_node* node2) -> bool
			{	// sorting for first component (tree)
				return (*node1->vec)[0] < (*node2->vec)[0];
			},
			[](const t_node* node1, const t_node* node2) -> bool
			{	// sorting for second component (heap)
				return (*node1->vec)[1] < (*node2->vec)[1];
			});
	}


	static void free_nodes(t_node* node)
	{
		if(!node) return;

		free_nodes(node->left);
		free_nodes(node->right);

		delete node;
	}


private:
	t_node m_root{};
	std::size_t m_idx = 0;
};

// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// graph containers
// ----------------------------------------------------------------------------

/**
 * adjacency matrix
 * @see (FUH 2021), Kurseinheit 4, pp. 3-5
 * @see https://en.wikipedia.org/wiki/Adjacency_matrix
 */
template<class _t_weight = unsigned int>
class AdjacencyMatrix
{
public:
	using t_weight = _t_weight;
	using t_mat = tl2::mat<t_weight, std::vector>;


public:
	AdjacencyMatrix() = default;
	~AdjacencyMatrix() = default;


	std::size_t GetNumVertices() const
	{
		return m_mat.size1();
	}


	const std::string& GetVertexIdent(std::size_t i) const
	{
		return m_vertexidents[i];
	}


	std::optional<std::size_t> GetVertexIndex(const std::string& vert) const
	{
		auto iter = std::find(m_vertexidents.begin(), m_vertexidents.end(), vert);
		if(iter == m_vertexidents.end())
			return std::nullopt;

		return iter - m_vertexidents.begin();
	}


	void AddVertex(const std::string& id)
	{
		t_mat matNew = tl2::zero<t_mat>(m_mat.size1()+1, m_mat.size2()+1);

		for(std::size_t i=0; i<m_mat.size1(); ++i)
			for(std::size_t j=0; j<m_mat.size2(); ++j)
				matNew(i,j) = m_mat(i,j);

		m_mat = std::move(matNew);
		m_vertexidents.push_back(id);
	}


	void RemoveVertex(std::size_t idx)
	{
		m_mat = tl2::submat<t_mat>(m_mat, idx, idx);
		m_vertexidents.erase(m_vertexidents.begin() + idx);
	}


	void RemoveVertex(const std::string& id)
	{
		std::size_t idx = GetVertexIndex(id);
		RemoveVertex(idx);
	}


	void SetWeight(std::size_t idx1, std::size_t idx2, t_weight w)
	{
		m_mat(idx1, idx2) = w;
	}


	void SetWeight(const std::string& vert1, const std::string& vert2, t_weight w)
	{
		auto idx1 = GetVertexIndex(vert1);
		auto idx2 = GetVertexIndex(vert2);

		if(idx1 && idx2)
			SetWeight(*idx1, *idx2, w);
	}


	t_weight GetWeight(std::size_t idx1, std::size_t idx2) const
	{
		return m_mat(idx1, idx2);
	}


	t_weight GetWeight(const std::string& vert1, const std::string& vert2) const
	{
		auto idx1 = GetVertexIndex(vert1);
		auto idx2 = GetVertexIndex(vert2);

		if(idx1 && idx2)
		return GetWeight(*idx1, *idx2);

		return t_weight{0};
	}


	void AddEdge(std::size_t idx1, std::size_t idx2, t_weight w=0)
	{
		SetWeight(idx1, idx2, w);
	}


	void AddEdge(const std::string& vert1, const std::string& vert2, t_weight w=0)
	{
		SetWeight(vert1, vert2, w);
	}


	std::vector<std::tuple<std::size_t, std::size_t, t_weight>> GetEdges() const
	{
		std::vector<std::tuple<std::size_t, std::size_t, t_weight>> edges;
		edges.reserve(m_mat.size1() * m_mat.size2());

		for(std::size_t i=0; i<m_mat.size1(); ++i)
		{
			for(std::size_t j=0; j<m_mat.size2(); ++j)
			{
				if(GetWeight(i, j))
					edges.emplace_back(std::make_tuple(i, j, m_mat(i, j)));
			}
		}

		return edges;
	}


	void RemoveEdge(const std::string& vert1, const std::string& vert2)
	{
		SetWeight(vert1, vert2, t_weight{0});
	}


	bool IsAdjacent(std::size_t idx1, std::size_t idx2) const
	{
		return GetWeight(idx1, idx2) != t_weight{0};
	}


	bool IsAdjacent(const std::string& vert1, const std::string& vert2) const
	{
		return GetWeight(vert1, vert2) != t_weight{0};
	}


	std::vector<std::size_t> GetNeighbours(std::size_t idx, bool outgoing_edges=true) const
	{
		std::vector<std::size_t> neighbours;

		// neighbour vertices on outgoing edges
		if(outgoing_edges)
		{
			for(std::size_t idxOther=0; idxOther<m_mat.size2(); ++idxOther)
			{
				if(GetWeight(idx, idxOther))
					neighbours.push_back(idxOther);
			}
		}

		// neighbour vertices on incoming edges
		else
		{
			for(std::size_t idxOther=0; idxOther<m_mat.size1(); ++idxOther)
			{
				if(GetWeight(idxOther, idx))
					neighbours.push_back(idxOther);
			}
		}

		return neighbours;
	}


	std::vector<std::string> GetNeighbours(const std::string& vert, bool outgoing_edges=true) const
	{
		auto iter = std::find(m_vertexidents.begin(), m_vertexidents.end(), vert);
		if(iter == m_vertexidents.end())
			return {};
		std::size_t idx = iter - m_vertexidents.begin();

		std::vector<std::string> neighbours;

		// neighbour vertices on outgoing edges
		if(outgoing_edges)
		{
			for(std::size_t idxOther=0; idxOther<m_mat.size2(); ++idxOther)
			{
				if(GetWeight(idx, idxOther))
					neighbours.push_back(m_vertexidents[idxOther]);
			}
		}

		// neighbour vertices on incoming edges
		else
		{
			for(std::size_t idxOther=0; idxOther<m_mat.size1(); ++idxOther)
			{
				if(GetWeight(idxOther, idx))
					neighbours.push_back(m_vertexidents[idxOther]);
			}
		}

		return neighbours;
	}


private:
	std::vector<std::string> m_vertexidents{};
	t_mat m_mat{};
};



/**
 * adjacency list
 * @see (FUH 2021), Kurseinheit 4, pp. 3-5
 * @see https://en.wikipedia.org/wiki/Adjacency_list
 */
template<class _t_weight = unsigned int>
class AdjacencyList
{
public:
	using t_weight = _t_weight;


public:
	AdjacencyList() = default;
	~AdjacencyList() = default;


	std::size_t GetNumVertices() const
	{
		return m_vertexidents.size();
	}


	const std::string& GetVertexIdent(std::size_t i) const
	{
		return m_vertexidents[i];
	}


	std::optional<std::size_t> GetVertexIndex(const std::string& vert) const
	{
		auto iter = std::find(m_vertexidents.begin(), m_vertexidents.end(), vert);
		if(iter == m_vertexidents.end())
			return std::nullopt;

		return iter - m_vertexidents.begin();
	}


	void AddVertex(const std::string& id)
	{
		m_vertexidents.push_back(id);
		m_nodes.push_back(nullptr);
	}


	void RemoveVertex(std::size_t idx)
	{
		m_vertexidents.erase(m_vertexidents.begin() + idx);
		m_nodes.erase(m_nodes.begin() + idx);

		for(std::size_t idx1=0; idx1<m_nodes.size(); ++idx1)
		{
			std::shared_ptr<AdjNode> node = m_nodes[idx1];
			std::shared_ptr<AdjNode> node_prev;

			while(node)
			{
				if(node->idx == idx)
				{
					if(node_prev)
						node_prev->next = node->next;
					else
						m_nodes[idx1] = node->next;
					break;
				}

				node_prev = node;
				node = node->next;
			}
		}
	}


	void RemoveVertex(const std::string& id)
	{
		std::size_t idx = GetVertexIndex(id);
		RemoveVertex(idx);
	}


	void SetWeight(std::size_t idx1, std::size_t idx2, t_weight w)
	{
		std::shared_ptr<AdjNode> node = m_nodes[idx1];

		while(node)
		{
			if(node->idx == idx2)
			{
				node->weight = w;
				break;
			}

			node = node->next;
		}
	}


	void SetWeight(const std::string& vert1, const std::string& vert2, t_weight w)
	{
		auto idx1 = GetVertexIndex(vert1);
		auto idx2 = GetVertexIndex(vert2);

		if(idx1 && idx2)
			SetWeight(*idx1, *idx2, w);
	}


	t_weight GetWeight(std::size_t idx1, std::size_t idx2) const
	{
		std::shared_ptr<AdjNode> node = m_nodes[idx1];

		while(node)
		{
			if(node->idx == idx2)
				return node->weight;

			node = node->next;
		}

		return t_weight{};
	}


	t_weight GetWeight(const std::string& vert1, const std::string& vert2) const
	{
		auto idx1 = GetVertexIndex(vert1);
		auto idx2 = GetVertexIndex(vert2);

		if(idx1 && idx2)
			return GetWeight(*idx1, *idx2);

		return t_weight{0};
	}


	void AddEdge(std::size_t idx1, std::size_t idx2, t_weight w=0)
	{
		std::shared_ptr<AdjNode> node = m_nodes[idx1];
		m_nodes[idx1] = std::make_shared<AdjNode>();
		m_nodes[idx1]->next = node;
		m_nodes[idx1]->idx = idx2;
		m_nodes[idx1]->weight = w;
	}


	void AddEdge(const std::string& vert1, const std::string& vert2, t_weight w=0)
	{
		auto idx1 = GetVertexIndex(vert1);
		auto idx2 = GetVertexIndex(vert2);
		if(!idx1 || !idx2)
			return;

		AddEdge(*idx1, *idx2, w);
	}


	void RemoveEdge(const std::string& vert1, const std::string& vert2)
	{
		auto idx1 = GetVertexIndex(vert1);
		auto idx2 = GetVertexIndex(vert2);
		if(!idx1 || !idx2)
			return;

		std::shared_ptr<AdjNode> node = m_nodes[*idx1];
		std::shared_ptr<AdjNode> node_prev;

		while(node)
		{
			if(node->idx == *idx2)
			{
				if(node_prev)
					node_prev->next = node->next;
				else
					m_nodes[*idx1] = node->next;
				break;
			}

			node_prev = node;
			node = node->next;
		}
	}


	bool IsAdjacent(std::size_t idx1, std::size_t idx2) const
	{
		return GetWeight(idx1, idx2) != t_weight{0};
	}


	bool IsAdjacent(const std::string& vert1, const std::string& vert2) const
	{
		return GetWeight(vert1, vert2) != t_weight{0};
	}


	std::vector<std::size_t> GetNeighbours(std::size_t idx, bool outgoing_edges=true) const
	{
		std::vector<std::size_t> neighbours;
		neighbours.reserve(GetNumVertices());

		// neighbour vertices on outgoing edges
		if(outgoing_edges)
		{
			std::shared_ptr<AdjNode> node = m_nodes[idx];

			while(node)
			{
				neighbours.push_back(node->idx);
				node = node->next;
			}
		}

		// neighbour vertices on incoming edges
		else
		{
			for(std::size_t i=0; i<m_nodes.size(); ++i)
			{
				std::shared_ptr<AdjNode> node = m_nodes[i];

				while(node)
				{
					if(node->idx == idx)
					{
						neighbours.push_back(i);
						break;
					}
					node = node->next;
				}
			}
		}

		return neighbours;
	}


	std::vector<std::string> GetNeighbours(const std::string& vert, bool outgoing_edges=true) const
	{
		std::size_t idx = GetVertexIndex(vert);
		std::vector<std::size_t> neighbour_indices = GetNeighbours(idx, outgoing_edges);

		std::vector<std::string> neighbours;
		neighbours.reserve(neighbour_indices.size());

		for(std::size_t neighbour_index : neighbour_indices)
		{
			const std::string& id = GetVertexIdent(neighbour_index);
			neighbours.push_back(id);
		}

		return neighbours;
	}


private:
	struct AdjNode
	{
		std::size_t idx{};
		t_weight weight{};

		std::shared_ptr<AdjNode> next{};
	};

	std::vector<std::string> m_vertexidents{};
	std::vector<std::shared_ptr<AdjNode>> m_nodes{};
};

// ----------------------------------------------------------------------------

}

#endif
