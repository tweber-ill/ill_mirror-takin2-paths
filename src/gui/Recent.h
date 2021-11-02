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

#ifndef __RECENT_FILES_H__
#define __RECENT_FILES_H__

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>

#include <functional>


// ----------------------------------------------------------------------------
class RecentFiles
{
public:
	RecentFiles() = default;
	~RecentFiles() = default;

	RecentFiles(const RecentFiles& other) = default;
	RecentFiles& operator=(const RecentFiles& other) = default;


	// adds a file to the recent files menu
	void AddRecentFile(const QString &file);

	// sets the recent file menu
	void SetRecentFiles(const QStringList &files);
	const QStringList& GetRecentFiles() const { return m_recentFiles; }

	// creates the "recent files" sub-menu
	void RebuildRecentFiles();

	// remove superfluous entries
	void TrimEntries();

	// set the function to be called when the menu element is clicked
	void SetOpenFunc(const std::function<bool(const QString& filename)>* func)
	{ m_open_func = func; }

	void SetRecentFilesMenu(QMenu *menu) { m_menuOpenRecent = menu; };
	QMenu* GetRecentFilesMenu() { return m_menuOpenRecent; }

	void SetMaxRecentFiles(std::size_t num) { m_maxRecentFiles = num; }


private:
	// maximum number of recent files
	std::size_t m_maxRecentFiles{ 16 };

	// recent file menu
	QMenu* m_menuOpenRecent{ nullptr };

	// recent file list and currently active file
	QStringList m_recentFiles{};

	// function to be called when the menu element is clicked
	const std::function<bool(const QString& filename)>* m_open_func{ nullptr };
};
// ----------------------------------------------------------------------------


#endif
