/**
 * TAS paths tool -- settings dialog and resource management
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

#include <string>
#include "tlibs2/libs/qt/gl.h"
#include "src/core/types.h"



// ----------------------------------------------------------------------------
// global settings variables
// ----------------------------------------------------------------------------
// application binary path
extern std::string g_apppath;

// maximum number of threads for calculations
extern unsigned int g_maxnum_threads;


// number precisions
extern int g_prec, g_prec_gui;

// epsilons
extern t_real g_eps, g_eps_angular, g_eps_gui;
extern t_real g_eps_voronoiedge;

// subdivision length of lines for interpolation
extern t_real g_line_subdiv_len;

// crystal angle offset
extern t_real g_a3_offs;

// angular deltas for calculation step width
extern t_real g_a2_delta;
extern t_real g_a4_delta;

// which backend to use for voronoi diagram calculation?
// 0: boost.polygon, 1: cgal
extern int g_voronoi_backend;

// which path finding strategy to use?
// 0: shortest path, 1: avoid walls
extern int g_pathstrategy;

// verify the generated path?
extern int g_verifypath;

// which polygon intersection method should be used?
// 0: sweep, 1: half-plane test
extern int g_poly_intersection_method;


// gui theme
extern QString g_theme;

// gui font
extern QString g_font;

// path tracker fps
extern unsigned int g_pathtracker_fps;

// renderer fps
extern unsigned int g_timer_fps;

extern int g_light_follows_cursor;
extern int g_enable_shadow_rendering;

extern int g_combined_screenshots;

// camera translation scaling factor
extern tl2::t_real_gl g_move_scale;

// camera rotation scaling factor
extern tl2::t_real_gl g_rotation_scale;
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


signals:
	// signal emitted when settings are applied
	void SettingsHaveChanged();
};

#endif
// ----------------------------------------------------------------------------
