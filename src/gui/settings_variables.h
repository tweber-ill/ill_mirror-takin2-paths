/**
 * TAS paths tool -- global settings variables
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
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

#ifndef __TASPATHS_SETTINGS_VARIABLES__
#define __TASPATHS_SETTINGS_VARIABLES__

#include <QtCore/QString>

#include <string>
#include "tlibs2/libs/qt/gl.h"
#include "src/core/types.h"



// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------
// application binary path
extern std::string g_apppath;

// maximum number of threads for calculations
extern unsigned int g_maxnum_threads;


// number precisions
extern int g_prec, g_prec_gui;

// epsilons
extern t_real g_eps, g_eps_angular, g_eps_gui;
extern t_real g_eps_voronoiedge;

// subdivision length of lines for interpolation
extern t_real g_line_subdiv_len;

// crystal angle offset
extern t_real g_a3_offs;

// angular deltas for calculation step width
extern t_real g_a2_delta;
extern t_real g_a4_delta;


// which polygon intersection method should be used?
// 0: sweep, 1: half-plane test
extern int g_poly_intersection_method;

// which backend to use for voronoi diagram calculation?
// 0: boost.polygon, 1: cgal
extern int g_voronoi_backend;

// use region calculation function
extern int g_use_region_function;


// which path finding strategy to use?
// 0: shortest path, 1: avoid walls
extern int g_pathstrategy;

// verify the generated path?
extern int g_verifypath;


// path tracker fps
extern unsigned int g_pathtracker_fps;

// renderer fps
extern unsigned int g_timer_fps;

extern int g_light_follows_cursor;
extern int g_enable_shadow_rendering;

// screenshots
extern int g_combined_screenshots;
extern int g_automatic_screenshots;

// camera translation scaling factor
extern tl2::t_real_gl g_move_scale;

// camera rotation scaling factor
extern tl2::t_real_gl g_rotation_scale;


// gui theme
extern QString g_theme;

// gui font
extern QString g_font;

// use native menubar?
extern int g_use_native_menubar;

// use native dialogs?
extern int g_use_native_dialogs;
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// variables register
// ----------------------------------------------------------------------------
struct SettingsVariable
{
	const char* description{};
	const char* key{};
	std::variant<t_real*, int*, unsigned int*> value{};
	bool is_angle{false};
};


constexpr std::array<SettingsVariable, 22> g_settingsvariables
{{
	// epsilons and precisions
	{.description = "Calculation epsilon", .key = "settings/eps", .value = &g_eps,},
	{.description = "Angular epsilon", .key = "settings/eps_angular", .value = &g_eps_angular, .is_angle = true},
	{.description = "Voronoi edge epsilon", .key = "settings/eps_voronoi_edge", .value = &g_eps_voronoiedge},
	{.description = "Drawing epsilon", .key = "settings/eps_gui", .value = &g_eps_gui},
	{.description = "Number precision", .key = "settings/prec", .value = &g_prec},
	{.description = "GUI number precision", .key = "settings/prec_gui", .value = &g_prec_gui},

	{.description = "Line subdivision length", .key = "settings/line_subdiv_len", .value = &g_line_subdiv_len},

	// threading options
	{.description = "Maximum number of threads", .key = "settings/maxnum_threads", .value = &g_maxnum_threads},

	// angle options
	{.description = "Sample rotation offset", .key = "settings/a3_offs", .value = &g_a3_offs, .is_angle = true},
	{.description = "Monochromator scattering angle delta", .key = "settings/a2_delta", .value = &g_a2_delta, .is_angle = true},
	{.description = "Sample scattering angle delta", .key = "settings/a4_delta", .value = &g_a4_delta, .is_angle = true},

	// mesh options
	{.description = "Polygon intersection method", .key = "settings/poly_inters_method", .value = &g_poly_intersection_method},
	{.description = "Voronoi calculation backend", .key = "settings/voronoi_backend", .value = &g_voronoi_backend},
	{.description = "Use region function", .key = "settings/use_region_function", .value = &g_use_region_function},

	// path options
	{.description = "Path finding strategy", .key = "settings/path_finding_strategy", .value = &g_pathstrategy},
	{.description = "Verify generated path", .key = "settings/verify_path", .value = &g_verifypath},
	{.description = "Path tracker FPS", .key = "settings/pathtracker_fps", .value = &g_pathtracker_fps},

	// renderer options
	{.description = "Renderer FPS", .key = "settings/renderer_fps", .value = &g_timer_fps},
	{.description = "Light follows cursor", .key = "settings/light_follows_cursor", .value = &g_light_follows_cursor},
	{.description = "Enable shadow rendering", .key = "settings/enable_shadow_rendering", .value = &g_enable_shadow_rendering},

	// screenshot options
	{.description = "Combine instrument/configuration space screenshots", .key = "settings/combined_screenshots", .value = &g_combined_screenshots},
	{.description = "Automatically take screenshots (careful!)", .key = "settings/automatic_screenshots", .value = &g_automatic_screenshots},
}};
// ----------------------------------------------------------------------------


#endif
