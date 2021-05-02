/**
 * TAS path tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __PATHS_TOOL_H__
#define __PATHS_TOOL_H__

#include <QtCore/QSettings>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QLabel>

#include <string>
#include <memory>

#include "tlibs2/libs/maths.h"

#include "PathsRenderer.h"
#include "TASProperties.h"
#include "XtalProperties.h"
#include "PathProperties.h"
#include "CamProperties.h"
#include "About.h"


// ----------------------------------------------------------------------------
class PathsTool : public QMainWindow
{ /*Q_OBJECT*/
private:
	QSettings m_sett{"takin", "taspaths"};

	// renderer
	std::shared_ptr<PathsRenderer> m_renderer{std::make_shared<PathsRenderer>(this)};

	// gl info strings
	std::string m_gl_ver, m_gl_shader_ver, m_gl_vendor, m_gl_renderer;

	QStatusBar *m_statusbar{nullptr};
	QLabel *m_labelStatus{nullptr};
	QLabel *m_labelCollisionStatus{nullptr};

	QMenu *m_menuOpenRecent{nullptr};
	QMenuBar *m_menubar{nullptr};

	std::shared_ptr<AboutDlg> m_dspacingslgAbout;
	std::shared_ptr<TASPropertiesDockWidget> m_tasProperties;
	std::shared_ptr<XtalPropertiesDockWidget> m_xtalProperties;
	std::shared_ptr<PathPropertiesDockWidget> m_pathProperties;
	std::shared_ptr<CamPropertiesDockWidget> m_camProperties;

	std::string m_initialInstrFile = "instrument.taspaths";

	// recent file list and currently active file
	QStringList m_recentFiles;
	QString m_curFile;

	// instrument configuration
	InstrumentSpace m_instrspace;

	// mouse picker
	t_real m_mouseX, m_mouseY;
	std::string m_curObj;

	// crystal matrices
	t_mat m_B = tl2::B_matrix<t_mat>(5., 5., 5., tl2::pi<t_real>*0.5, tl2::pi<t_real>*0.5, tl2::pi<t_real>*0.5);
	t_mat m_UB = tl2::unit<t_mat>(3);

	// scattering plane
	t_vec m_plane_rlu[3] = {
		tl2::create<t_vec>({ 1, 0, 0 }),
		tl2::create<t_vec>({ 0, 1, 0 }),
		tl2::create<t_vec>({ 0, 0, 1 }),
	};

	// mono and ana d-spacings
	t_real m_dspacings[2] = { 3.355, 3.355 };

	// scattering senses
	t_real m_sensesCCW[3] = { 1., -1., 1. };


protected:
	void UpdateUB();


protected:
	// events
	virtual void showEvent(QShowEvent *) override;
	virtual void hideEvent(QHideEvent *) override;
	virtual void closeEvent(QCloseEvent *) override;

	// File -> New
	void NewFile();

	// File -> Open
	void OpenFile();

	// File -> Save
	void SaveFile();

	// File -> Save As
	void SaveFileAs();

	// load file
	bool OpenFile(const QString &file);

	// save file
	bool SaveFile(const QString &file);

	// adds a file to the recent files menu
	void AddRecentFile(const QString &file);

	// remember current file and set window title
	void SetCurrentFile(const QString &file);

	// sets the recent file menu
	void SetRecentFiles(const QStringList &files);

	// creates the "recent files" sub-menu
	void RebuildRecentFiles();


protected slots:
	// go to crystal coordinates
	void GotoCoordinates(t_real h, t_real k, t_real l, t_real ki, t_real kf);

	// called after the plotter has initialised
	void AfterGLInitialisation();

	// mouse coordinates on base plane
	void CursorCoordsChanged(t_real_gl x, t_real_gl y);

	// mouse is over an object
	void PickerIntersection(const t_vec3_gl* pos, std::string obj_name, const t_vec3_gl* posSphere);

	// clicked on an object
	void ObjectClicked(const std::string& obj, bool left, bool middle, bool right);

	// dragging an object
	void ObjectDragged(const std::string& obj, t_real_gl x, t_real_gl y);

	void UpdateStatusLabel();

	// set status flag indicating if the instrument is colliding
	void SetCollisionStatus(bool colliding);


public:
	/**
	 * create UI
	 */
	PathsTool(QWidget* pParent=nullptr);

	~PathsTool() = default;

	void SetInitialInstrumentFile(const std::string& file) { m_initialInstrFile = file;  }
};
// ----------------------------------------------------------------------------


#endif
