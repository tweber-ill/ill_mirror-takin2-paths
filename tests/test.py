#
# py interface test
# @author Tobias Weber <tweber@ill.fr>
# @date 14-July-2021
# @license see 'LICENSE' file
#

import sys
import os

cwd = os.getcwd()
sys.path.append(cwd)
#print(sys.path)

import taspaths as tas


# load instrument
file_name = "../res/instrument.taspaths"
instrspace = tas.InstrumentSpace()
[file_ok, file_date] = tas.InstrumentSpace.load(file_name, instrspace)

if file_ok:
	print("Loaded \"%s\" dated %s." % (file_name, file_date))
else:
	print("Cannot load \"%s\"" % (file_name))
	exit(-1)
