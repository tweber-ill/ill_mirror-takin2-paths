#
# @author Tobias Weber <tweber@ill.fr>
# @date feb-2021
# @license GPLv3, see 'LICENSE' file
#

cmake_minimum_required(VERSION 3.0)

project(taspaths)
enable_language(CXX)


list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/tlibs2/cmake"
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
option(BUILD_TEST_TOOLS "build test tools" FALSE)


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


message("Build type: ${CMAKE_BUILD_TYPE}")
message("Selected Qt version ${QT_VER}.")
message("Selected GL version ${GL_MAJOR_VER}.${GL_MINOR_VER}.")


# -----------------------------------------------------------------------------
# system specific settings
# -----------------------------------------------------------------------------
message("Building for ${CMAKE_SYSTEM_NAME} systems.")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# pass linker --subsystem option
	add_compile_options(-Wl,--subsystem,windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_compile_options(-mmacosx-version-min=10.14)
endif()

include_directories("${PROJECT_SOURCE_DIR}")
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# packages
# -----------------------------------------------------------------------------
find_package(Threads REQUIRED)

set(Boost_NO_BOOST_CMAKE FALSE)
find_package(Boost REQUIRED COMPONENTS filesystem)
message("Using Boost version ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}.")
include_directories("${Boost_INCLUDE_DIRS}/..")


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


if(QT_VER EQUAL 6)
	find_package(Qt6 REQUIRED COMPONENTS Core Gui Svg Widgets OpenGL OpenGLWidgets)
elseif(QT_VER EQUAL 5)
	find_package(Qt5 REQUIRED COMPONENTS Core Gui Svg Widgets OpenGL)
else()
	message(FATAL_ERROR "Unknown Qt version selected: ${QT_VER}")
endif()

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# compiler settings
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(${Boost_CXX_FLAGS})
add_compile_options(-Wall -Wextra)

add_definitions(-D_GL_MAJ_VER=${GL_MAJOR_VER} -D_GL_MIN_VER=${GL_MINOR_VER})
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# target executable settings
# -----------------------------------------------------------------------------
add_executable(taspaths
	src/gui/PathsTool.cpp src/gui/PathsTool.h
	src/gui/Settings.cpp src/gui/Settings.h
	src/gui/About.cpp src/gui/About.h
	src/gui/PathsRenderer.cpp src/gui/PathsRenderer.h
	src/gui/TASProperties.cpp src/gui/TASProperties.h
	src/gui/XtalProperties.cpp src/gui/XtalProperties.h
	src/gui/PathProperties.cpp src/gui/PathProperties.h
	src/gui/CamProperties.cpp src/gui/CamProperties.h

	src/core/Geometry.cpp src/core/Geometry.h
	src/core/Instrument.cpp src/core/Instrument.h
	src/core/types.h

	tlibs2/libs/qt/gl.cpp tlibs2/libs/qt/gl.h
)


if(BUILD_TEST_TOOLS)
	add_executable(hull
		src/tests/hull.cpp src/tests/hull.h
		src/libs/hull.h
	)

	add_executable(lines
		src/tests/lines.cpp src/tests/lines.h
		src/libs/lines.h
	)
endif()


#if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#	target_link_options(taspaths
#		# create an __info_plist section in the binary
#		PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_SOURCE_DIR}/../../../core/setup_mac/InfoBundle.plist
#	)
#endif()

if(QT_VER EQUAL 6)
	include_directories("${Qt6Core_INCLUDE_DIRS}/..")

	target_link_libraries(taspaths
		"${Boost_LIBRARIES}"
		"${Lapacke_LIBRARIES}"
		Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets
	)


	if(BUILD_TEST_TOOLS)
		target_link_libraries(hull
			Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
			${Boost_LIBRARIES}
			${Lapacke_LIBRARIES}
			-lqhull_r -lqhullcpp
		)

		target_link_libraries(lines
			Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
			${Boost_LIBRARIES}
			${Lapacke_LIBRARIES}
			Threads::Threads
		)
	endif()

elseif(QT_VER EQUAL 5)
	include_directories("${Qt5Core_INCLUDE_DIRS}/..")

	target_link_libraries(taspaths
		"${Boost_LIBRARIES}"
		"${Lapacke_LIBRARIES}"
		Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL
	)


	if(BUILD_TEST_TOOLS)
		target_link_libraries(hull
			Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg
			${Boost_LIBRARIES}
			${Lapacke_LIBRARIES}
			-lqhull_r -lqhullcpp
		)

		target_link_libraries(lines
			Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg
			${Boost_LIBRARIES}
			${Lapacke_LIBRARIES}
			Threads::Threads
		)
	endif()
endif()


# link resource directory
#add_custom_command(
#	TARGET taspaths PRE_BUILD
#	COMMAND if [ ! -d res ]\; then ln -sf ../res\; fi
#)

# -----------------------------------------------------------------------------
