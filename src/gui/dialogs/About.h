/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
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

#ifndef __TAKIN_PATHS_ABOUT_H__
#define __TAKIN_PATHS_ABOUT_H__


#include <QtWidgets/QDialog>
#include <QtCore/QSettings>


class AboutDlg : public QDialog
{
public:
	AboutDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~AboutDlg();

	AboutDlg(const AboutDlg& other);
	const AboutDlg& operator=(const AboutDlg&);


protected:
	virtual void accept() override;


private:
	QSettings *m_sett{nullptr};
};


#endif
