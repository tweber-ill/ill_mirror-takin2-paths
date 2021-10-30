/**
 * settings dialog
 * @author Tobias Weber <tweber@ill.fr>
 * @date aug-2021
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

#include "settings.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QFontDialog>

#include "tlibs2/libs/file.h"
#include "tlibs2/libs/maths.h"



// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------
QString g_theme = "";
QString g_font = "";
int g_use_native_menubar = 0;
int g_use_native_dialogs = 1;
// ----------------------------------------------------------------------------



template<class T>
static void get_setting(QSettings* sett, const char* key, T* val)
{
	if(sett->contains(key))
		*val = sett->value(key, *val).template value<T>();
}


GeoSettingsDlg::GeoSettingsDlg(QWidget* parent, QSettings *sett)
	: QDialog{parent}, m_sett{sett}
{
	setWindowTitle("Settings");

	// restore dialog geometry
	if(m_sett && m_sett->contains("settings/geo"))
		restoreGeometry(m_sett->value("settings/geo").toByteArray());
	else
		resize(512, 425);


	// general settings
	QWidget *panelGeneral = new QWidget(this);
	QGridLayout *gridGeneral = new QGridLayout(panelGeneral);
	gridGeneral->setSpacing(4);
	gridGeneral->setContentsMargins(6, 6, 6, 6);


	// theme
	QLabel *labelTheme = new QLabel("GUI Style:", panelGeneral);
	labelTheme->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	m_comboTheme = new QComboBox(panelGeneral);
	m_comboTheme->addItems(QStyleFactory::keys());

	get_setting<QString>(sett, "settings/theme", &g_theme);
	if(g_theme != "")
	{
		int idxTheme = m_comboTheme->findText(g_theme);
		if(idxTheme >= 0 && idxTheme < m_comboTheme->count())
			m_comboTheme->setCurrentIndex(idxTheme);
	}

	// font
	QLabel *labelFont = new QLabel("Font:", panelGeneral);
	labelFont->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	m_editFont = new QLineEdit(panelGeneral);
	m_editFont->setReadOnly(true);
	QPushButton *btnFont = new QPushButton("Select...", panelGeneral);

	get_setting<QString>(sett, "settings/font", &g_font);
	if(g_font == "")
		g_font = QApplication::font().toString();
	m_editFont->setText(g_font);

	// native menubar
	m_checkMenubar = new QCheckBox("Use native menubar", panelGeneral);
	get_setting<int>(sett, "settings/native_menubar", &g_use_native_menubar);
	m_checkMenubar->setChecked(g_use_native_menubar!=0);

	// native dialogs
	m_checkDialogs = new QCheckBox("Use native dialogs", panelGeneral);
	get_setting<int>(sett, "settings/native_dialogs", &g_use_native_dialogs);
	m_checkDialogs->setChecked(g_use_native_dialogs!=0);

	// add widgets to layout
	int yGeneral = 0;
	gridGeneral->addWidget(labelTheme, yGeneral,0,1,1);
	gridGeneral->addWidget(m_comboTheme, yGeneral++,1,1,2);
	gridGeneral->addWidget(labelFont, yGeneral,0,1,1);
	gridGeneral->addWidget(m_editFont, yGeneral,1,1,1);
	gridGeneral->addWidget(btnFont, yGeneral++,2,1,1);
	gridGeneral->addWidget(m_checkMenubar, yGeneral++,0,1,3);
	gridGeneral->addWidget(m_checkDialogs, yGeneral++,0,1,3);

	QSpacerItem *spacer_end = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
	gridGeneral->addItem(spacer_end, yGeneral++,0,1,3);


	// main grid
	QGridLayout *grid = new QGridLayout(this);
	grid->setSpacing(4);
	grid->setContentsMargins(12, 12, 12, 12);
	int y = 0;

	grid->addWidget(panelGeneral, y++,0,1,1);

	QDialogButtonBox *buttons = new QDialogButtonBox(this);
	buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
	grid->addWidget(buttons, y++,0,1,1);


	// connections
	connect(btnFont, &QPushButton::clicked, [this]()
	{
		// current font
		QFont font = QApplication::font();

		// select a new font
		bool okClicked = false;
		font = QFontDialog::getFont(&okClicked, font, this);
		if(okClicked)
		{
			g_font = font.toString();
			if(g_font == "")
				g_font = QApplication::font().toString();
			m_editFont->setText(g_font);

			//QApplication::setFont(font);
		}

		// hack for the QFontDialog hiding the settings dialog
		this->show();
		this->raise();
		this->activateWindow();
	});

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(buttons, &QDialogButtonBox::clicked, [this, buttons](QAbstractButton* btn)
	{
		// apply button was pressed
		if(btn == static_cast<QAbstractButton*>(buttons->button(QDialogButtonBox::Apply)))
			ApplySettings();
	});
}


GeoSettingsDlg::~GeoSettingsDlg()
{
}


/**
 * read the settings and set the global variables
 */
void GeoSettingsDlg::ReadSettings(QSettings* sett)
{
	if(!sett)
		return;

	get_setting<QString>(sett, "settings/theme", &g_theme);
	get_setting<QString>(sett, "settings/font", &g_font);
	get_setting<int>(sett, "settings/native_menubar", &g_use_native_menubar);
	get_setting<int>(sett, "settings/native_dialogs", &g_use_native_dialogs);

	ApplyGuiSettings();
}


/**
 * 'Apply' was clicked, write the settings from the global variables
 */
void GeoSettingsDlg::ApplySettings()
{
	g_theme = m_comboTheme->currentText();
	g_font = m_editFont->text();
	g_use_native_menubar = m_checkMenubar->isChecked();
	g_use_native_dialogs = m_checkDialogs->isChecked();

	// write out the settings
	if(m_sett)
	{
		m_sett->setValue("settings/theme", g_theme);
		m_sett->setValue("settings/font", g_font);
		m_sett->setValue("settings/native_menubar", g_use_native_menubar);
		m_sett->setValue("settings/native_dialogs", g_use_native_dialogs);
	}

	ApplyGuiSettings();
}


void GeoSettingsDlg::ApplyGuiSettings()
{
	// set gui theme
	if(g_theme != "")
	{
		if(QStyle* theme = QStyleFactory::create(g_theme); theme)
			QApplication::setStyle(theme);
	}

	// set gui font
	if(g_font != "")
	{
		QFont font;
		if(font.fromString(g_font))
			QApplication::setFont(font);
	}

	// set native menubar and dialogs
	QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, !g_use_native_menubar);
	QApplication::setAttribute(Qt::AA_DontUseNativeDialogs, !g_use_native_dialogs);
}


/**
 * 'OK' was clicked
 */
void GeoSettingsDlg::accept()
{
	ApplySettings();

	if(m_sett)
		m_sett->setValue("settings/geo", saveGeometry());
	QDialog::accept();
}
// ----------------------------------------------------------------------------
