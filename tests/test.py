#
# py interface test
# @author Tobias Weber <tweber@ill.fr>
# @date 14-july-2021
# @license see 'LICENSE' file
#

import sys
import os
import math

cwd = os.getcwd()
sys.path.append(cwd)
#print(sys.path)

import taspaths as tas


# -----------------------------------------------------------------------------
# load instrument
# -----------------------------------------------------------------------------
print("Loading instrument definition...")

file_name = "../res/instrument.taspaths"
instrspace = tas.InstrumentSpace()
[file_ok, file_date] = tas.InstrumentSpace.load(file_name, instrspace)

if file_ok:
	print("Loaded \"%s\" dated %s." % (file_name, file_date))
else:
	print("Cannot load \"%s\"" % (file_name))
	exit(-1)

print("Instrument definition loaded.\n")
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# build path mesh
# -----------------------------------------------------------------------------
print("Building path mesh...")

mem = tas.MemManager()
senses = mem.NewRealArray(3)
mem.SetRealArray(senses, 0, 1)
mem.SetRealArray(senses, 1, -1)
mem.SetRealArray(senses, 2, 1)

builder = tas.PathsBuilder()
builder.SetInstrumentSpace(instrspace)
builder.SetScatteringSenses(senses)
print("Path builder uses %d threads." % builder.GetMaxNumThreads())

# angular ranges to probe
angle_padding = 4.
a2_delta = 1./180.*math.pi
a4_delta = 2./180.*math.pi
a2_begin = 0. - angle_padding*a2_delta
a2_end = math.pi + angle_padding*a2_delta
a4_begin = -math.pi - angle_padding*a2_delta
a4_end = math.pi + angle_padding*a2_delta

if not builder.CalculateConfigSpace(
	a2_delta, a4_delta,
	a2_begin, a2_end,
	a4_begin, a4_end):
	print("Angular configuration space could not be calculated.")

if not builder.CalculateWallContours(True, False):
	print("Obstacle contours could not be calculated.")

if not builder.CalculateLineSegments():
	print("Line segments could not be calculated.")

if not builder.CalculateVoronoi(False):
	print("Voronoi diagram could not be calculated.")

print("Finished building path mesh.\n")
# -----------------------------------------------------------------------------


#if not builder.SaveToLinesTool("lines.xml"):
#	print("Could not save line segment diagram.")


# -----------------------------------------------------------------------------
# find path
# -----------------------------------------------------------------------------
print("Calculating path...")

a2_start = 40./180.*math.pi
a4_start = -80./180.*math.pi
a2_target = 120./180.*math.pi
a4_target = 105./180.*math.pi

path = builder.FindPath(a2_start, a4_start, a2_target, a4_target)
if not path.ok:
	print("No path could be found.")

builder.SetSubdivisionLength(0.5)
vertices = builder.GetPathVerticesAsPairs(path, True)

print("Finished calculating path.\n")

for vertex in vertices:
	print(vertex)
# -----------------------------------------------------------------------------
