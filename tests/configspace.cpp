/**
 * create an image of the instrument's configuration space
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @license see 'LICENSE' file
 *
 * g++-10 -std=c++20 -I.. -o configspace configspace.cpp ../src/core/Geometry.cpp ../src/core/Instrument.cpp -lboost_filesystem
 */

#include <iostream>
#include "src/core/Instrument.h"


int main(int argc, char** argv)
{
	if(argc < 2)
	{
		std::cerr << "Please give an instrument file name." << std::endl;
		return -1;
	}

	// load instrument definition
	std::string filename = argv[1];
	InstrumentSpace instrspace;

	if(auto [ok, msg] = load_instrumentspace(filename, instrspace); !ok)
	{
		std::cerr << "Error: " << msg << std::endl;
		return -1;
	}

	// TODO

	return 0;
}
