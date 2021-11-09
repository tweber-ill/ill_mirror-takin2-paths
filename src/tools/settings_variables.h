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

#ifndef __TASPATHS_TOOLS_SETTINGS_VARIABLES__
#define __TASPATHS_TOOLS_SETTINGS_VARIABLES__

#include <QtCore/QString>

#include <array>
#include <variant>

#include "src/core/types.h"
#include "settings_common.h"



// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------
// maximum number of threads for calculations
extern unsigned int g_maxnum_threads;

// maximum number of recent files
extern unsigned int g_maxnum_recents;

// number precision
extern int g_prec;

// epsilon
extern t_real g_eps;

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
constexpr std::array<SettingsVariable, 4> g_settingsvariables
{{
	// epsilon
	{
		.description = "Calculation epsilon",
		.key = "settings/eps",
		.value = &g_eps,
	},

	// precision
	{
		.description = "Number precision",
		.key = "settings/prec",
		.value = &g_prec
	},

	// threading options
	{
		.description = "Maximum number of threads",
		.key = "settings/maxnum_threads",
		.value = &g_maxnum_threads},

	// file options
	{
		.description = "Maximum number of recent files",
		.key = "settings/maxnum_recents",
		.value = &g_maxnum_recents
	},
}};
// ----------------------------------------------------------------------------


#endif
