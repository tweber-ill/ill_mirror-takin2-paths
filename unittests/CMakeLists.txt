#
# unit tests
# @author Tobias Weber <tweber@ill.fr>
# @date 24-apr-2021
# @license see 'LICENSE' file
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
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# run individual test cases
# -----------------------------------------------------------------------------
add_test(intersect_circle intersect_circle)
add_test(intersect_circle_poly intersect_circle_poly)
add_test(intersect_line intersect_line)
# -----------------------------------------------------------------------------
