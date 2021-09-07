/**
 * settings dialog
 * @author Tobias Weber <tweber@ill.fr>
 * @date aug-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TASPATHS_TOOLS_SETTINGS__
#define __TASPATHS_TOOLS_SETTINGS__

#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>

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
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// settings dialog
// ----------------------------------------------------------------------------
class SettingsDlg : public QDialog
{
public:
	SettingsDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~SettingsDlg();

	SettingsDlg(const SettingsDlg&) = delete;
	const SettingsDlg& operator=(const SettingsDlg&) = delete;

	static void ReadSettings(QSettings* sett);


protected:
	virtual void accept() override;

	void ApplySettings();
	static void ApplyGuiSettings();


private:
	QSettings *m_sett{nullptr};

	QComboBox *m_comboTheme{nullptr};
	QLineEdit *m_editFont{nullptr};
};

#endif
// ----------------------------------------------------------------------------
