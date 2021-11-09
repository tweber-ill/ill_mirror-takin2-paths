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

#include "settings_variables.h"
#include "tlibs2/libs/maths.h"



// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------
// maximum number of threads
unsigned int g_maxnum_threads = 4;

// maximum number of recent files
unsigned int g_maxnum_recents = 16;

// epsilons and precisions
int g_prec = 6;
t_real g_eps = 1e-6;

// gui theme
QString g_theme = "Fusion";

// gui font
QString g_font = "";

// use native menubar?
int g_use_native_menubar = 0;

// use native dialogs?
int g_use_native_dialogs = 1;
// ----------------------------------------------------------------------------
