/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Settings.h"
#include "tlibs2/libs/file.h"


std::string g_apppath = ".";

unsigned int g_timer_fps = 30;

tl2::t_real_gl g_move_scale = tl2::t_real_gl(1./75.);
tl2::t_real_gl g_rotation_scale = 0.02;

int g_prec = 6;
int g_prec_gui = 4;

t_real g_eps = 1e-6;
t_real g_eps_gui = 1e-4;

t_real g_a3_offs = tl2::pi<t_real>*0.5;

/**
 * get the path to a resource file
 */
std::string find_resource(const std::string& resfile)
{
	fs::path res = resfile;
	fs::path apppath = g_apppath;

	// iterate possible resource directories
	for(const fs::path& path :
	{
		apppath/"res"/res, apppath/".."/"res"/res,
		apppath/"Resources"/res, apppath/".."/"Resources"/res,
	})
	{
		if(fs::exists(path))
			return path.string();
	}

	return "";
}
