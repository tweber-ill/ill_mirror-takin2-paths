/**
 * global type definitions
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
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

#ifndef __PATHS_GLOBAL_TYPES_H__
#define __PATHS_GLOBAL_TYPES_H__


#include "tlibs2/libs/maths.h"


/**
 * which SSSP algorithm to use for finding the shortest path?
 *  1: standard dijkstra (no negative weights)
 *  2: general dijkstra (which works with negative weights)
 *  3: bellman (very slow!)
 */
#define TASPATHS_SSSP_IMPL 2


/**
 * fixed-size array wrapper
 */
template<class T, std::size_t N>
class t_arr : public std::array<T, N>
{
public:
	using allocator_type = void;


public:
	constexpr t_arr() noexcept
	{
		for(std::size_t i=0; i<this->size(); ++i)
			this->operator[](i) = T{};
	}


	/*constexpr*/ ~t_arr() noexcept = default;


	/**
	 * dummy constructor to fulfill interface requirements
	 */
	constexpr t_arr(std::size_t) noexcept : t_arr{}
	{};


	/**
	 * copy constructor
	 */
	constexpr t_arr(const t_arr& other) noexcept
	{
		this->operator=(other);
	}


	/**
	 * assignment
	 */
	constexpr t_arr& operator=(const t_arr& other) noexcept
	{
		static_cast<std::array<T, N>*>(this)->operator=(other);
		return *this;
	}


	/**
	 * move constructor
	 */
	//t_arr(t_arr&& other) noexcept : std::array<T, N>(std::forward<t_arr&&>(other)) {}
};


template<class T> using t_arr2 = t_arr<T, 2>;
template<class T> using t_arr4 = t_arr<T, 4>;


using t_real = double;

// dynamic container types
using t_vec = tl2::vec<t_real, std::vector>;
using t_vec_int = tl2::vec<int, std::vector>;
using t_mat = tl2::mat<t_real, std::vector>;

// static container types
using t_vec2 = tl2::vec<t_real, t_arr2>;
using t_vec2_int = tl2::vec<int, t_arr2>;
using t_mat22 = tl2::mat<t_real, t_arr4>;


#endif
