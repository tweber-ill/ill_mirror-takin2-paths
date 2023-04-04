#
# py workflow demo
# @author Tobias Weber <tweber@ill.fr>
# @date 14-july-2021
# @license GPLv3, see 'LICENSE' file
#
# -----------------------------------------------------------------------------
# TAS-Paths (part of the Takin software suite)
# Copyright (C) 2021  Tobias WEBER (Institut Laue-Langevin (ILL),
#                     Grenoble, France).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# -----------------------------------------------------------------------------
#

import sys
import os
import math as m

cwd = os.getcwd()
sys.path.append(cwd)
#print(sys.path)

import taspaths as tas


# -----------------------------------------------------------------------------
# helper functions
# -----------------------------------------------------------------------------
def error(msg):
	print("Error: %s" % msg)
	exit(-1)

def warning(msg):
	print("Warning: %s" % msg)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# options
# -----------------------------------------------------------------------------
write_pathmesh = False
write_path = False
show_plot = True
file_names = [  # possible instrument file paths
	"./res/mira.taspaths",
	"../res/mira.taspaths",
	"/usr/local/share/taspaths/res/mira.taspaths"
]
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# load instrument
# -----------------------------------------------------------------------------
print("Loading instrument definition...")

# create the instrument space and load an instrument definition
instrspace = tas.InstrumentSpace()
for file_name in file_names:
	[file_ok, file_date] = tas.InstrumentSpace.load(file_name, instrspace)

	if file_ok:
		print("Loaded \"%s\", dated %s." % (file_name, file_date))
		break

if not file_ok:
	error("Could not load \"%s\"." % (file_names[0]))
print("Instrument definition loaded.\n")
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# set-up a sample single-crystal
# -----------------------------------------------------------------------------
tascalc = tas.TasCalculator()
senses = [ -1., 1., -1. ]
tascalc.SetScatteringSenses(senses[0]>=0., senses[1]>=0., senses[2]>=0.)
tascalc.SetSampleLatticeConstants(5, 5, 5)
tascalc.SetSampleLatticeAngles(90, 90, 90, True)
tascalc.SetSampleScatteringPlane(1.,0.,0., 0.,1.,0.)
tascalc.SetSampleAngleOffset(90./180.*m.pi)  # depending on a3 convention
tascalc.UpdateB()
tascalc.UpdateUB()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# set-up the start and target coordinates of a path
# -----------------------------------------------------------------------------
print("Calculating path endpoints...")

# fixed-ki mode at ki = 1.4/A
tascalc.SetKi(1.4)
start_angles = tascalc.GetAngles(1.,0.,0., 1.3)
target_angles = tascalc.GetAngles(1.,1.,0., 1.2)

# take absolute angles
start_angles.anaXtalAngle = start_angles.anaXtalAngle * senses[2]
start_angles.sampleXtalAngle = start_angles.sampleXtalAngle * senses[1]
start_angles.sampleScatteringAngle = start_angles.sampleScatteringAngle * senses[1]
target_angles.anaXtalAngle = target_angles.anaXtalAngle * senses[2]
target_angles.sampleXtalAngle = target_angles.sampleXtalAngle * senses[1]
target_angles.sampleScatteringAngle = target_angles.sampleScatteringAngle * senses[1]

print("Start angles: a1 = %.2f deg, a5 = %.2f deg, a3 = %.2f deg, a4 = %.2f deg." % (
	start_angles.monoXtalAngle / m.pi*180.,
	start_angles.anaXtalAngle / m.pi*180.,
	start_angles.sampleXtalAngle / m.pi*180.,
	start_angles.sampleScatteringAngle / m.pi*180.))

print("Target angles: a1 = %.2f deg, a5 = %.2f deg, a3 = %.2f deg, a4 = %.2f deg." % (
	target_angles.monoXtalAngle / m.pi*180.,
	target_angles.anaXtalAngle / m.pi*180.,
	target_angles.sampleXtalAngle / m.pi*180.,
	target_angles.sampleScatteringAngle / m.pi*180.))
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# build path mesh
# -----------------------------------------------------------------------------
print("Building path mesh...")

# create the paths builder object
builder = tas.PathsBuilder()
builder.AddConsoleProgressHandler()
builder.SetInstrumentSpace(instrspace)
builder.SetTasCalculator(tascalc)
print("Path builder uses %d threads." % builder.GetMaxNumThreads())

# angular ranges to probe
a6_delta = 4./180.*m.pi
a4_delta = 4./180.*m.pi
a6_begin = -m.pi; a6_end = m.pi
a4_begin = -m.pi; a4_end = m.pi

builder.StartPathMeshWorkflow()

if not builder.CalculateConfigSpace(
	a6_delta, a4_delta,
	a6_begin, a6_end,
	a4_begin, a4_end):
	error("Angular configuration space could not be calculated.")

if not builder.CalculateWallContours(True, False):
	error("Obstacle contours could not be calculated.")

if not builder.CalculateLineSegments(False):
	error("Line segments could not be calculated.")

if not builder.CalculateVoronoi(False, tas.VoronoiBackend_BOOST, True):
	error("Voronoi diagram could not be calculated.")

if not builder.CalculateWallsIndexTree():
	error("Obstacle index tree could not be calculated.")

builder.FinishPathMeshWorkflow(True)
print("Finished building path mesh.\n")
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# find path
# -----------------------------------------------------------------------------
print("Calculating path...")

path = builder.FindPath(
	start_angles.anaXtalAngle * 2., start_angles.sampleScatteringAngle,
	target_angles.anaXtalAngle * 2., target_angles.sampleScatteringAngle,
	tas.PathStrategy_PENALISE_WALLS)
if not path.ok:
	error("No path could be found.")

builder.SetSubdivisionLength(0.5)
vertices = builder.GetPathVerticesAsPairs(path, True, True)

print("Finished calculating path.\n")
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# output
# -----------------------------------------------------------------------------
# write the path mesh vertices to a file
if write_pathmesh:
	if not builder.SaveToLinesTool("lines.xml"):
		warning("Could not save line segment diagram.")

# write the path vertices to a file
if write_path:
	with open("path.dat", "w") as datafile:
		for vertex in vertices:
			datafile.write("%.4f %.4f\n" % (vertex[0], vertex[1]))

# plot the angular configuration space
if show_plot:
	import matplotlib.pyplot as plt

	# plot obstacles
	numgroups = builder.GetNumberOfLineSegmentRegions()
	print("Number of regions: %d." % numgroups)
	for regionidx in range(numgroups):
		region = builder.GetLineSegmentRegionAsArray(regionidx)
		x1, y1, x2, y2 = zip(*region)
		plt.fill(x1, y1, "-", linewidth = 1.,
			fill = not builder.IsRegionInverted(regionidx),
			color = "#ff0000")

	# plot path
	x, y = zip(*vertices)
	plt.xlabel("Sample Scattering Angle 2\u03b8_S (deg)")
	plt.ylabel("Analyser Scattering Angle 2\u03b8_A (deg)")
	plt.plot(x, y, "-", linewidth=2)

	plt.show()
# -----------------------------------------------------------------------------
