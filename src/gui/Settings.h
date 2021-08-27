/**
 * TAS paths tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date apr-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __TASPATHS_SETTINGS__
#define __TASPATHS_SETTINGS__

#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>

#include <string>
#include "tlibs2/libs/qt/gl.h"
#include "src/core/types.h"



// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------
// application binary path
extern std::string g_apppath;

// renderer fps
extern unsigned int g_timer_fps;

// camera translation scaling factor
extern tl2::t_real_gl g_move_scale;

// camera rotation scaling factor
extern tl2::t_real_gl g_rotation_scale;

// number precisions
extern int g_prec, g_prec_gui;

// epsilons
extern t_real g_eps, g_eps_angular, g_eps_gui;

// crystal angle offset
extern t_real g_a3_offs;

// angular deltas for calculation step width
extern t_real g_a2_delta;
extern t_real g_a4_delta;

// maximum number of threads for calculations
extern unsigned int g_maxnum_threads;

// gui theme
extern QString g_theme;

// gui font
extern QString g_font;

// which polygon intersection method should be used?
// 0: sweep, 1: half-plane test
extern int g_poly_intersection_method;
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// functions
// ----------------------------------------------------------------------------
/**
 * get the path to a resource file
 */
extern std::string find_resource(const std::string& resfile);
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// settings dialog
// ----------------------------------------------------------------------------
class SettingsDlg : public QDialog
{ Q_OBJECT
public:
	SettingsDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~SettingsDlg();

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


signals:
	// signal emitted when settings are applied
	void SettingsHaveChanged();
};

#endif
// ----------------------------------------------------------------------------
