
# Building on unix systems
Use the following commands on passcave directory :
- `mkdir build; cd build; cmake -DCMAKE_BUILD_TYPE=Release ../src && make; cd ..`

If you want smaller binaries then use `-DCMAKE_BUILD_TYPE=MinSizeRel` instead of `-DCMAKE_BUILD_TYPE=Release`.

# Building for windows
`mkdir build; cd build`
`cmake -DCMAKE_PREFIX_PATH="C:\Qt\Tools\mingw730_64" -DCMAKE_BUILD_TYPE=Release ../src`

set CMAKE_PREFIX_PATH=C:\\Qt\\Qt5.0.1\\5.0.1\\msvc2010\\
set CMAKE_PREFIX_PATH=C:\Qt\Tools\mingw730_64

# Dependencies
- On Ubuntu : build-essential, cmake, libgcrypt20-dev, qt5-default, qttools5-dev

# Packaging
You first need to build the package by following previous instructions.
Then use one of the following commands inside the build directory:
- Mac OSX Bundle: `cpack -G Bundle`
- Debian package: `cpack -G DEB`
- Windows installer: `cpack -G NSIS`
- Windows 64 installer: `cpack -G NSIS64`
- Red Hat / Fedora package: `cpack -G RPM`
- Arch linux package: `makepkg`
The resulting package is generated in the same directory.

# Known Issues
- Sorting may not be accurate when it involves non-ASCII chars;
- You cannot add / remove properties;
- Not tested on some platforms;
- MINOR BUG: if you modify something from the interface (double clicking and element), then the save button is not enabled immediately (however the document is marked as modified)

# License
Passcave' code is published under the GNU General Public License, version 3.

# Authors
Passcave is developed and maintained by Samy SADI.
Other contributors from the community are also welcome. 

# Contributing
The application is still under active development.
Our next development tasks include:
- Refactoring the code: Some of the code was written (very) long time ago, and needs to be refactored to respect modern C++ guidelines.
- Write a command line interface to manipulate the passwords.
- Write unitary tests.
- Write a browser extension and interface it with the passcave service.

We are, of course, accepting pull requests. Just make sure to respect the next guidelines:
- Respect the code style.
- The simpler diffs the better. For instance, make sure the code was not reformatted.
- Create separate PR for code reformatting.
- Make sure your changes does not break anything in existing code. In particular, make sure the existing junit tests can complete successfully.

