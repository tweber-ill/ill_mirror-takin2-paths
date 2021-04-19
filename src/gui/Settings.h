/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TASPATHS_SETTINGS__
#define __TASPATHS_SETTINGS__

#include <string>
#include "tlibs2/libs/glplot.h"

// application binary path
extern std::string g_apppath;

// renderer fps
extern unsigned int g_timer_fps;

// camera translation scaling factor
extern tl2::t_real_gl g_move_scale;

// camera rotation scaling factor
extern tl2::t_real_gl g_rotation_scale;


/**
 * get the path to a resource file
 */
extern std::string find_resource(const std::string& resfile);

#endif
