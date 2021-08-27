/**
 * TAS path tool, process helpers
 * @author Tobias Weber <tweber@ill.fr>
 * @date aug-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TASPATHS_PROC_H__
#define __TASPATHS_PROC_H__

#include <string>


/**
 * start a sub-process
 */
extern void create_process(const std::string& binary);


#endif
