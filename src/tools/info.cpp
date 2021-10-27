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

#include "info.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>


/**
 * constructor
 */
GeoInfoDlg::GeoInfoDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Infos");
	setSizeGripEnabled(true);

	// restore dialog geometry
	if(m_sett && m_sett->contains("info/geo"))
		restoreGeometry(m_sett->value("info/geo").toByteArray());

	auto grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(12, 12, 12, 12);

	int y = 0;
	m_edit = new QPlainTextEdit(this);
	m_edit->setReadOnly(true);
	m_edit->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
	grid->addWidget(m_edit, y++,0,1,1);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok);
	buttons->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
	grid->addWidget(buttons, y++,0,1,1);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
}


/**
 * set the info text to be shown
 */
void GeoInfoDlg::SetInfo(const QString& str)
{
	if(!m_edit)
		return;

	m_edit->setPlainText(str);
}


/**
 * destructor
 */
GeoInfoDlg::~GeoInfoDlg()
{
}


/**
 * copy constructor
 */
GeoInfoDlg::GeoInfoDlg(const GeoInfoDlg& other) : QDialog(other.parentWidget())
{
	operator=(other);
}


/**
 * assignment operator
 */
const GeoInfoDlg& GeoInfoDlg::operator=(const GeoInfoDlg&)
{
	m_sett = nullptr;
	return *this;
}


/**
 * 'OK' or 'Apply' button has been pressed
 */
void GeoInfoDlg::accept()
{
	if(m_sett)
		m_sett->setValue("info/geo", saveGeometry());
	QDialog::accept();
}
