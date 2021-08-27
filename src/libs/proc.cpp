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

#include <boost/process.hpp>
#include <boost/property_tree/ptree.hpp>

namespace proc = boost::process;
namespace this_proc = boost::this_process;


/**
 * start a sub-process
 */
void create_process(const std::string& binary)
{
	proc::spawn(binary.c_str(), this_proc::environment());
	//std::system((binary + "&").c_str());
}
