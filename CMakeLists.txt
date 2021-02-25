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
	add_compile_options(-mmacosx-version-min=10.10)
endif()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# packages
find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# compiler settings
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(${Boost_CXX_FLAGS})
add_compile_options(-Wall -Wextra)

include_directories("${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../..")
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

target_link_libraries(paths ${Boost_LIBRARIES})
qt5_use_modules(paths Core Gui Widgets OpenGL)


# link resource directory
add_custom_command(
	TARGET paths PRE_BUILD
	COMMAND if [ ! -d res ]\; then ln -sf ../res\; fi
)

# -----------------------------------------------------------------------------
