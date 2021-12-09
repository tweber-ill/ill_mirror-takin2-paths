# TAS-Paths
Pathfinding software for triple-axis spectrometers.

[![DOI](https://zenodo.org/badge/DOI/10.5281/zenodo.4625649.svg)](https://doi.org/10.5281/zenodo.4625649)


## Online Resources

### Website
The software's website can be found under [www.ill.eu/tas-paths](http://www.ill.eu/tas-paths).

### Demonstration Videos
Basic tutorial videos are available here:
- [Simplified pathfinding workflow in version 1.3.3](https://youtu.be/xs2BLuppQPQ).
- [Basic pathfinding workflow](https://youtu.be/iFnPrxYqipk).
- [Visualising the instrument's angular configuration space](https://youtu.be/WPUCVzMDKDc).


## Pathfinding
Steps to try out the pathfinding functionality:
- Move existing or add new walls or obstacles to the scene.
- Open the configuration space dialog using the "Calculation" -> "Angular Configuration Space..." menu item.
- Click the "Calculate Mesh" button in the configuration space dialog to compute the roadmap corresponding to the current instrument and wall configuration.
- Click the "Move Instrument Position" and "Move Target Position" radio buttons and click in the configuration space plot to set and move the start and target positions, respectively. 
- A path from the start to the target position is calculated. It can be directly traced by clicking the "Go" button in the main window's "Path Properties" dock window.


## Building
- Install development versions of at least the following external libraries: [*Boost*](https://www.boost.org/), [*Qt*](https://www.qt.io/), and optionally [*Lapack(e)*](https://www.netlib.org/lapack/).
- Clone the source repository: `git clone https://code.ill.fr/scientific-software/takin/paths`.
- Go to the repository's root directory: `cd paths`.
- Get the external dependencies: `./setup/get_libs.sh`.
- Get the external licenses (for a release package): `./setup/get_3rdparty_licenses.sh`.
- Build the binaries using: `./setup/release_build.sh`.
- Optionally create a package using either `./setup/deb/mk.sh`, `./setup/osx/mk.sh`, or `./setup/mingw/mk.sh`, depending on the system.
- The application can be started via `./build/taspaths`.


## External Dependencies
|Library     |URL                                        |License URL                                                          |
|------------|-------------------------------------------|---------------------------------------------------------------------|
|Boost       |http://www.boost.org                       |http://www.boost.org/LICENSE_1_0.txt                                 |
|CGAL        |https://www.cgal.org                       |https://github.com/CGAL/cgal/blob/master/Installation/LICENSE        |
|Qt          |https://www.qt.io                          |https://github.com/qt/qt5/blob/dev/LICENSE.QT-LICENSE-AGREEMENT      |
|QCustomPlot |https://www.qcustomplot.com                |https://gitlab.com/DerManu/QCustomPlot/-/raw/master/GPL.txt          |
|Lapack(e)   |https://www.netlib.org/lapack/lapacke.html |http://www.netlib.org/lapack/LICENSE.txt                             |
|QHull       |http://www.qhull.org                       |https://github.com/qhull/qhull/blob/master/COPYING.txt               |
|SWIG        |http://www.swig.org                        |https://github.com/swig/swig/blob/master/LICENSE                     |
|Python      |https://www.python.org                     |https://github.com/python/cpython/blob/main/Doc/license.rst          |
|Numpy       |https://numpy.org                          |https://github.com/numpy/numpy/blob/main/LICENSE.txt                 |
|Scipy       |https://www.scipy.org                      |https://github.com/scipy/scipy/blob/master/LICENSE.txt               |
|Matplotlib  |https://matplotlib.org                     |https://github.com/matplotlib/matplotlib/blob/master/LICENSE/LICENSE |
