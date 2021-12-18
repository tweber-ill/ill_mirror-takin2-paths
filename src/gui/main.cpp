/**
 * TAS path tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-2021
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

#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QLoggingCategory>
#include <QtGui/QFileOpenEvent>
#include <QtWidgets/QApplication>

#include <optional>
#include <boost/predef.h>

#if BOOST_OS_MACOS
	#include <unistd.h>
	#include <pwd.h>
#endif

#include "PathsTool.h"
#include "settings_variables.h"
#include "tlibs2/libs/file.h"
#include "tlibs2/libs/str.h"
#include "tlibs2/libs/helper.h"


/**
 * the get application directory (if this exists)
 */
static std::optional<std::string> get_appdir_path(const std::string& _binpath)
{
	fs::path binpath = fs::absolute(_binpath);

#if BOOST_OS_MACOS
	std::string dir = binpath.filename().string();
	std::string parentdir = binpath.parent_path().filename().string();

	if(tl2::str_is_equal<std::string>(dir, "macos", false) &&
		tl2::str_is_equal<std::string>(parentdir, "contents", false))
	{
		return binpath.parent_path().parent_path().string();
	}
#endif

	return std::nullopt;
}


/**
 * main application
 */
class PathsApp : public QApplication
{
public:
	/**
	 * constructor getting command-line arguments
	 */
	explicit PathsApp(int& argc, char **argv) : QApplication{argc, argv}
	{
		// application settings
		//QApplication::setAttribute(Qt::AA_NativeWindows, true);
		QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
		QApplication::addLibraryPath(QDir::currentPath() + QDir::separator() + "Qt_Plugins");

		setOrganizationName("eu.ill.cs.takin");
		setApplicationName("taspaths");
		//setApplicationDisplayName(TASPATHS_TITLE);
		setApplicationVersion(TASPATHS_VERSION);

		// paths
		g_apppath = applicationDirPath().toStdString();
		g_appdirpath = get_appdir_path(g_apppath);
#if BOOST_OS_MACOS
		// get the real home directory, not the sandboxed one,
		// see: https://developer.apple.com/forums/thread/107593
		const ::uid_t uid = ::getuid();
		const ::passwd* pwd = ::getpwuid(uid);
		g_homepath = pwd->pw_dir;
#else
		g_homepath = QDir::homePath().toStdString();
#endif

		// standard paths
		if(QStringList deskdirs = QStandardPaths::standardLocations(
			QStandardPaths::DesktopLocation); deskdirs.size())
			g_desktoppath = deskdirs[0].toStdString();
		else
			g_desktoppath = g_homepath;

		if(QStringList docdirs = QStandardPaths::standardLocations(
			QStandardPaths::DocumentsLocation); docdirs.size())
			g_docpath = docdirs[0].toStdString();
		else
			g_docpath = g_homepath;

		if(QStringList imgdirs = QStandardPaths::standardLocations(
			QStandardPaths::PicturesLocation); imgdirs.size())
			g_imgpath = imgdirs[0].toStdString();
		else
			g_imgpath = g_docpath;

		// override standard paths with own subdir
		if(g_use_taspaths_subdir)
		{
			//std::string taspaths_subdir = "TAS-Paths Files";
			std::string taspaths_subdir = "Desktop";

			QDir taspathsdir(g_homepath.c_str());
			bool taspaths_subdir_ok = false;

			taspaths_subdir_ok = taspathsdir.exists(taspaths_subdir.c_str());
			//if(!taspaths_subdir_ok)
			//	taspaths_subdir_ok = taspathsdir.mkdir(taspaths_subdir.c_str());

			//if(taspaths_subdir_ok)
			{
				taspathsdir.cd(taspaths_subdir.c_str());
				g_docpath = taspathsdir.absolutePath().toStdString();
				g_imgpath = taspathsdir.absolutePath().toStdString();
			}
		}

		// qt plugin libraries
		addLibraryPath(applicationDirPath() + QDir::separator() + ".." +
			QDir::separator() + "Libraries" + QDir::separator() + "Qt_Plugins");
#ifdef DEBUG
		if(g_appdirpath)
			std::cout << "Application directory path: " << (*g_appdirpath) << "." << std::endl;
		std::cout << "Application binary path: " << g_apppath << "." << std::endl;
#endif
	}


	/**
	 * no default constructor
	 */
	PathsApp() = delete;


	/**
	 * default destructor
	 */
	virtual ~PathsApp() = default;


	/**
	 * get the initial file to be loaded
	 */
	const QString& GetInitialFile() const
	{
		return m_init_file;
	}


	/**
	 * associate a main window with the application
	 */
	void SetMainWnd(const std::shared_ptr<PathsTool>& paths)
	{
		m_paths = paths;
	}


protected:
	/**
	 * receive file open events
	 * @see https://doc.qt.io/qt-5/qfileopenevent.html
	 */
	virtual bool event(QEvent *evt) override
	{
		if(!evt)
			return false;

		switch(evt->type())
		{
			// open a file
			case QEvent::FileOpen:
			{
				m_init_file = static_cast<QFileOpenEvent*>(evt)->file();

				// if the main window is ready, directly open it
				if(m_paths)
					m_paths->OpenFile(m_init_file);
				break;
			}

			default:
			{
				break;
			}
		}

		return QApplication::event(evt);
	}


private:
	// file requested to open
	QString m_init_file{};

	// main application window
	std::shared_ptr<PathsTool> m_paths{};
};


/**
 * main entry point
 */
int main(int argc, char** argv)
{
	try
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

		// default gl surface format
		tl2::set_gl_format(true, _GL_MAJ_VER, _GL_MIN_VER, 8);
		tl2::set_locales();

		// set maximum number of threads
		g_maxnum_threads = std::max<unsigned int>(1, std::thread::hardware_concurrency()/2);

		// create application
		auto app = std::make_unique<PathsApp>(argc, argv);

		// set up resource paths
		fs::path apppath = g_apppath;
		g_res.AddPath((apppath / "res").string());
		g_res.AddPath((apppath / ".." / "res").string());
		g_res.AddPath((apppath / "Resources").string());
		g_res.AddPath((apppath / ".." / "Resources").string());
		g_res.AddPath(g_apppath);
		g_res.AddPath((apppath / "..").string());
		g_res.AddPath(fs::path("/usr/local/share/taspaths/res").string());
		g_res.AddPath(fs::path("/usr/share/taspaths/res").string());
		g_res.AddPath(fs::path("/usr/local/share/taspaths").string());
		g_res.AddPath(fs::path("/usr/share/taspaths").string());
		g_res.AddPath(fs::current_path().string());

		// make type definitions known as qt meta objects
		qRegisterMetaType<t_real>("t_real");
		qRegisterMetaType<t_vec>("t_vec");
		qRegisterMetaType<t_mat>("t_mat");
		qRegisterMetaType<std::string>("std::string");
		qRegisterMetaType<std::size_t>("std::size_t");
		qRegisterMetaType<CalculationState>("CalculationState");

		// create main window
		auto mainwnd = std::make_shared<PathsTool>(nullptr);

		// the main window is not yet ready, indirectly open a given file
		if(argc > 1)
			mainwnd->SetInitialInstrumentFile(argv[1]);
		else if(const QString& init_file = app->GetInitialFile(); init_file!="")
			mainwnd->SetInitialInstrumentFile(init_file.toStdString());

		// sequence to show the window,
		// see: https://doc.qt.io/qt-5/qdialog.html#code-examples
		mainwnd->show();
		mainwnd->raise();
		mainwnd->activateWindow();

		// run application
		app->SetMainWnd(mainwnd);
		return app->exec();
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return -1;
	}

	return 0;
}
