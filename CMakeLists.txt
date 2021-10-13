#
# @author Tobias Weber <tweber@ill.fr>
# @date feb-2021
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

cmake_minimum_required(VERSION 3.0)

project(taspaths)
enable_language(CXX)


list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/cmake"
	"${PROJECT_SOURCE_DIR}/tlibs2/cmake"
)

#if(NOT "$ENV{LOCAL_BOOST_CMAKE_DIR}" STREQUAL "")
#	message("Using local boost cmake dir: $ENV{LOCAL_BOOST_CMAKE_DIR}.")
#
#	list(PREPEND CMAKE_MODULE_PATH
#		"$ENV{LOCAL_BOOST_CMAKE_DIR}"
#	)
#endif()


option(USE_QT6 "use qt 6" FALSE)
option(USE_LAPACK "use lapack" TRUE)
option(USE_OVD "use openvoronoi" TRUE)
option(USE_CGAL "use cgal" TRUE)
option(USE_PY "use python scripting" FALSE)
option(BUILD_TEST_TOOLS "build test tools" TRUE)
option(UNIT_TESTS "build unit tests" FALSE)


if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(CMAKE_VERBOSE_MAKEFILE TRUE)
endif()


set(GL_MAJOR_VER 3)
set(GL_MINOR_VER 3)

if(USE_QT6)
	set(QT_VER 6)
else()
	set(QT_VER 5)
endif()


message("Build type: ${CMAKE_BUILD_TYPE}.")
message("Compiler: " ${CMAKE_CXX_COMPILER_ID}.)
message("Selected Qt version ${QT_VER}.")
message("Selected GL version ${GL_MAJOR_VER}.${GL_MINOR_VER}.")


set(WARN_OPTS -Wall -Wextra -Weffc++)
add_compile_options(${WARN_OPTS})


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	# generate debug symbols
	add_compile_options(-g -ggdb)

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_compile_options("-DNDEBUG")
	add_compile_options("-Wno-#pragma-messages")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		# remove pragma messages
		# see: https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html
		add_compile_options(-fcompare-debug-second)
	endif()
endif()


# -----------------------------------------------------------------------------
# compiler settings
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)

add_definitions(-D_GL_MAJ_VER=${GL_MAJOR_VER} -D_GL_MIN_VER=${GL_MINOR_VER})
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# system specific settings
# -----------------------------------------------------------------------------
message("Building for ${CMAKE_SYSTEM_NAME} systems.")

set(BOOST_SUFFIX)
set(MINGW_WINSOCK)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# pass linker --subsystem option
	add_compile_options(-Wl,--subsystem,windows)

	set(BOOST_SUFFIX -x64)
	set(MINGW_WINSOCK "ws2_32")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_compile_options(-mmacosx-version-min=10.15)
endif()

include_directories("${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/externals")

if(QT_VER EQUAL 6)
	include_directories("${Qt6Core_INCLUDE_DIRS}/..")
elseif(QT_VER EQUAL 5)
	include_directories("${Qt5Core_INCLUDE_DIRS}/..")
endif()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# packages
# -----------------------------------------------------------------------------
find_package(Threads REQUIRED)

set(Boost_NO_BOOST_CMAKE FALSE)
set(Boost_USE_MULTITHREADED TRUE)
set(Boost_FIND_QUIETLY FALSE)

find_package(Boost REQUIRED COMPONENTS filesystem${BOOST_SUFFIX})
message("Using Boost version ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}.")
include_directories("${Boost_INCLUDE_DIRS}/..")


find_package(Qhull)
if(Qhull_FOUND)
	add_definitions(-DUSE_QHULL)
	include_directories("${Qhull_INCLUDE_DIRS}")
endif()


if(USE_OVD)
	find_package(OVD)
	if(OVD_FOUND)
		message("Openvoronoi enabled.")
		add_definitions(-DUSE_OVD)
		include_directories("${OVD_INCLUDE_DIRS}")
	else()
		message("Openvoronoi disabled.")
		set(OVD_LIBRARIES "")
	endif()
endif()


if(USE_CGAL)
	message("CGAL enabled.")
	add_definitions(-DUSE_CGAL)
	set(CGAL_LIBRARIES "-lgmp")
endif()


if(USE_LAPACK)
	find_package(Lapacke)
	if(Lapacke_FOUND)
		message("Lapacke enabled.")
		add_definitions(-DUSE_LAPACK)
		include_directories("${Lapacke_INCLUDE_DIRS}")
	else()
		message("Lapacke disabled.")
	endif()
endif()


if(USE_PY)
	find_package(Python3 REQUIRED COMPONENTS Interpreter Development REQUIRED)
	message("Python version ${Python3_VERSION} enabled; packages: ${Python3_SITEARCH}.")

	cmake_policy(SET CMP0078 NEW)
	cmake_policy(SET CMP0086 NEW)

	find_package(SWIG REQUIRED)
	set(UseSWIG_TARGET_NAME_PREFERENCE STANDARD)
	include(${SWIG_USE_FILE})

	set_source_files_properties(scripting/taspaths.i PROPERTIES CPLUSPLUS TRUE)
	set_source_files_properties(scripting/taspaths.i PROPERTIES SWIG_FLAGS "-I ${PROJECT_SOURCE_DIR}")

	swig_add_library(taspaths_py LANGUAGE python SOURCES scripting/taspaths.i)

	target_link_libraries(taspaths_py
		taspaths_core
		Python3::Python Threads::Threads
		"${Boost_LIBRARIES}" "${Lapacke_LIBRARIES}"
	)
else()
	message("Python disabled.")
endif()


if(QT_VER EQUAL 6)
	find_package(Qt6 REQUIRED
		COMPONENTS Core Gui Svg Widgets
			OpenGL OpenGLWidgets
			PrintSupport)
elseif(QT_VER EQUAL 5)
	find_package(Qt5 REQUIRED
		COMPONENTS Core Gui Svg Widgets OpenGL
			PrintSupport)
else()
	message(FATAL_ERROR "Unknown Qt version selected: ${QT_VER}")
endif()

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# more compiler settings
# -----------------------------------------------------------------------------
add_compile_options(${Boost_CXX_FLAGS})
#add_definitions(-DQCUSTOMPLOT_USE_OPENGL)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	# disable some warnings for generated source files
	set_source_files_properties("${PROJECT_BINARY_DIR}/qcustomplot_local_autogen/mocs_compilation.cpp"
		PROPERTIES COMPILE_FLAGS "-Wno-effc++")
	set_source_files_properties("${PROJECT_BINARY_DIR}/taspaths_autogen/mocs_compilation.cpp"
		PROPERTIES COMPILE_FLAGS "-Wno-effc++")

	# disable warnings for external source files
	set_source_files_properties("${PROJECT_SOURCE_DIR}/externals/qcustomplot/qcustomplot.cpp"
		PROPERTIES COMPILE_FLAGS "-Wno-effc++ \
			-Wno-implicit-fallthrough \
			-Wno-deprecated-enum-enum-conversion \
			-Wno-misleading-indentation")
endif()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# target library settings
# -----------------------------------------------------------------------------
# core library
add_library(taspaths_core STATIC
	src/core/Geometry.cpp src/core/Geometry.h
	src/core/Axis.cpp src/core/Axis.h
	src/core/Instrument.cpp src/core/Instrument.h
	src/core/InstrumentSpace.cpp src/core/InstrumentSpace.h
	src/core/PathsBuilder.cpp src/core/PathsBuilder.h
	src/core/PathsExporter.cpp src/core/PathsExporter.h
	src/core/TasCalculator.cpp src/core/TasCalculator.h
	src/core/types.h

	src/libs/lines.h src/libs/graphs.h
	src/libs/voronoi.h src/libs/voronoi_lines.h
	src/libs/hull.h src/libs/img.h
	src/libs/ptree.h
)

set_property(TARGET taspaths_core
	PROPERTY POSITION_INDEPENDENT_CODE True)

target_link_libraries(taspaths_core
	"${Boost_LIBRARIES}"
	"${Lapacke_LIBRARIES}"
	#"${Qhull_LIBRARIES}"
	"${CGAL_LIBRARIES}"
	Threads::Threads
)


# make a 3rd party qcustomplot library
add_library(qcustomplot_local SHARED
	externals/qcustomplot/qcustomplot.cpp externals/qcustomplot/qcustomplot.h
)

set_property(TARGET qcustomplot_local
	PROPERTY POSITION_INDEPENDENT_CODE True)

if(QT_VER EQUAL 6)
	target_link_libraries(qcustomplot_local
		Qt6::Core Qt6::Gui Qt6::Widgets
		#Qt6::OpenGL Qt6::OpenGLWidgets
		Qt6::PrintSupport
	)
elseif(QT_VER EQUAL 5)
	target_link_libraries(qcustomplot_local
		Qt5::Core Qt5::Gui Qt5::Widgets
		#Qt5::OpenGL
		Qt5::PrintSupport
	)
endif()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# target executable settings
# -----------------------------------------------------------------------------
add_executable(taspaths
	src/gui/main.cpp
	src/gui/PathsTool.cpp src/gui/PathsTool.h
	src/gui/Settings.cpp src/gui/Settings.h
	src/gui/About.cpp src/gui/About.h
	src/gui/Licenses.cpp src/gui/Licenses.h
	src/gui/PathsRenderer.cpp src/gui/PathsRenderer.h
	src/gui/dock/TASProperties.cpp src/gui/dock/TASProperties.h
	src/gui/dock/XtalProperties.cpp src/gui/dock/XtalProperties.h
	src/gui/dock/CoordProperties.cpp src/gui/dock/CoordProperties.h
	src/gui/dock/PathProperties.cpp src/gui/dock/PathProperties.h
	src/gui/dock/CamProperties.cpp src/gui/dock/CamProperties.h
	src/gui/ConfigSpace.cpp src/gui/ConfigSpace.h
	src/gui/XtalConfigSpace.cpp src/gui/XtalConfigSpace.h
	src/gui/GeoBrowser.cpp src/gui/GeoBrowser.h

	src/libs/lines.h src/libs/graphs.h
	src/libs/voronoi.h src/libs/voronoi_lines.h
	src/libs/hull.h src/libs/img.h
	src/libs/ptree.h
	src/libs/proc.cpp src/libs/proc.h

	tlibs2/libs/qt/gl.cpp tlibs2/libs/qt/gl.h
)


if(BUILD_TEST_TOOLS)
	add_executable(taspaths_hull
		src/tools/hull_main.cpp
		src/tools/hull.cpp src/tools/hull.h
		src/tools/vertex.cpp src/tools/vertex.h
		src/tools/about.cpp src/tools/about.h
		src/tools/settings.cpp src/tools/settings.h

		src/libs/lines.h src/libs/hull.h
		src/libs/voronoi.h
	)

	add_executable(taspaths_lines
		src/tools/lines_main.cpp
		src/tools/lines.cpp src/tools/lines.h
		src/tools/vertex.cpp src/tools/vertex.h
		src/tools/about.cpp src/tools/about.h
		src/tools/settings.cpp src/tools/settings.h

		src/libs/lines.h src/libs/hull.h
		src/libs/voronoi_lines.h
	)

	add_executable(taspaths_poly
		src/tools/poly_main.cpp
		src/tools/poly.cpp src/tools/poly.h
		src/tools/vertex.cpp src/tools/vertex.h
		src/tools/about.cpp src/tools/about.h
		src/tools/settings.cpp src/tools/settings.h

		src/libs/lines.h src/libs/hull.h
		src/libs/voronoi_lines.h
	)
endif()


#if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#	target_link_options(taspaths
#		# create an __info_plist section in the binary
#		PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_SOURCE_DIR}/../../../core/setup_mac/InfoBundle.plist
#	)
#endif()

if(QT_VER EQUAL 6)
	target_link_libraries(taspaths
		taspaths_core
		"${Boost_LIBRARIES}"
		"${Lapacke_LIBRARIES}"
		#"${Qhull_LIBRARIES}"
		"${MINGW_WINSOCK}"
		Threads::Threads
		Qt6::Core Qt6::Gui Qt6::Widgets
		Qt6::OpenGL Qt6::OpenGLWidgets
		Qt6::PrintSupport
		qcustomplot_local
	)


	if(BUILD_TEST_TOOLS)
		target_link_libraries(taspaths_hull
			"${Boost_LIBRARIES}"
			"${Lapacke_LIBRARIES}"
			"${Qhull_LIBRARIES}"
			Qt6::Core Qt6::Gui Qt6::Widgets
			Qt6::Svg
		)

		target_link_libraries(taspaths_lines
			"${Boost_LIBRARIES}"
			"${Lapacke_LIBRARIES}"
			"${OVD_LIBRARIES}"
			"${CGAL_LIBRARIES}"
			"${MINGW_WINSOCK}"
			Threads::Threads
			Qt6::Core Qt6::Gui Qt6::Widgets
			Qt6::Svg
		)

		target_link_libraries(taspaths_poly
			"${Boost_LIBRARIES}"
			"${Lapacke_LIBRARIES}"
			Qt6::Core Qt6::Gui Qt6::Widgets
			Qt6::Svg
		)
	endif()

elseif(QT_VER EQUAL 5)
	target_link_libraries(taspaths
		taspaths_core
		"${Boost_LIBRARIES}"
		"${Lapacke_LIBRARIES}"
		#"${Qhull_LIBRARIES}"
		"${MINGW_WINSOCK}"
		Threads::Threads
		Qt5::Core Qt5::Gui Qt5::Widgets
		Qt5::OpenGL Qt5::PrintSupport
		qcustomplot_local
	)


	if(BUILD_TEST_TOOLS)
		target_link_libraries(taspaths_hull
			"${Boost_LIBRARIES}"
			"${Lapacke_LIBRARIES}"
			"${Qhull_LIBRARIES}"
			##/usr/x86_64-w64-mingw32/sys-root/mingw/lib/libqhullstatic.a
			#/usr/x86_64-w64-mingw32/sys-root/mingw/lib/libqhullstatic_r.a
			##/usr/x86_64-w64-mingw32/sys-root/mingw/lib/libqhullcpp.a
			Qt5::Core Qt5::Gui Qt5::Widgets
			Qt5::Svg
		)

		target_link_libraries(taspaths_lines
			"${Boost_LIBRARIES}"
			"${Lapacke_LIBRARIES}"
			"${OVD_LIBRARIES}"
			"${CGAL_LIBRARIES}"
			"${MINGW_WINSOCK}"
			Threads::Threads
			Qt5::Core Qt5::Gui Qt5::Widgets
			Qt5::Svg
		)

		target_link_libraries(taspaths_poly
			"${Boost_LIBRARIES}"
			"${Lapacke_LIBRARIES}"
			Qt5::Core Qt5::Gui Qt5::Widgets
			Qt5::Svg
		)
	endif()
endif()


# link resource directory
#add_custom_command(
#	TARGET taspaths PRE_BUILD
#	COMMAND if [ ! -d res ]\; then ln -sf ../res\; fi
#)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# unit tests
# -----------------------------------------------------------------------------
if(UNIT_TESTS)
	add_subdirectory(unittests)
endif()
# -----------------------------------------------------------------------------
