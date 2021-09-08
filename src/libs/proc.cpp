/**
 * TAS path tool, process helpers
 * @author Tobias Weber <tweber@ill.fr>
 * @date aug-2021
 * @license GPLv3, see 'LICENSE' file
 * @note Forked on 27-aug-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * References:
 *  * https://www.boost.org/doc/libs/1_75_0/doc/html/boost_process/tutorial.html
 *  * https://www.boost.org/doc/libs/1_75_0/doc/html/process.html
 *  * https://www.boost.org/doc/libs/1_75_0/doc/html/boost/process/child.html
 */

#include "proc.h"

#if !defined(__MINGW32__) && !defined(__MINGW64__)
	#include <boost/process.hpp>
	namespace proc = boost::process;
	namespace this_proc = boost::this_process;
#endif


/**
 * start a sub-process
 */
void create_process(const std::string& binary)
{
#if !defined(__MINGW32__) && !defined(__MINGW64__)
	proc::spawn(binary.c_str(), this_proc::environment());
#else
	std::system((binary + "&").c_str());
#endif
}
