/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "Settings.h"

#if __has_include(<filesystem>) && !defined(__APPLE__)
	#include <filesystem>
	namespace fs = std::filesystem;
#else
	#include <boost/filesystem.hpp>
	namespace fs = boost::filesystem;
#endif


std::string g_apppath = ".";

unsigned int g_timer_fps = 30;

t_real_gl g_move_scale = t_real_gl(1./75.);
t_real_gl g_rotation_scale = 0.02;


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
