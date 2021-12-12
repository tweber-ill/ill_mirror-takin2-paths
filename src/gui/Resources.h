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

#ifndef __TASPATHS_RESOURCES__
#define __TASPATHS_RESOURCES__

#include <vector>
#include <string>


class Resources
{
public:
	Resources() = default;
	~Resources() = default;

	void AddPath(const std::string& path);
	std::string FindFile(const std::string& file) const;


private:
	std::vector<std::string> m_paths{};
};


#endif
