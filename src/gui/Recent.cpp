/**
 * recent files
 * @author Tobias Weber <tweber@ill.fr>
 * @date nov-2021
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

#include "Recent.h"

#include <QtCore/QObject>
#include <QtCore/QFile>


/**
 * adds a file to the recent files menu
 */
void RecentFiles::AddRecentFile(const QString &file)
{
	for(const auto &recentfile : m_recentFiles)
	{
		// file already in list?
		if(recentfile == file)
			return;
	}

	m_recentFiles.push_back(file);
	RebuildRecentFiles();
}


/**
 * sets the recent file menu
 */
void RecentFiles::SetRecentFiles(const QStringList &files)
{
	m_recentFiles = files;
	RebuildRecentFiles();
}


/**
 * creates the "recent files" sub-menu
 */
void RecentFiles::RebuildRecentFiles()
{
	m_menuOpenRecent->clear();

	std::size_t num_recent_files = 0;
	for(auto iter = m_recentFiles.rbegin(); iter != m_recentFiles.rend();)
	{
		QString filename = *iter;

		// remove recent file entries which do not exist anymore
		if(!QFile::exists(filename))
		{
			// get corresponding forward iterator
			auto iter_fwd = (iter+1).base();
			++iter;
			m_recentFiles.erase(iter_fwd);
			continue;
		}

		auto *acFile = new QAction(
			QIcon::fromTheme("document"), filename, m_menuOpenRecent);

		QObject::connect(acFile, &QAction::triggered, [this, filename]()
		{
			if(!m_open_func)
				return;
			(*m_open_func)(filename);
		});
		m_menuOpenRecent->addAction(acFile);

		if(++num_recent_files >= m_maxRecentFiles)
			break;

		++iter;
	}
}


/**
 * remove superfluous entries
 */
void RecentFiles::TrimEntries()
{
	// remove superfluous entries and save the recent files list
	while((std::size_t)m_recentFiles.size() > m_maxRecentFiles)
		m_recentFiles.pop_front();
}
