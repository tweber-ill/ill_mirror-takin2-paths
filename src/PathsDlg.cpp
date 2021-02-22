/**
 * TAS path tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include <QtCore/QSettings>
#include <QtCore/QDir>

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>

#include <sstream>
#include <string>

#include "tlibs2/libs/math20.h"
#include "tlibs2/libs/str.h"
#include "tlibs2/libs/helper.h"

#include "src/PathsRenderer.h"


using t_real = double;
using t_vec = tl2::vec<t_real, std::vector>;
using t_mat = tl2::mat<t_real, std::vector>;


// ----------------------------------------------------------------------------
class PathsDlg : public QMainWindow
{ /*Q_OBJECT*/
private:
	QSettings m_sett{"takin", "paths"};

	std::shared_ptr<PathsRenderer> m_plot{std::make_shared<PathsRenderer>(this)};
	// gl info strings
	std::string m_gl_ver, m_gl_shader_ver, m_gl_vendor, m_gl_renderer;

	bool m_mouseDown[3] = {false, false, false};

	QStatusBar *m_statusbar = nullptr;
	QLabel *m_labelStatus = nullptr;

	QMenuBar *m_menubar = nullptr;


protected:
	virtual void closeEvent(QCloseEvent *) override
	{
		// save window size, position, and state
		m_sett.setValue("geo", saveGeometry());
		m_sett.setValue("state", saveState());
	}


protected slots:
	/**
	 * called after the plotter has initialised
	 */
	void AfterGLInitialisation()
	{
		// GL device info
		std::tie(m_gl_ver, m_gl_shader_ver, m_gl_vendor, m_gl_renderer) = m_plot->GetGlDescr();
	}


	/**
	 * mouse button pressed
	 */
	void MouseDown(bool left, bool mid, bool right)
	{
		if(left) m_mouseDown[0] = true;
		if(mid) m_mouseDown[1] = true;
		if(right) m_mouseDown[2] = true;
	}


	/**
	 * mouse button released
	 */
	void MouseUp(bool left, bool mid, bool right)
	{
		if(left) m_mouseDown[0] = false;
		if(mid) m_mouseDown[1] = false;
		if(right) m_mouseDown[2] = false;
	}


public:
	/**
	 * create UI
	 */
	PathsDlg(QWidget* pParent=nullptr) : QMainWindow{pParent}
	{
		setWindowTitle("TAS Paths");


		// --------------------------------------------------------------------
		// plot widget
		// --------------------------------------------------------------------
		auto plotpanel = new QWidget(this);

		connect(m_plot.get(), &PathsRenderer::MouseDown, this, &PathsDlg::MouseDown);
		connect(m_plot.get(), &PathsRenderer::MouseUp, this, &PathsDlg::MouseUp);
		connect(m_plot.get(), &PathsRenderer::AfterGLInitialisation, this, &PathsDlg::AfterGLInitialisation);

		auto pGrid = new QGridLayout(plotpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		pGrid->addWidget(m_plot.get(), 0,0,1,4);

		setCentralWidget(plotpanel);
		// --------------------------------------------------------------------


		// --------------------------------------------------------------------
		// menu bar
		// --------------------------------------------------------------------
		m_menubar = new QMenuBar(this);


		// file menu
		QMenu *menuFile = new QMenu("File", m_menubar);
		QAction *actionQuit = new QAction(QIcon::fromTheme("application-exit"), "Quit", menuFile);
		actionQuit->setMenuRole(QAction::QuitRole);
		connect(actionQuit, &QAction::triggered, this, &PathsDlg::close);
		menuFile->addAction(actionQuit);


		// help menu
		QMenu *menuHelp = new QMenu("Help", m_menubar);

		QAction *actionAboutQt = new QAction(QIcon::fromTheme("help-about"), "About Qt Libraries...", menuHelp);
		actionAboutQt->setMenuRole(QAction::AboutQtRole);
		connect(actionAboutQt, &QAction::triggered, this, []() { qApp->aboutQt(); });
		menuHelp->addAction(actionAboutQt);

		QAction *actionAboutGl = new QAction(QIcon::fromTheme("help-about"), "About Renderer...", menuHelp);
		connect(actionAboutGl, &QAction::triggered, this, [this]()
		{
			std::ostringstream ostrInfo;
			ostrInfo << "Rendering using the following device:\n\n";
			ostrInfo << "GL Vendor: " << m_gl_vendor << "\n";
			ostrInfo << "GL Renderer: " << m_gl_renderer << "\n";
			ostrInfo << "GL Version: " << m_gl_ver << "\n";
			ostrInfo << "GL Shader Version: " << m_gl_shader_ver << "\n";
			QMessageBox::information(this, "About Renderer", ostrInfo.str().c_str());
		});
		menuHelp->addAction(actionAboutGl);

		QAction *actionAbout = new QAction(QIcon::fromTheme("help-about"), "About Program...", menuHelp);
		actionAbout->setMenuRole(QAction::AboutRole);
		connect(actionAbout, &QAction::triggered, this, []()
		{

		});
		menuHelp->addAction(actionAbout);


		m_menubar->addMenu(menuFile);
		m_menubar->addMenu(menuHelp);
		setMenuBar(m_menubar);
		// --------------------------------------------------------------------


		// --------------------------------------------------------------------
		// status bar
		// --------------------------------------------------------------------
		m_labelStatus = new QLabel(this);
		m_labelStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		m_labelStatus->setFrameStyle(int(QFrame::Sunken) | int(QFrame::Panel));
		m_labelStatus->setLineWidth(1);

		m_statusbar = new QStatusBar(this);
		m_statusbar->addPermanentWidget(m_labelStatus);
		setStatusBar(m_statusbar);
		// --------------------------------------------------------------------


		// --------------------------------------------------------------------
		// restory window size, position, and state
		// --------------------------------------------------------------------
		if(m_sett.contains("geo"))
			restoreGeometry(m_sett.value("geo").toByteArray());
		else
			resize(800, 600);

		if(m_sett.contains("state"))
			restoreState(m_sett.value("state").toByteArray());
		// --------------------------------------------------------------------
	}
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------

/**
 * main
 */
int main(int argc, char** argv)
{
	qRegisterMetaType<std::size_t>("std::size_t");

	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();

	QApplication::addLibraryPath(QString(".") + QDir::separator() + "qtplugins");
	auto app = std::make_unique<QApplication>(argc, argv);
	auto dlg = std::make_unique<PathsDlg>(nullptr);
	dlg->show();

	return app->exec();
}

// ----------------------------------------------------------------------------
