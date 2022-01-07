/**
 * TAS paths tool -- resource file handling
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @note Forked on 5-November-2021 from my privately developed "qm" project (https://github.com/t-weber/qm).
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

#include "Resources.h"
#include "tlibs2/libs/file.h"


/**
 * add a resource search path entry
 */
void Resources::AddPath(const std::string& pathname)
{
	m_paths.push_back(pathname);
}


/**
 * find a resource file
 */
std::string Resources::FindFile(const std::string& filename) const
{
	fs::path file{filename};

	for(const std::string& pathname : m_paths)
	{
		fs::path path{pathname};
		fs::path respath = path / file;

		if(fs::exists(respath))
			return respath.string();
	}

	return "";
}
