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
#include "tlibs2/libs/qt/gl.h"
#include "src/core/types.h"


/**
 * settings dialog
 */
class SettingsDlg : public QDialog
{ Q_OBJECT
public:
	/**
	 * constructor
	 */
	SettingsDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);

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


protected:
	virtual void accept() override;
	void ApplySettings();
	static void ApplyGuiSettings();


private:
	QSettings *m_sett{nullptr};
	QTableWidget *m_table{nullptr};

	QComboBox *m_comboTheme{nullptr};
	QLineEdit *m_editFont{nullptr};
	QCheckBox *m_checkMenubar{nullptr};
	QCheckBox *m_checkDialogs{nullptr};


signals:
	// signal emitted when settings are applied
	void SettingsHaveChanged();
};

#endif
