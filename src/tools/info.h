/**
 * info dialog
 * @author Tobias Weber <tweber@ill.fr>
 * @date oct-2021
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

#ifndef __TAKIN_PATHS_TOOLS_INFO_H__
#define __TAKIN_PATHS_TOOLS_INFO_H__


#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QPlainTextEdit>


class GeoInfoDlg : public QDialog
{
public:
	GeoInfoDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~GeoInfoDlg();

	GeoInfoDlg(const GeoInfoDlg& other);
	const GeoInfoDlg& operator=(const GeoInfoDlg&);

	void SetInfo(const QString& str);


protected:
	virtual void accept() override;


private:
	QSettings *m_sett{nullptr};
	QPlainTextEdit *m_edit{nullptr};
};


#endif
