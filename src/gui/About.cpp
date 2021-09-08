/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date mar-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include "About.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>

#include <boost/config.hpp>
#include <boost/version.hpp>


/**
 * constructor
 */
AboutDlg::AboutDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("About");
	setSizeGripEnabled(true);

	// restore dialog geometry
	if(m_sett && m_sett->contains("about/geo"))
		restoreGeometry(m_sett->value("about/geo").toByteArray());

	auto grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(12, 12, 12, 12);

	int y = 0;

	QLabel *labTitle = new QLabel("Takin / TAS-Paths", this);
	QFont fontTitle = labTitle->font();
	fontTitle.setPointSize(fontTitle.pointSize()*1.5);
	fontTitle.setWeight(QFont::Bold);
	labTitle->setFont(fontTitle);
	grid->addWidget(labTitle, y++,0,1,2);

	QSpacerItem *spacer0 = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
	grid->addItem(spacer0, y++,0,1,2);

	QLabel *labSubtitle1 = new QLabel("DOI: ", this);
	QFont fontLabel1 = labSubtitle1->font();
	fontLabel1.setWeight(QFont::Bold);
	labSubtitle1->setFont(fontLabel1);
	grid->addWidget(labSubtitle1, y,0,1,1);
	QLabel *labSubtitle2 = new QLabel("<a href=\"https://doi.org/10.5281/zenodo.4625649\">"
		"10.5281/zenodo.4625649</a>.", this);
	labSubtitle2->setOpenExternalLinks(1);
	grid->addWidget(labSubtitle2, y++,1,1,1);

	QSpacerItem *spacer1 = new QSpacerItem(1, 8, QSizePolicy::Minimum, QSizePolicy::Fixed);
	grid->addItem(spacer1, y++,0,1,2);

	QLabel *labAuthor1 = new QLabel("Author: ", this);
	fontLabel1.setWeight(QFont::Bold);
	labAuthor1->setFont(fontLabel1);
	grid->addWidget(labAuthor1, y,0,1,1);
	QLabel *labAuthor2 = new QLabel("Tobias Weber <tweber@ill.fr>.", this);
	grid->addWidget(labAuthor2, y++,1,1,1);

	QLabel *labDate1 = new QLabel("Date: ", this);
	labDate1->setFont(fontLabel1);
	grid->addWidget(labDate1, y,0,1,1);
	QLabel *labDate2 = new QLabel("February 2021 - September 2021.", this);
	grid->addWidget(labDate2, y++,1,1,1);

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
 * copy constructor
 */
AboutDlg::AboutDlg(const AboutDlg& other) : QDialog(other.parentWidget())
{
	operator=(other);
}


/**
 * assignment operator
 */
const AboutDlg& AboutDlg::operator=(const AboutDlg&)
{
	m_sett = nullptr;
	return *this;
}


/**
 * destructor
 */
AboutDlg::~AboutDlg()
{
}


/**
 * 'OK' or 'Apply' button has been pressed
 */
void AboutDlg::accept()
{
	if(m_sett)
		m_sett->setValue("about/geo", saveGeometry());
	QDialog::accept();
}
