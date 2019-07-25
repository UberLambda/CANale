# CANale
The firmware uploader for [CANnuccia](https://github.com/UberLambda/CANnuccia),
used for for in-field programming of MCUs via CAN bus.  

CANale needs an interface to send CAN commands to CANnuccia devices; see [here](https://doc.qt.io/qt-5/qtcanbus-backends.html#can-bus-plugins) for a list of supported backends.

## Platforms
The project should compile and run on Windows, MacOS and Linux. Tested on:

Platform | Architecture | Compiler
:-:|:-:|:-:|:-:
Manjaro Linux | x86_64 | Clang 8.0
Windows 10 | x86_64 | MSVC 15 + Clang

## Command-line usage
`canale -b <backend> -i <interface> <cmd1> <cmd2>... <cmdn>`  
where `<backend>` is one of the supported Qt Can Bus plugins and `<interface>` is the CAN interface to use.

Commands are run sequentially, in the same order as they are specified in, and include:

Syntax | Effect
|-|-|
`start+<dev1>,<dev2>...,<devn>` | Stops CANnuccia from timing out on the target devices and unlocks their flash memory for writing.
`flash+<dev>+<elfpath>` | Flashes the ELF file at `<elfpath>` to the device with the given id.
`stop+<dev1>,<dev2>...,<devn>` | Locks flash memory on the target devices, terminating CANnuccia and making them jump to the flashed program.

Device ids can be specified in decimal, hex (`0xNN`), octal (`0oNN`) or binary (`0bNN`).

#### Usage example
`canale -b socketcan -i can0 start+0xAA,0xBB flash+0xAA+prog1.elf flash+0xBB+prog2.elf stop+0xAA,0xBB`
will:
1. Connect to `can0` using SocketCAN
2. Unlock the devices with id 0xAA, then 0xBB for writing
3. Flash prog1.elf to device 0xAA
4. Flash prog2.elf to device 0xBB
5. Lock devices 0xAA, then 0xBB and make them start the flashed program

in this order.

## Building prerequisites
Required, must be installed manually:
- [CMake 3.14+](https://cmake.org/)
- [Qt 5](https://qt.io/download) (tested on Qt 5.12+, needs the [Qt Serial Bus](https://doc.qt.io/qt-5/qtserialbus-index.html) module)

Required, automatically downloaded as git submodules (`git clone --recursive`):
- [ELFIO](https://github.com/serge1/ELFIO)
- [CANnuccia](https://github.com/UberLambda/CANnuccia)

Optional:
- [Python 3.7+](https://python.org) (for the scripts in [tools/](tools/))

## Building
Create a build directory and [generate build files via CMake](https://cmake.org/runningcmake/), then compile the project. Make sure the required dependencies can be found by CMake.

## Public API
CANale is comprised of a core library, libcanale, and frontends (just canale-cli for now).  
libcanale exposes all of CANale's functionality via two APIs:
- A C++/Qt high-level API; see [src/canale.hh](src/canale.hh).
- A C wrapper over the C++ API; see [include/canale.h](include/canale.h).

## License
CANale is licensed under the [Mozilla Public License, Version 2](LICENSE).  
Third-party dependencies are distributed under their respective licenses;
see [src/3rdparty/CMakeLists.txt](src/3rdparty/CMakeLists.txt) for more details.  
Qt is available under the [GNU Lesser General Public License version 3](http://www.gnu.org/licenses/lgpl-3.0.html#).
