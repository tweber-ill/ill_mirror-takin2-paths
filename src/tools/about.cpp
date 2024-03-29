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

#include "about.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>

#include <boost/config.hpp>
#include <boost/version.hpp>


/**
 * constructor
 */
GeoAboutDlg::GeoAboutDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("About");
	setSizeGripEnabled(true);

	// restore dialog geometry
	if(m_sett && m_sett->contains("tools_about/geo"))
		restoreGeometry(m_sett->value("tools_about/geo").toByteArray());

	auto grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(12, 12, 12, 12);

	int y = 0;

	QLabel *labTitle = new QLabel("Geometry Library and Tools", this);
	QFont fontTitle = labTitle->font();
	fontTitle.setPointSize(fontTitle.pointSize()*1.5);
	fontTitle.setWeight(QFont::Bold);
	labTitle->setFont(fontTitle);
	grid->addWidget(labTitle, y++,0,1,2);

	QSpacerItem *spacer0 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
	grid->addItem(spacer0, y++,0,1,2);

	QLabel *labDOI1 = new QLabel("DOI: ", this);
	QFont fontLabel1 = labDOI1->font();
	fontLabel1.setWeight(QFont::Bold);
	labDOI1->setFont(fontLabel1);
	grid->addWidget(labDOI1, y,0,1,1);
	QLabel *labDOI2 = new QLabel("<a href=\"https://doi.org/10.5281/zenodo.4297475\">"
		"10.5281/zenodo.4297475</a>.", this);
	labDOI2->setOpenExternalLinks(1);
	grid->addWidget(labDOI2, y++,1,1,1);

	QSpacerItem *spacer1 = new QSpacerItem(1, 8, QSizePolicy::Minimum, QSizePolicy::Fixed);
	grid->addItem(spacer1, y++,0,1,2);

	QLabel *labAuthor1 = new QLabel("Author: ", this);
	fontLabel1.setWeight(QFont::Bold);
	labAuthor1->setFont(fontLabel1);
	grid->addWidget(labAuthor1, y,0,1,1);
	QLabel *labAuthor2 = new QLabel("Tobias Weber <tobias.weber@tum.de>.", this);
	grid->addWidget(labAuthor2, y++,1,1,1);

	QLabel *labDate1 = new QLabel("Date: ", this);
	labDate1->setFont(fontLabel1);
	grid->addWidget(labDate1, y,0,1,1);
	QLabel *labDate2 = new QLabel("November 2020 - September 2021.", this);
	grid->addWidget(labDate2, y++,1,1,1);

	QLabel *labLic1 = new QLabel("License: ", this);
	fontLabel1.setWeight(QFont::Bold);
	labLic1->setFont(fontLabel1);
	grid->addWidget(labLic1, y,0,1,1);
	QLabel *labLic2 = new QLabel("GNU GPL Version 3.", this);
	grid->addWidget(labLic2, y++,1,1,1);

	QSpacerItem *spacer2 = new QSpacerItem(1, 8, QSizePolicy::Minimum, QSizePolicy::Fixed);
	grid->addItem(spacer2, y++,0,1,2);

	QLabel *labBuildDate1 = new QLabel("Build Timestamp: ", this);
	labBuildDate1->setFont(fontLabel1);
	grid->addWidget(labBuildDate1, y,0,1,1);
	QString buildDate = QString{__DATE__} + QString{", "} + QString{__TIME__} + QString{"."};
	QLabel *labBuildDate2 = new QLabel(buildDate, this);
	grid->addWidget(labBuildDate2, y++,1,1,1);

	QLabel *labCompiler1 = new QLabel("Compiler: ", this);
	labCompiler1->setFont(fontLabel1);
	grid->addWidget(labCompiler1, y,0,1,1);
	QString compiler = QString{BOOST_COMPILER} + QString{"."};
	QLabel *labCompiler2 = new QLabel(compiler, this);
	grid->addWidget(labCompiler2, y++,1,1,1);

	QLabel *labCPPLib1 = new QLabel("C++ Library: ", this);
	labCPPLib1->setFont(fontLabel1);
	grid->addWidget(labCPPLib1, y,0,1,1);
	QString cpplib2 = QString{BOOST_STDLIB} + QString{"."};
	QLabel *labCPPLib = new QLabel(cpplib2, this);
	grid->addWidget(labCPPLib, y++,1,1,1);

	QLabel *labBoostLib1 = new QLabel("Boost Library: ", this);
	labBoostLib1->setFont(fontLabel1);
	grid->addWidget(labBoostLib1, y,0,1,1);
	QString boostlib2 = QString{"Version %1.%2.%3."}
		.arg(BOOST_VERSION / 100000)
		.arg((BOOST_VERSION % 100000) / 100)
		.arg(BOOST_VERSION % 100);
	QLabel *labBoostLib = new QLabel(boostlib2, this);
	grid->addWidget(labBoostLib, y++,1,1,1);

	QSpacerItem *spacer3 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
	grid->addItem(spacer3, y++,0,1,2);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok);
	grid->addWidget(buttons, y++,0,1,2);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
}


/**
 * destructor
 */
GeoAboutDlg::~GeoAboutDlg()
{
}


/**
 * copy constructor
 */
GeoAboutDlg::GeoAboutDlg(const GeoAboutDlg& other) : QDialog(other.parentWidget())
{
	operator=(other);
}


/**
 * assignment operator
 */
const GeoAboutDlg& GeoAboutDlg::operator=(const GeoAboutDlg&)
{
	m_sett = nullptr;
	return *this;
}


/**
 * 'OK' or 'Apply' button has been pressed
 */
void GeoAboutDlg::accept()
{
	if(m_sett)
		m_sett->setValue("tools_about/geo", saveGeometry());
	QDialog::accept();
}
