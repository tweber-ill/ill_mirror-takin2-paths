# TAS-Paths
Triple-axis path-finding tool.

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.4625649.svg)](https://doi.org/10.5281/zenodo.4625649)


## Building
- Install development versions of the following external libraries: [*Boost*](https://www.boost.org/), [*Qt*](https://www.qt.io/), and optionally [*Lapack(e)*](https://www.netlib.org/lapack/).
- Clone the source repository: `git clone https://code.ill.fr/scientific-software/takin/paths`.
- Go to the repository's root directory: `cd paths`.
- Get the *tlibs* library: `./setup/get_libs.sh`.
- Create a build directory in the repository's root directory: `mkdir build && cd build`.
- Use CMake from the build directory: `cmake -DCMAKE_BUILD_TYPE=Release ..`.
- Use Make from the build directory: `make -j4`.


## External Dependencies
============ =========================================== ==============================================================
Library      URL                                         License URL
============ =========================================== ==============================================================
Boost        http://www.boost.org                        http://www.boost.org/LICENSE_1_0.txt
CGAL         https://github.com/CGAL/cgal                https://github.com/CGAL/cgal/blob/master/Installation/LICENSE
QCustomPlot  https://www.qcustomplot.com                 https://gitlab.com/DerManu/QCustomPlot/-/raw/master/GPL.txt
Lapack(e)    https://www.netlib.org/lapack/lapacke.html  http://www.netlib.org/lapack/LICENSE.txt
QHull        http://www.qhull.org                        https://github.com/qhull/qhull/blob/master/COPYING.txt
============ =========================================== ==============================================================
