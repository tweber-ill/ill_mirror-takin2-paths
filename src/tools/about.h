/**
 * geo lib and tools
 * @author Tobias Weber <tweber@ill.fr>
 * @date nov-2020 - jun-2021
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

#ifndef __TAKIN_PATHS_TOOLS_ABOUT_H__
#define __TAKIN_PATHS_TOOLS_ABOUT_H__


#include <QtWidgets/QDialog>
#include <QtCore/QSettings>


class GeoAboutDlg : public QDialog
{
public:
	GeoAboutDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~GeoAboutDlg();

	GeoAboutDlg(const GeoAboutDlg& other);
	const GeoAboutDlg& operator=(const GeoAboutDlg&);


protected:
	virtual void accept() override;


private:
	QSettings *m_sett{nullptr};
};


#endif
