#
# @author Tobias Weber <tweber@ill.fr>
# @date 15-aug-2020
# @note Forked on 19-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
# @license see 'LICENSE' file
#

cmake_minimum_required(VERSION 3.0)

project(tests)
enable_language(CXX)


find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Svg REQUIRED)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)


set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(-Wall -Wextra)


add_definitions(${Boost_CXX_FLAGS})

include_directories(
	${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src
	${PROJECT_SOURCE_DIR}/.. ${PROJECT_SOURCE_DIR}/../src
	${Boost_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS}/..
	${Qt5Svg_INCLUDE_DIRS}
)


add_executable(hull
	hull.cpp hull.h
	../src/libs/hull.h
)

target_link_libraries(hull
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg
	${Boost_LIBRARIES}
	-lqhull_r -lqhullcpp
)


add_executable(lines
	lines.cpp lines.h
	../src/libs/lines.h
)

target_link_libraries(lines
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg
	${Boost_LIBRARIES}
	Threads::Threads
)
