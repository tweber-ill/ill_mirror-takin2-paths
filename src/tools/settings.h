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

#ifndef __TASPATHS_TOOLS_SETTINGS__
#define __TASPATHS_TOOLS_SETTINGS__

#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>

#include <string>
#include "tlibs2/libs/qt/gl.h"
#include "src/core/types.h"



// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------
// gui theme
extern QString g_theme;

// gui font
extern QString g_font;

// use native menubar?
extern int g_use_native_menubar;

// use native dialogs?
extern int g_use_native_dialogs;
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// settings dialog
// ----------------------------------------------------------------------------
class GeoSettingsDlg : public QDialog
{
public:
	GeoSettingsDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~GeoSettingsDlg();

	GeoSettingsDlg(const GeoSettingsDlg&) = delete;
	const GeoSettingsDlg& operator=(const GeoSettingsDlg&) = delete;

	static void ReadSettings(QSettings* sett);


protected:
	virtual void accept() override;

	void ApplySettings();
	static void ApplyGuiSettings();


private:
	QSettings *m_sett{nullptr};

	QComboBox *m_comboTheme{nullptr};
	QLineEdit *m_editFont{nullptr};
	QCheckBox *m_checkMenubar{nullptr};
	QCheckBox *m_checkDialogs{nullptr};
};

#endif
// ----------------------------------------------------------------------------
