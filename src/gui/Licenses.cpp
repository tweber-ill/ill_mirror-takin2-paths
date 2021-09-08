/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date sep-2021
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

#include "Licenses.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>

#include "Settings.h"
#include "tlibs2/libs/file.h"


/**
 * constructor
 */
LicensesDlg::LicensesDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Software Licenses");
	setSizeGripEnabled(true);

	// restore dialog geometry
	if(m_sett && m_sett->contains("licenses/geo"))
		restoreGeometry(m_sett->value("licenses/geo").toByteArray());


	// tab widget
	auto tabwidget = new QTabWidget(this);

	// license tab
	{
		auto tab = new QWidget(tabwidget);
		auto grid = new QGridLayout(tab);
		grid->setSpacing(4);
		grid->setContentsMargins(4, 4, 4, 4);

		auto text = new QTextEdit(tab);
		text->setReadOnly(true);
		grid->addWidget(text, 0, 0, 1, 1);

		std::string license_file = find_resource("LICENSE");
		auto [license_ok, license_text] = tl2::load_file<std::string>(license_file);

		if(license_file == "" || !license_ok)
			text->setPlainText("Error: \"LICENSE\"file could not be read!");
		else
			text->setPlainText(license_text.c_str());

		tabwidget->addTab(tab, "License");
	}

	// 3rd party licenses tab
	{
		auto tab = new QWidget(tabwidget);
		auto grid = new QGridLayout(tab);
		grid->setSpacing(4);
		grid->setContentsMargins(4, 4, 4, 4);

		auto text = new QTextEdit(tab);
		text->setReadOnly(true);
		grid->addWidget(text, 0, 0, 1, 1);

		tabwidget->addTab(tab, "3rd Party Licenses");
	}


	// main layout grid
	auto grid_main = new QGridLayout(this);
	grid_main->setSpacing(4);
	grid_main->setContentsMargins(12, 12, 12, 12);
	int y_main = 0;

	grid_main->addWidget(tabwidget, y_main++, 0, 1, 1);

	//QSpacerItem *spacer_end = new QSpacerItem(1, 4, QSizePolicy::Minimum, QSizePolicy::Fixed);
	//grid_main->addItem(spacer_end, y_main++,0,1,1);
	
	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok);
	grid_main->addWidget(buttons, y_main++, 0, 1, 1);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
}


/**
 * copy constructor
 */
LicensesDlg::LicensesDlg(const LicensesDlg& other)
	: QDialog(other.parentWidget())
{
	operator=(other);
}


/**
 * assignment operator
 */
LicensesDlg& LicensesDlg::operator=(const LicensesDlg&)
{
	m_sett = nullptr;
	return *this;
}


/**
 * destructor
 */
LicensesDlg::~LicensesDlg()
{
}


/**
 * 'OK' or 'Apply' button has been pressed
 */
void LicensesDlg::accept()
{
	if(m_sett)
		m_sett->setValue("licenses/geo", saveGeometry());
	QDialog::accept();
}
