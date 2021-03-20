/**
 * algorithms on property trees
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __PTREE_ALGOS_H__
#define __PTREE_ALGOS_H__

#include <string>
#include <boost/algorithm/string.hpp>


/**
 * @brief replaces all values in a property tree
 */
template<class t_prop, class t_map>
void replace_ptree_values(t_prop& prop, const t_map& map)
{
	const auto& replace_str = [&map](std::string& str) -> void
	{
		for(const auto& pair : map)
			boost::algorithm::replace_all(str, pair.first, pair.second);
	};

	// iterate tree nodes
	for(auto& node : prop)
	{
		//std::cout << node.first << std::endl;
		if(std::string val = node.second.template get<std::string>(""); !val.empty())
		{
			replace_str(val);
			node.second.template put_value<std::string>(val);
		}

		replace_ptree_values(node.second, map);
	}
}


#endif
