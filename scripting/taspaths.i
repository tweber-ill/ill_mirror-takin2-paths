/**
 * swig interface
 * @author Tobias Weber <tweber@ill.fr>
 * @date 14-july-2021
 * @license see 'LICENSE' file
 *
 * References:
 *	* http://www.swig.org/tutorial.html
 *	* http://www.swig.org/Doc4.0/SWIGPlus.html#SWIGPlus
 */

%module taspaths
%{
	#include "src/core/types.h"
	#include "src/core/Geometry.h"
	#include "src/core/Axis.h"
	#include "src/core/Instrument.h"
	#include "src/core/InstrumentSpace.h"
	#include "src/core/PathsBuilder.h"
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
%template(VectorPairRealReal) std::vector<std::pair<double, double>>;
//%template(VectorVec) std::vector<t_vec>;

%include "src/core/types.h"
%include "src/core/Geometry.h"
%include "src/core/Axis.h"
%include "src/core/Instrument.h"
%include "src/core/InstrumentSpace.h"
%include "src/core/PathsBuilder.h"
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
