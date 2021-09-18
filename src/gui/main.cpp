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
#include <QtCore/QLoggingCategory>
#include <QtWidgets/QApplication>

#include "PathsTool.h"
#include "tlibs2/libs/helper.h"


/**
 * main
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

		//QApplication::setAttribute(Qt::AA_NativeWindows, true);
		QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
		QApplication::addLibraryPath(QDir::currentPath() + QDir::separator() + "Qt_Plugins");

		auto app = std::make_unique<QApplication>(argc, argv);
		app->setOrganizationName("tw");
		app->setApplicationName("taspaths");
		app->setApplicationDisplayName("TAS-Paths");
		app->setApplicationVersion("0.9");

		g_apppath = app->applicationDirPath().toStdString();
		app->addLibraryPath(app->applicationDirPath() + QDir::separator() + ".." +
			QDir::separator() + "Libraries" + QDir::separator() + "Qt_Plugins");
		std::cout << "Application binary path: " << g_apppath << "." << std::endl;

		// make type definitions known as qt meta objects
		qRegisterMetaType<t_real>("t_real");
		qRegisterMetaType<t_vec>("t_vec");
		qRegisterMetaType<t_mat>("t_mat");
		qRegisterMetaType<std::string>("std::string");

		auto mainwnd = std::make_unique<PathsTool>(nullptr);
		if(argc > 1)
			mainwnd->SetInitialInstrumentFile(argv[1]);
		mainwnd->show();
		mainwnd->raise();
		mainwnd->activateWindow();

		return app->exec();
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
		return -1;
	}

	return 0;
}
