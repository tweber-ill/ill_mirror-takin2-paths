/**
 * swig interface
 * @author Tobias Weber <tweber@ill.fr>
 * @date 14-july-2021
 * @license see 'LICENSE' file
 *
 * References:
 *	* http://www.swig.org/tutorial.html
 *	* http://www.swig.org/Doc4.0/SWIGPlus.html#SWIGPlus
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

%module taspaths
%{
	#include "src/core/types.h"
	#include "src/core/Geometry.h"
	#include "src/core/Axis.h"
	#include "src/core/Instrument.h"
	#include "src/core/InstrumentSpace.h"
	#include "src/core/PathsBuilder.h"
	#include "src/core/PathsExporter.h"
	#include "src/core/TasCalculator.h"
%}


// standard includes: /usr/share/swig/4.0.2/python/
%include <std_pair.i>
%include <std_array.i>
%include <std_vector.i>
%include <std_map.i>
%include <std_unordered_map.i>
%include <std_string.i>

%template(PairBoolString) std::pair<bool, std::string>;
%template(PairRealReal) std::pair<double, double>;
%template(ArrayReal4) std::array<double, 4>;
%template(VectorPairRealReal) std::vector<std::pair<double, double>>;
%template(VectorArrayReal4) std::vector<std::array<double, 4>>;
//%template(VectorVec) std::vector<t_vec>;

%include "src/core/types.h"
%include "src/core/Geometry.h"
%include "src/core/Axis.h"
%include "src/core/Instrument.h"
%include "src/core/InstrumentSpace.h"
%include "src/core/PathsBuilder.h"
%include "src/core/PathsExporter.h"
%include "src/core/TasCalculator.h"
//%include "tlibs2/libs/maths.h"


// helper functions
%inline
%{
	/**
	 * simple memory manager for use in scripts
	 */
	class MemManager
	{
		public:
			MemManager()
			{
			}

			~MemManager()
			{
				for(t_real* arr : m_real_arrs)
					delete[] arr;

				m_real_arrs.clear();
			}

			t_real* NewRealArray(std::size_t n)
			{
				t_real* arr = new t_real[n];
				m_real_arrs.push_back(arr);
				return arr;
			}

			void SetRealArray(t_real *arr, std::size_t i, t_real val)
			{
				arr[i] = val;
			}

		private:
			std::vector<t_real*> m_real_arrs;
	};
%}
