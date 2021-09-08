/**
 * line intersection test program
 * @author Tobias Weber <tweber@ill.fr>
 * @date 11-Nov-2020
 * @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license GPLv3, see 'LICENSE' file
 */

#include "lines.h"
#include "tlibs2/libs/helper.h"

#include <QtWidgets/QApplication>
#include <memory>
#include <iostream>


int main(int argc, char** argv)
{
	try
	{
		auto app = std::make_unique<QApplication>(argc, argv);
		app->setOrganizationName("tw");
		app->setApplicationName("lines");
		tl2::set_locales();

		auto vis = std::make_unique<LinesWnd>();
		vis->show();
		vis->raise();
		vis->activateWindow();

		return app->exec();
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	return -1;
}
