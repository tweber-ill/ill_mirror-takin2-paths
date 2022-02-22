/**
 * algorithms on property trees
 * @author Tobias Weber <tweber@ill.fr>
 * @date March 2021
 * @note Forked on 20-mar-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 * @license GPLv3, see 'LICENSE' file
 *
 * References:
 *  * https://www.boost.org/doc/libs/1_77_0/doc/html/property_tree.html
 *  * https://www.boost.org/doc/libs/1_77_0/doc/html/string_algo.html
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

#ifndef __PTREE_ALGOS_H__
#define __PTREE_ALGOS_H__

#include <string>
#include <optional>

#include <boost/algorithm/string.hpp>

#include "tlibs2/libs/expr.h"
#include "tlibs2/libs/str.h"


/**
 * @brief replaces all values in a property tree
 */
template<class t_prop, class t_map>
void replace_ptree_values(t_prop& prop, const t_map& map)
{
	const auto& replace_str = [&map](std::string& str)
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


/**
 * @brief parse an expression string from a property tree
 */
template<class t_var, class t_prop>
std::optional<t_var> parse_ptree_value(
	const t_prop& prop,
	const std::string& key,
	const std::optional<std::string>& default_value = std::nullopt)
{
	try
	{
		auto optVal = prop.template get_optional<std::string>(key);
		if(!optVal)
		{
			if(default_value)
				optVal = *default_value;
			else
				return std::nullopt;
		}

		tl2::ExprParser<t_var> parser;
		if(!parser.parse(*optVal))
			return std::nullopt;

		return parser.eval();
	}
	catch(const std::exception& ex)
	{
		return std::nullopt;
	}
}


#endif
