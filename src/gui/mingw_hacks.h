/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TAKIN_PATHS_MINGW_HACKS_H__
#define __TAKIN_PATHS_MINGW_HACKS_H__

#if defined(__MINGW64__) || defined(__MINGW64__)
	// for boost.asio
	#include <winsock2.h>
#endif

#endif
