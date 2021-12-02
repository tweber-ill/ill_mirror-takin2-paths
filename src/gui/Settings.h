/**
 * TAS paths tool -- settings dialog
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
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

#ifndef __TASPATHS_SETTINGS__
#define __TASPATHS_SETTINGS__

#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>

#include <string>
#include <variant>
#include <unordered_map>

#include "tlibs2/libs/qt/gl.h"
#include "src/core/types.h"
#include "settings_common.h"


/**
 * settings dialog
 */
class SettingsDlg : public QDialog
{ Q_OBJECT
public:
	/**
	 * constructor
	 */
	SettingsDlg(QWidget* parent = nullptr,
		QSettings *sett = nullptr,
		bool hide_optional_settings = false);

	/**
	 * destructor
	 */
	virtual ~SettingsDlg();

	/**
	 * copy constructor
	 */
	SettingsDlg(const SettingsDlg&) = delete;
	const SettingsDlg& operator=(const SettingsDlg&) = delete;

	static void ReadSettings(QSettings* sett);
	static void SaveDefaultSettings();


protected:
	virtual void accept() override;

	void PopulateSettingsTable();
	void RestoreDefaultSettings();

	void ApplySettings();
	static void ApplyGuiSettings();


private:
	QSettings *m_sett{nullptr};
	bool m_hide_optional_settings{false};
	QTableWidget *m_table{nullptr};

	QComboBox *m_comboTheme{nullptr};
	QLineEdit *m_editFont{nullptr};
	QCheckBox *m_checkMenubar{nullptr};
	QCheckBox *m_checkDialogs{nullptr};
	QCheckBox *m_checkAnimations{nullptr};
	QCheckBox *m_checkTabbedDocks{nullptr};
	QCheckBox *m_checkNestedDocks{nullptr};

	// default setting values
	static std::unordered_map<std::string, SettingsVariable::t_variant> s_defaults;


signals:
	// signal emitted when settings are applied
	void SettingsHaveChanged();
};

#endif
