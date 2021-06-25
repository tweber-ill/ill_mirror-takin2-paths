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

// maximum number of threads for calculations
extern unsigned int g_maxnum_threads;
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
{
public:
	SettingsDlg(QWidget* parent = nullptr, QSettings *sett = nullptr);
	virtual ~SettingsDlg();

protected:
	virtual void accept() override;
	void ApplySettings();

private:
	QSettings *m_sett{nullptr};
	QTableWidget *m_tab{nullptr};
};

#endif
// ----------------------------------------------------------------------------
