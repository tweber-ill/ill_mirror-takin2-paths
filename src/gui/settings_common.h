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

#ifndef __TASPATHS_SETTINGS_VARIABLES_COMMON__
#define __TASPATHS_SETTINGS_VARIABLES_COMMON__

#include <variant>

#include "src/core/types.h"



// ----------------------------------------------------------------------------
// settings variable struct
// ----------------------------------------------------------------------------
enum class SettingsVariableEditor
{
	NONE,
	YESNO,
	COMBOBOX,
};


struct SettingsVariable
{
	const char* description{};
	const char* key{};
	std::variant<t_real*, int*, unsigned int*> value{};
	bool is_angle{false};
	SettingsVariableEditor editor{SettingsVariableEditor::NONE};
};
// ----------------------------------------------------------------------------


#endif
