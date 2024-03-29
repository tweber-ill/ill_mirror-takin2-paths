#
# unit tests
# @author Tobias Weber <tweber@ill.fr>
# @date 24-apr-2021
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
cmake_policy(SET CMP0078 NEW)
cmake_policy(SET CMP0086 NEW)

project(paths_unittests)
enable_language(CXX)
enable_testing()

list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/../tlibs2/cmake"
)


option(USE_LAPACK "use lapack" TRUE)


message("Build type: ${CMAKE_BUILD_TYPE}")

if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(CMAKE_VERBOSE_MAKEFILE TRUE)
endif()


# -----------------------------------------------------------------------------
# compiler settings
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(${Boost_CXX_FLAGS})
add_compile_options(-Wall -Wextra)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# packages
# -----------------------------------------------------------------------------
# boost
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
find_package(Boost REQUIRED)
add_definitions(${Boost_CXX_FLAGS})

message("Using Boost version ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}.")


# lapack
if(USE_LAPACK)
        find_package(Lapacke)
        if(Lapacke_FOUND)
                message("Lapacke enabled.")
                add_definitions(-DUSE_LAPACK)
        else()
                message("Lapacke disabled.")
        endif()
endif()
# -----------------------------------------------------------------------------


include_directories(
	"${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/.."
	"${PROJECT_SOURCE_DIR}/externals" "${PROJECT_SOURCE_DIR}/../externals"
	"${Lapacke_INCLUDE_DIRS}"
)


# -----------------------------------------------------------------------------
# build individual test cases
# -----------------------------------------------------------------------------
add_executable(intersect_circle intersect_circle.cpp)
target_link_libraries(intersect_circle ${Lapacke_LIBRARIES})

add_executable(intersect_circle_poly intersect_circle_poly.cpp)
target_link_libraries(intersect_circle_poly ${Lapacke_LIBRARIES})

add_executable(intersect_line intersect_line.cpp)
target_link_libraries(intersect_line ${Lapacke_LIBRARIES})

add_executable(dijkstra dijkstra.cpp)
target_link_libraries(dijkstra ${Lapacke_LIBRARIES})

add_executable(index_trees index_trees.cpp)
target_link_libraries(index_trees ${Lapacke_LIBRARIES})

add_executable(voronoi voronoi.cpp)
target_link_libraries(voronoi ${Lapacke_LIBRARIES} -lgmp)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# run individual test cases
# -----------------------------------------------------------------------------
add_test(intersect_circle intersect_circle)
add_test(intersect_circle_poly intersect_circle_poly)
add_test(intersect_line intersect_line)
add_test(dijkstra dijkstra)
add_test(index_trees index_trees)
add_test(voronoi voronoi)
# -----------------------------------------------------------------------------
