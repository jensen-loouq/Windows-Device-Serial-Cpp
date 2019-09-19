![serial logo](https://static.thenounproject.com/png/74769-200.png)
# Windows-Desktop-Serial #
A modern C++ wrapper class for the Win32 API serial communications. The Win32 API for serial communication is documented around 1995, and slightly difficult to use (especially for those not familiar with C).
This wrapper class seeks to make the consumption of the Win32 Serial API easier to use, by porting it to C++11 language standards and with the support of the STL. This wrapper class pursues a design to force
the creation of serial devices during runtime.


## Getting Started


### Dependencies
* CoreZero
	* SDK Source Files
		* [Delegate](https://github.com/LooUQ/CoreZero-SDK/blob/master/src/CoreZero.Delegate.hpp)
		* [Event](https://github.com/LooUQ/CoreZero-SDK/blob/master/src/CoreZero.Event.hpp)


### Building
- [x] [Visual Studio](https://visualstudio.microsoft.com/vs/): You can include the source files in a Win32 Console project via shared code project or directly and build using MSVC.
- [ ] [GCC](https://gcc.gnu.org/): If you have [MinGW](http://mingw.org/), you can build using g++ with the provided makefile.


## Examples

### Basic Send (reactive - event handles receive)
```cpp
#include <iostream>
#include <Win32.Devices.SerialDevice.h>

using namespace Win32::Devices;


void HandleRxData(std::string);

int main()
{
	SerialDevice at_port = { SerialDevice::FromPortNumber(10) };
	at_port.UsingEvents(true);
	at_port.ReceivedData += HandleRxData;
	at_port.BaudRate(CBR_115200);

	//	turn off echo
	at_port.Write("ATE0\r");
	while (true)
		;
}

void HandleRxData(std::string in)
{
	std::cout << in << std::endl;
}
```

Which should turn echo off on an AT device, returning 0(numeric) OK(verbose[default]).
> OK

## Authors

* [Jensen Miller](https://github.com/jensen-loouq) - [LooUQ Incorporated](https://github.com/LooUQ)

## License

This project is licensed under the GNU GPLv3 License. See the [LICENSE](LICENSE) file for details.

## ToDo Tasks
- [ ] Read data into a stream object held by the serial device.
- [x] Thread protect comm handle.
- [ ] Add Examples / Use cases.
- [ ] Emulate stl iostream.
- [ ] Fix synchronous send/recv.