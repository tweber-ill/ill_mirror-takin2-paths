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
