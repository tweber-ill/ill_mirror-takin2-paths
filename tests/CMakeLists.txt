#
# @author Tobias Weber <tweber@ill.fr>
# @date 15-aug-2020
# @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
# @license see 'LICENSE' file
#

cmake_minimum_required(VERSION 3.0)

project(taspath_tests)
enable_language(CXX)


option(USE_QT6 "use qt 6" FALSE)

if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(CMAKE_VERBOSE_MAKEFILE TRUE)
endif()

if(USE_QT6)
	set(QT_VER 6)
else()
	set(QT_VER 5)
endif()

message("Build type: ${CMAKE_BUILD_TYPE}")
message("Selected Qt version ${QT_VER}.")



# -----------------------------------------------------------------------------
# packages
# -----------------------------------------------------------------------------
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

if(QT_VER EQUAL 6)
	find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg)
elseif(QT_VER EQUAL 5)
	find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg)
else()
	message(FATAL_ERROR "Unknown Qt version selected: ${QT_VER}")
endif()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# compiler settings
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(${Boost_CXX_FLAGS})
add_compile_options(-Wall -Wextra)

add_definitions(${Boost_CXX_FLAGS})
# -----------------------------------------------------------------------------


if(QT_VER EQUAL 6)
	include_directories(
		${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src
		${PROJECT_SOURCE_DIR}/.. ${PROJECT_SOURCE_DIR}/../src
		${Boost_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}/..
		${Qt6Svg_INCLUDE_DIRS})
elseif(QT_VER EQUAL 5)
	include_directories(
		${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src
		${PROJECT_SOURCE_DIR}/.. ${PROJECT_SOURCE_DIR}/../src
		${Boost_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}/..
		${Qt5Svg_INCLUDE_DIRS})
endif()


add_executable(hull
	hull.cpp hull.h
	../src/libs/hull.h
)

add_executable(lines
	lines.cpp lines.h
	../src/libs/lines.h
)


if(QT_VER EQUAL 6)
	target_link_libraries(hull
		Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
		${Boost_LIBRARIES}
		-lqhull_r -lqhullcpp
	)

	target_link_libraries(lines
		Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Svg
		${Boost_LIBRARIES}
		Threads::Threads
	)
elseif(QT_VER EQUAL 5)
	target_link_libraries(hull
		Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg
		${Boost_LIBRARIES}
		-lqhull_r -lqhullcpp
	)

	target_link_libraries(lines
		Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg
		${Boost_LIBRARIES}
		Threads::Threads
	)
endif()
