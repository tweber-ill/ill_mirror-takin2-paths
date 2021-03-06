#
# @author Tobias Weber <tweber@ill.fr>
# @date feb-2021
# @license GPLv3, see 'LICENSE' file
#

cmake_minimum_required(VERSION 3.0)
project(paths)
enable_language(CXX)

option(USE_QT6 "use qt 6" FALSE)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(GL_MAJOR_VER 4)
set(GL_MINOR_VER 1)

if(USE_QT6)
	set(QT_VER 6)
else()
	set(QT_VER 5)
endif()

message("Selected Qt version ${QT_VER}.")
message("Selected GL version ${GL_MAJOR_VER}.${GL_MINOR_VER}.")

# -----------------------------------------------------------------------------
# system specific settings
message("Building for ${CMAKE_SYSTEM_NAME} systems.")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# pass linker --subsystem option
	add_compile_options(-Wl,--subsystem,windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_compile_options(-mmacosx-version-min=10.15)
endif()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# packages
find_package(Boost REQUIRED)

if(QT_VER EQUAL 6)
	find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)
elseif(QT_VER EQUAL 5)
	find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
else()
	message(FATAL_ERROR "Unknown Qt version selected: ${QT_VER}")
endif()

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# compiler settings
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(${Boost_CXX_FLAGS})
add_compile_options(-Wall -Wextra)

add_definitions(-D_GL_MAJ_VER=${GL_MAJOR_VER} -D_GL_MIN_VER=${GL_MINOR_VER})
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# target executable settings
add_executable(paths
	src/PathsTool.cpp src/globals.h
	src/PathsRenderer.cpp src/PathsRenderer.h
	src/Instrument.cpp src/Instrument.h
	tlibs2/libs/glplot.cpp tlibs2/libs/glplot.h
)

#if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#	target_link_options(paths
#		# create an __info_plist section in the binary
#		PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_SOURCE_DIR}/../../../core/setup_mac/InfoBundle.plist
#	)
#endif()

if(QT_VER EQUAL 6)
	target_include_directories(paths PUBLIC
		"${PROJECT_SOURCE_DIR}"
		"${Boost_INCLUDE_DIRS}/.." "${Qt6Core_INCLUDE_DIRS}/.."
	)

	target_link_libraries(paths
		${Boost_LIBRARIES}
		Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets
	)
elseif(QT_VER EQUAL 5)
	target_include_directories(paths PUBLIC
		"${PROJECT_SOURCE_DIR}"
		"${Boost_INCLUDE_DIRS}/.." "${Qt5Core_INCLUDE_DIRS}/.."
	)

	target_link_libraries(paths
		${Boost_LIBRARIES}
		Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL
	)
endif()

# link resource directory
add_custom_command(
	TARGET paths PRE_BUILD
	COMMAND if [ ! -d res ]\; then ln -sf ../res\; fi
)

# -----------------------------------------------------------------------------
