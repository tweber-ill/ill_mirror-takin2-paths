/**
 * TAS path tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
 * @license GPLv3, see 'LICENSE' file
 */

#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QLoggingCategory>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include <string>
#include <memory>
#include <unordered_map>

#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/str.h"
#include "tlibs2/libs/file.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/helper.h"
#include "src/core/ptree_algos.h"

#include "PathsRenderer.h"
#include "Properties.h"
#include "About.h"


#define MAX_RECENT_FILES 16
#define PROG_TITLE "TAS Path Optimisation Tool"
#define PROG_IDENT "takin_paths"
#define FILE_BASENAME "paths."



// ----------------------------------------------------------------------------
class PathsTool : public QMainWindow
{ /*Q_OBJECT*/
private:
	QSettings m_sett{"takin", "paths"};

	// renderer
	std::shared_ptr<PathsRenderer> m_renderer{std::make_shared<PathsRenderer>(this)};

	// gl info strings
	std::string m_gl_ver, m_gl_shader_ver, m_gl_vendor, m_gl_renderer;

	QStatusBar *m_statusbar{nullptr};
	QLabel *m_labelStatus{nullptr};

	QMenu *m_menuOpenRecent{nullptr};
	QMenuBar *m_menubar{nullptr};

	std::shared_ptr<AboutDlg> m_dlgAbout;
	std::shared_ptr<TASPropertiesDockWidget> m_tasProperties;

	// recent file list and currently active file
	QStringList m_recentFiles;
	QString m_curFile;

	// instrument configuration
	InstrumentSpace m_instrspace;

	// mouse picker
	t_real m_mouseX, m_mouseY;
	std::string m_curObj;


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

		m_instrspace.Clear();
		m_renderer->LoadInstrument(m_instrspace);
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
		try
		{
			if(file=="" || !QFile::exists(file))
				return false;


			// load xml
			pt::ptree prop;
			std::ifstream ifstr{file.toStdString()};

			if(!ifstr)
			{
				QMessageBox::critical(this, "Error", "Could not load file.");
				return false;
			}

			// check format and version
			pt::read_xml(ifstr, prop);
			if(auto opt = prop.get_optional<std::string>(FILE_BASENAME "ident");
				!opt || *opt != PROG_IDENT)
			{
				QMessageBox::critical(this, "Error", "Not a recognised file format. Ignoring.");
				return false;
			}

			if(auto optTime = prop.get_optional<t_real>(FILE_BASENAME "timestamp"); optTime)
				std::cout << "Loading file \"" << file.toStdString()
					<< "\" dated " << tl2::epoch_to_str(*optTime) << "." << std::endl;


			// get variables from config file
			std::unordered_map<std::string, std::string> propvars;

			if(auto vars = prop.get_child_optional(FILE_BASENAME "variables"); vars)
			{
				// iterate variables
				for(const auto &var : *vars)
				{
					const auto& key = var.first;
					std::string val = var.second.get<std::string>("<xmlattr>.value", "");
					//std::cout << key << " = " << val << std::endl;

					propvars.insert(std::make_pair(key, val));
				}

				replace_ptree_values(prop, propvars);
			}


			// load instrument definition
			if(auto instr = prop.get_child_optional(FILE_BASENAME "instrument_space"); instr)
			{
				if(!m_instrspace.Load(*instr))
				{
					QMessageBox::critical(this, "Error", "Instrument configuration could not be loaded.");
					return false;
				}

				t_real monoScAngle = m_instrspace.GetInstrument().GetMonochromator().GetAxisAngleOut()*t_real{180}/tl2::pi<t_real>;
				t_real sampleScAngle = m_instrspace.GetInstrument().GetSample().GetAxisAngleOut()*t_real{180}/tl2::pi<t_real>;
				t_real anaScAngle = m_instrspace.GetInstrument().GetAnalyser().GetAxisAngleOut()*t_real{180}/tl2::pi<t_real>;
				m_tasProperties->GetWidget()->SetMonoScatteringAngle(monoScAngle);
				m_tasProperties->GetWidget()->SetSampleScatteringAngle(sampleScAngle);
				m_tasProperties->GetWidget()->SetAnaScatteringAngle(anaScAngle);
			}
			else
			{
				QMessageBox::critical(this, "Error", "No instrument definition found.");
				return false;
			}


			SetCurrentFile(file);
			AddRecentFile(file);

			m_renderer->LoadInstrument(m_instrspace);
			m_instrspace.GetInstrument().AddUpdateSlot([this]()->void
			{
				if(m_renderer)
					m_renderer->UpdateInstrument();
			});
		}
		catch(const std::exception& ex)
		{
			QMessageBox::critical(this, "Error",
				QString{"Instrument configuration error: "} + ex.what() + QString{"."});
			return false;
		}
		return true;
	}


	/**
	 * save file
	 */
	bool SaveFile(const QString &file)
	{
		if(file=="")
			return false;

		pt::ptree prop;

		// set format and version
		prop.put(FILE_BASENAME "ident", PROG_IDENT);
		prop.put(FILE_BASENAME "timestamp", tl2::var_to_str(tl2::epoch<t_real>()));

		std::ofstream ofstr{file.toStdString()};
		if(!ofstr)
		{
			QMessageBox::critical(this, "Error", "Could not save file.");
			return false;
		}

		ofstr.precision(6);
		pt::write_xml(ofstr, prop, pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));

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

		m_renderer->LoadInstrument(m_instrspace);
	}


	/**
	 * mouse coordinates on base plane
	 */
	void CursorCoordsChanged(t_real_gl x, t_real_gl y)
	{
		m_mouseX = x;
		m_mouseY = y;
		UpdateStatusLabel();
	}


	/**
	 * mouse is over an object
	 */
	void PickerIntersection(const t_vec3_gl* pos, std::string obj_name, const t_vec3_gl* posSphere)
	{
		m_curObj = obj_name;
		UpdateStatusLabel();
	}


	/**
	 * clicked on an object
	 */
	void ObjectClicked(const std::string& obj, bool left, bool middle, bool right)
	{
		std::cout << "Clicked on " << obj << "." << std::endl;
		m_renderer->CentreCam(obj);
	}


	/**
	 * dragging an object
	 */
	void ObjectDragged(const std::string& obj, t_real_gl x, t_real_gl y)
	{
		std::cout << "Dragging " << obj << " to (" << x << ", " << y << ")." << std::endl;
	}


	void UpdateStatusLabel()
	{
		std::ostringstream ostr;
		ostr.precision(4);
		ostr << "x = " << m_mouseX << " m, y = " << m_mouseY << " m";
		if(m_curObj != "")
			ostr << ", object: " << m_curObj;
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
		// rendering widget
		// --------------------------------------------------------------------
		auto plotpanel = new QWidget(this);

		connect(m_renderer.get(), &PathsRenderer::FloorPlaneCoordsChanged, this, &PathsTool::CursorCoordsChanged);
		connect(m_renderer.get(), &PathsRenderer::PickerIntersection, this, &PathsTool::PickerIntersection);
		connect(m_renderer.get(), &PathsRenderer::ObjectClicked, this, &PathsTool::ObjectClicked);
		connect(m_renderer.get(), &PathsRenderer::ObjectDragged, this, &PathsTool::ObjectDragged);
		connect(m_renderer.get(), &PathsRenderer::AfterGLInitialisation, this, &PathsTool::AfterGLInitialisation);

		auto pGrid = new QGridLayout(plotpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		pGrid->addWidget(m_renderer.get(), 0,0,1,4);

		setCentralWidget(plotpanel);
		// --------------------------------------------------------------------


		// --------------------------------------------------------------------
		// dock widgets
		// --------------------------------------------------------------------
		m_tasProperties = std::make_shared<TASPropertiesDockWidget>(this);
		addDockWidget(Qt::RightDockWidgetArea, m_tasProperties.get());

		auto* taswidget = m_tasProperties->GetWidget().get();
		connect(taswidget, &TASPropertiesWidget::MonoScatteringAngleChanged,
		[this](t_real angle) -> void
		{
			m_instrspace.GetInstrument().GetMonochromator().
				SetAxisAngleOut(angle/t_real{180}*tl2::pi<t_real>);
		});
		connect(taswidget, &TASPropertiesWidget::SampleScatteringAngleChanged,
		[this](t_real angle) -> void
		{
			m_instrspace.GetInstrument().GetSample().
				SetAxisAngleOut(angle/t_real{180}*tl2::pi<t_real>);
		});
		connect(taswidget, &TASPropertiesWidget::AnaScatteringAngleChanged,
		[this](t_real angle) -> void
		{
			m_instrspace.GetInstrument().GetAnalyser().
				SetAxisAngleOut(angle/t_real{180}*tl2::pi<t_real>);
		});
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
		QAction *acQuit = new QAction(QIcon::fromTheme("application-exit"), "Quit", menuFile);

		// shortcuts
		acNew->setShortcut(QKeySequence::New);
		acOpen->setShortcut(QKeySequence::Open);
		acSave->setShortcut(QKeySequence::Save);
		acSaveAs->setShortcut(QKeySequence::SaveAs);
		acQuit->setShortcut(QKeySequence::Quit);

		m_menuOpenRecent = new QMenu("Open Recent", menuFile);
		m_menuOpenRecent->setIcon(QIcon::fromTheme("document-open-recent"));

		acQuit->setMenuRole(QAction::QuitRole);


		connect(acNew, &QAction::triggered, this, [this]() { this->NewFile(); });
		connect(acOpen, &QAction::triggered, this, [this]() { this->OpenFile(); });
		connect(acSave, &QAction::triggered, this, [this]() { this->SaveFile(); });
		connect(acSaveAs, &QAction::triggered, this, [this]() { this->SaveFileAs(); });
		connect(acQuit, &QAction::triggered, this, &PathsTool::close);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acOpen);
		menuFile->addMenu(m_menuOpenRecent);
		menuFile->addSeparator();
		menuFile->addAction(acSave);
		menuFile->addAction(acSaveAs);
		menuFile->addSeparator();
		menuFile->addAction(acQuit);


		// view menu
		QMenu *menuView = new QMenu("View", m_menubar);

		QAction *acPersp = new QAction("Perspective Projection", menuView);
		acPersp->setCheckable(true);
		acPersp->setChecked(true);

		connect(acPersp, &QAction::toggled, this, [this](bool b)
		{
			if(m_renderer)
				m_renderer->SetPerspectiveProjection(b);
		});

		menuView->addAction(m_tasProperties->toggleViewAction());
		menuView->addSeparator();
		menuView->addAction(acPersp);


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

		connect(actionAbout, &QAction::triggered, this, [this]()
		{
			if(!this->m_dlgAbout)
				this->m_dlgAbout = std::make_shared<AboutDlg>(this);

			m_dlgAbout->show();
			m_dlgAbout->activateWindow();
		});

		menuHelp->addAction(actionAboutQt);
		menuHelp->addAction(actionAboutGl);
		menuHelp->addSeparator();
		menuHelp->addAction(actionAbout);


		// menu bar
		m_menubar->addMenu(menuFile);
		m_menubar->addMenu(menuView);
		m_menubar->addMenu(menuHelp);
		//m_menubar->setNativeMenuBar(0);
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
	// qt log handler
	QLoggingCategory::setFilterRules("*=true\n*.debug=false\n*.info=false\n");
	qInstallMessageHandler([](QtMsgType ty, const QMessageLogContext& ctx, const QString& log) -> void
	{
		auto get_msg_type = [](const QtMsgType& _ty) -> std::string
		{
			switch(_ty)
			{
				case QtDebugMsg: return "debug";
				case QtWarningMsg: return "warning";
				case QtCriticalMsg: return "critical error";
				case QtFatalMsg: return "fatal error";
				case QtInfoMsg: return "info";
				default: return "<n/a>";
			}
		};

		auto get_str = [](const char* pc) -> std::string
		{
			if(!pc) return "<n/a>";
			return std::string{"\""} + std::string{pc} + std::string{"\""};
		};

		std::cerr << "Qt " << get_msg_type(ty);
		if(ctx.function)
		{
			std::cerr << " in "
				<< "file " << get_str(ctx.file) << ", "
				<< "function " << get_str(ctx.function) << ", "
				<< "line " << ctx.line;
		}
		std::cerr << ": " << log.toStdString() << std::endl;
	});

	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();

	//QApplication::setAttribute(Qt::AA_NativeWindows, true);
	QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
	QApplication::addLibraryPath(QDir::currentPath() + QDir::separator() + "qtplugins");
	auto app = std::make_unique<QApplication>(argc, argv);
	app->addLibraryPath(app->applicationDirPath() + QDir::separator() + "qtplugins");

	auto dlg = std::make_unique<PathsTool>(nullptr);
	dlg->show();

	return app->exec();
}
// ----------------------------------------------------------------------------
