#
# @author Tobias Weber <tweber@ill.fr>
# @date feb-2021
# @license GPLv3, see 'LICENSE' file
#

cmake_minimum_required(VERSION 3.0)
project(paths)

set(CMAKE_VERBOSE_MAKEFILE TRUE)


# -----------------------------------------------------------------------------
# system specific settings
message("Building for ${CMAKE_SYSTEM_NAME} systems.")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# pass linker --subsystem option
	add_compile_options(-Wl,--subsystem,windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	#add_compile_options(-mmacosx-version-min=10.15)
endif()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# packages
find_package(Boost REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets)

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

message("Qt Core include dirs: ${Qt6Core_INCLUDE_DIRS}")
message("Qt Gui include dirs: ${Qt6Gui_INCLUDE_DIRS}")
message("Qt Widgets include dirs: ${Qt6Widgets_INCLUDE_DIRS}")
message("Qt OpenGL include dirs: ${Qt6OpenGL_INCLUDE_DIRS}")
message("Qt OpenGLWidgets include dirs: ${Qt6OpenGLWidgets_INCLUDE_DIRS}")
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# compiler settings
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(${Boost_CXX_FLAGS})
add_compile_options(-Wall -Wextra)

add_definitions(-D_GL_MAJ_VER=3 -D_GL_MIN_VER=3)

include_directories("${PROJECT_SOURCE_DIR}" 
	"${Boost_INCLUDE_DIRS}/.." 
	"${Qt6Core_INCLUDE_DIRS}" "${Qt6Gui_INCLUDE_DIRS}"
	"${Qt6Widgets_INCLUDE_DIRS}" "${Qt6OpenGL_INCLUDE_DIRS}"
	"${Qt6OpenGLWidgets_INCLUDE_DIRS}"
	"../..")
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# linker settings
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

target_link_libraries(paths ${Boost_LIBRARIES} Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)

# link resource directory
add_custom_command(
	TARGET paths PRE_BUILD
	COMMAND if [ ! -d res ]\; then ln -sf ../res\; fi
)

# -----------------------------------------------------------------------------
