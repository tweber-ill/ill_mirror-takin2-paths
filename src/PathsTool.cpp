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
#include <QtWidgets/QFileDialog>

#include <string>

#include "tlibs2/libs/math20.h"
#include "tlibs2/libs/str.h"
#include "tlibs2/libs/file.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/helper.h"

#include "PathsRenderer.h"
#include "Instrument.h"
#include "globals.h"


#define MAX_RECENT_FILES 16
#define PROG_TITLE "TAS Paths"
#define PROG_IDENT "takin_paths"
#define FILE_BASENAME "paths/"



// ----------------------------------------------------------------------------
class PathsTool : public QMainWindow
{ /*Q_OBJECT*/
private:
	QSettings m_sett{"takin", "paths"};

	// renderer
	std::shared_ptr<PathsRenderer> m_renderer{std::make_shared<PathsRenderer>(this)};

	// gl info strings
	std::string m_gl_ver, m_gl_shader_ver, m_gl_vendor, m_gl_renderer;

	bool m_mouseDown[3] = {false, false, false};

	QStatusBar *m_statusbar = nullptr;
	QLabel *m_labelStatus = nullptr;

	QMenu *m_menuOpenRecent = nullptr;
	QMenuBar *m_menubar = nullptr;

	// recent file list and currently active file
	QStringList m_recentFiles;
	QString m_curFile;

	// instrument configuration
	Instrument m_instr;


protected:
	virtual void closeEvent(QCloseEvent *) override
	{
		// save window size, position, and state
		m_sett.setValue("geo", saveGeometry());
		m_sett.setValue("state", saveState());

		// remove superfluous entries and save the recent files list
		while(m_recentFiles.size() > MAX_RECENT_FILES)
			m_recentFiles.pop_front();
		m_sett.setValue("recent_files", m_recentFiles);
	}


	/**
	 * File -> New
	 */
	void NewFile()
	{
		SetCurrentFile("");

		m_instr.Clear();
		m_renderer->LoadInstrument(m_instr);
	}


	/**
	 * File -> Open
	 */
	void OpenFile()
	{
		QString dirLast = m_sett.value("cur_dir", "").toString();

		QString filename = QFileDialog::getOpenFileName(this, "Open File", dirLast, "Paths Files (*.paths)");
		if(filename=="" || !QFile::exists(filename))
			return;

		if(OpenFile(filename))
			m_sett.setValue("cur_dir", QFileInfo(filename).path());
	}


	/**
	 * File -> Save
	 */
	void SaveFile()
	{
		if(m_curFile == "")
			SaveFileAs();
		else
			SaveFile(m_curFile);
	}


	/**
	 * File -> Save As
	 */
	void SaveFileAs()
	{
		QString dirLast = m_sett.value("cur_dir", "").toString();

		QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "Paths Files (*.paths)");
		if(filename=="")
			return;

		if(SaveFile(filename))
			m_sett.setValue("cur_dir", QFileInfo(filename).path());
	}


	/**
	 * load file
	 */
	bool OpenFile(const QString &file)
	{
		if(file=="" || !QFile::exists(file))
			return false;


		// load xml
		tl2::Prop<std::string> prop;
		prop.SetSeparator('/');
		if(!prop.Load(file.toStdString(), tl2::PropType::XML))
		{
			QMessageBox::critical(this, "Error", "Could not load file.");
			return false;
		}

		// check format and version
		auto optIdent = prop.QueryOpt<std::string>(FILE_BASENAME "ident");
		auto optTime = prop.QueryOpt<t_real>(FILE_BASENAME "timestamp");
		if(!optIdent || *optIdent != PROG_IDENT)
		{
			QMessageBox::critical(this, "Error", "Not a recognised file format. Ignoring.");
			return false;
		}

		std::cout << "Loading file \"" << file.toStdString()
			<< "\" dated " << tl2::epoch_to_str(*optTime) << "." << std::endl;


		if(!m_instr.Load(prop, FILE_BASENAME "instrument/"))
		{
			QMessageBox::critical(this, "Error", "Instrument configuration could not be loaded.");
			return false;
		}

		SetCurrentFile(file);
		AddRecentFile(file);

		m_renderer->LoadInstrument(m_instr);
		return true;
	}


	/**
	 * save file
	 */
	bool SaveFile(const QString &file)
	{
		if(file=="")
			return false;

		std::unordered_map<std::string, std::string> data;

		// set format and version
		data[FILE_BASENAME "ident"] = PROG_IDENT;
		data[FILE_BASENAME "timestamp"] = tl2::var_to_str(tl2::epoch<t_real>());


		tl2::Prop<std::string> prop;
		prop.SetSeparator('/');
		prop.Add(data);

		if(!prop.Save(file.toStdString(), tl2::PropType::XML))
		{
			QMessageBox::critical(this, "Error", "Could not save file.");
			return false;
		}

		SetCurrentFile(file);
		AddRecentFile(file);
		return true;
	}


	/**
	 * adds a file to the recent files menu
	 */
	void AddRecentFile(const QString &file)
	{
		for(const auto &recentfile : m_recentFiles)
		{
			// file already in list?
			if(recentfile == file)
				return;
		}

		m_recentFiles.push_back(file);
		RebuildRecentFiles();
	}


	/**
	 * remember current file and set window title
	 */
	void SetCurrentFile(const QString &file)
	{
		static const QString title(PROG_TITLE);
		m_curFile = file;

		if(m_curFile == "")
			this->setWindowTitle(title);
		else
			this->setWindowTitle(title + " -- " + m_curFile);
	}


	/**
	 * sets the recent file menu
	 */
	void SetRecentFiles(const QStringList &files)
	{
		m_recentFiles = files;
		RebuildRecentFiles();
	}


	/**
	 * creates the "recent files" sub-menu
	 */
	void RebuildRecentFiles()
	{
		m_menuOpenRecent->clear();

		std::size_t num_recent_files = 0;
		for(auto iter = m_recentFiles.rbegin(); iter != m_recentFiles.rend(); ++iter)
		{
			QString filename = *iter;
			auto *acFile = new QAction(QIcon::fromTheme("document"), filename, m_menubar);

			connect(acFile, &QAction::triggered, [this, filename]() { this->OpenFile(filename); });
			m_menuOpenRecent->addAction(acFile);

			if(++num_recent_files >= MAX_RECENT_FILES)
				break;
		}
	}


protected slots:
	/**
	 * called after the plotter has initialised
	 */
	void AfterGLInitialisation()
	{
		// GL device info
		std::tie(m_gl_ver, m_gl_shader_ver, m_gl_vendor, m_gl_renderer) 
			= m_renderer->GetGlDescr();

		m_renderer->LoadInstrument(m_instr);
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


	/**
	 * mouse coordinates on base plane
	 */
	void MouseCoordsChanged(t_real_gl x, t_real_gl y)
	{
		std::ostringstream ostr;
		ostr.precision(4);
		ostr << "x = " << x << " m, y = " << y << " m";
		m_labelStatus->setText(ostr.str().c_str());
	}


public:
	/**
	 * create UI
	 */
	PathsTool(QWidget* pParent=nullptr) : QMainWindow{pParent}
	{
		setWindowTitle(PROG_TITLE);


		// --------------------------------------------------------------------
		// plot widget
		// --------------------------------------------------------------------
		auto plotpanel = new QWidget(this);

		connect(m_renderer.get(), &PathsRenderer::MouseDown, this, &PathsTool::MouseDown);
		connect(m_renderer.get(), &PathsRenderer::MouseUp, this, &PathsTool::MouseUp);
		connect(m_renderer.get(), &PathsRenderer::BasePlaneCoordsChanged, this, &PathsTool::MouseCoordsChanged);
		connect(m_renderer.get(), &PathsRenderer::AfterGLInitialisation, this, &PathsTool::AfterGLInitialisation);

		auto pGrid = new QGridLayout(plotpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		pGrid->addWidget(m_renderer.get(), 0,0,1,4);

		setCentralWidget(plotpanel);
		// --------------------------------------------------------------------


		// --------------------------------------------------------------------
		// menu bar
		// --------------------------------------------------------------------
		m_menubar = new QMenuBar(this);


		// file menu
		QMenu *menuFile = new QMenu("File", m_menubar);

		QAction *acNew = new QAction(QIcon::fromTheme("document-new"), "New", menuFile);
		QAction *acOpen = new QAction(QIcon::fromTheme("document-open"), "Open...", menuFile);
		QAction *acSave = new QAction(QIcon::fromTheme("document-save"), "Save", menuFile);
		QAction *acSaveAs = new QAction(QIcon::fromTheme("document-save-as"), "Save As...", menuFile);
		QAction *actionQuit = new QAction(QIcon::fromTheme("application-exit"), "Quit", menuFile);

		m_menuOpenRecent = new QMenu("Open Recent", menuFile);
		m_menuOpenRecent->setIcon(QIcon::fromTheme("document-open-recent"));

		actionQuit->setMenuRole(QAction::QuitRole);


		connect(acNew, &QAction::triggered, this, [this]() { this->NewFile(); });
		connect(acOpen, &QAction::triggered, this, [this]() { this->OpenFile(); });
		connect(acSave, &QAction::triggered, this, [this]() { this->SaveFile(); });
		connect(acSaveAs, &QAction::triggered, this, [this]() { this->SaveFileAs(); });
		connect(actionQuit, &QAction::triggered, this, &PathsTool::close);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acOpen);
		menuFile->addMenu(m_menuOpenRecent);
		menuFile->addSeparator();
		menuFile->addAction(acSave);
		menuFile->addAction(acSaveAs);
		menuFile->addSeparator();
		menuFile->addAction(actionQuit);


		// help menu
		QMenu *menuHelp = new QMenu("Help", m_menubar);

		QAction *actionAboutQt = new QAction(QIcon::fromTheme("help-about"), "About Qt Libraries...", menuHelp);
		QAction *actionAboutGl = new QAction(QIcon::fromTheme("help-about"), "About Renderer...", menuHelp);
		QAction *actionAbout = new QAction(QIcon::fromTheme("help-about"), "About Program...", menuHelp);
	
		actionAboutQt->setMenuRole(QAction::AboutQtRole);
		actionAbout->setMenuRole(QAction::AboutRole);
	
		connect(actionAboutQt, &QAction::triggered, this, []() { qApp->aboutQt(); });

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

		connect(actionAbout, &QAction::triggered, this, []()
		{

		});
	
		menuHelp->addAction(actionAboutQt);
		menuHelp->addAction(actionAboutGl);
		menuHelp->addAction(actionAbout);


		// menu bar
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

		// recent files
		if(m_sett.contains("recent_files"))
			SetRecentFiles(m_sett.value("recent_files").toStringList());
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
	auto dlg = std::make_unique<PathsTool>(nullptr);
	dlg->show();

	return app->exec();
}
// ----------------------------------------------------------------------------
