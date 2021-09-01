/**
 * polygon splitting and kernel calculation test program
 * @author Tobias Weber <tweber@ill.fr>
 * @date 11-nov-2020
 * @note Forked on 4-aug-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 */

#include "poly.h"
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
		app->setApplicationName("polygon");
		tl2::set_locales();

		auto vis = std::make_unique<PolyWnd>();
		vis->show();

		return app->exec();
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	return -1;
}
