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
	//#include "src/core/PathsBuilder.h"
%}

// standard includes: /usr/share/swig/4.0.2/python/
%include <std_pair.i>
%include <std_array.i>
%include <std_vector.i>
%include <std_map.i>
%include <std_unordered_map.i>
%include <std_string.i>

%template(PairBoolString) std::pair<bool, std::string>;

%include "src/core/types.h"
%include "src/core/Geometry.h"
%include "src/core/Axis.h"
%include "src/core/Instrument.h"
%include "src/core/InstrumentSpace.h"

