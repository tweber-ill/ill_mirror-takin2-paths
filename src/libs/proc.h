/**
 * TAS path tool, process helpers
 * @author Tobias Weber <tweber@ill.fr>
 * @date aug-2021
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite) and private "Geo" project
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

#ifndef __TASPATHS_PROC_H__
#define __TASPATHS_PROC_H__

#include <string>


/**
 * start a sub-process
 */
extern void create_process(const std::string& binary);


#endif
