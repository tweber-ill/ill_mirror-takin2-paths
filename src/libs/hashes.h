/**
 * hash functions
 * @author Tobias Weber <tweber@ill.fr>
 * @date July-2021
 * @note Forked on 14-jul-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 * @license GPLv3, see 'LICENSE' file
 *
 * references:
 *	- https://www.boost.org/doc/libs/1_76_0/doc/html/hash/combine.html
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite) and private "Geo" project
 * Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL), 
 *                     Grenoble, France).
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

#ifndef __GEO_HELPERS_H__
#define __GEO_HELPERS_H__

#include <unordered_set>
#include <functional>

#include <boost/functional/hash.hpp>


namespace geo {


/**
 * get the hash of a value
 */
template<class T>
std::size_t unordered_hash(const T& t)
{
	return std::hash<T>{}(t);
}


/**
 * get the combined hash of several values,
 * where the order doesn't matter
 */
template<class T1, class ...T2>
std::size_t unordered_hash(const T1& t1, const T2& ...t2)
{
	std::size_t hash1 = unordered_hash<T1>(t1);
	std::size_t hash2 = unordered_hash<T2...>(t2...);

	std::size_t hash_comb1 = 0;
	boost::hash_combine(hash_comb1, hash1);
	boost::hash_combine(hash_comb1, hash2);

	std::size_t hash_comb2 = 0;
	boost::hash_combine(hash_comb2, hash2);
	boost::hash_combine(hash_comb2, hash1);

	// order matters for boost::hash_combine -> sort hashes to make it unordered
	if(hash_comb2 < hash_comb1)
		std::swap(hash_comb1, hash_comb2);

	std::size_t hash_comb = 0;
	boost::hash_combine(hash_comb, hash_comb1);
	boost::hash_combine(hash_comb, hash_comb2);

	return hash_comb;
}


}
#endif
