#include "CppUnitTest.h"
#include <Win32.Devices.SerialDevice.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Win32::Devices;

constexpr int TestPort = 12;
constexpr uint32_t TestBuadRate = CBR_115200;

namespace tests
{
	TEST_CLASS(Test_SerialDevice)
	{
	public:
		
		TEST_METHOD(OpenSerialPort)
		{
			SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };
			//	If the constructor does not throw an exception (nullptr_t) then the test passes
		}


		TEST_METHOD(SendAndRecv)
		{
			SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };

			serial_device.BaudRate(TestBuadRate);
			serial_device.Write("ATE0\r");
			
			std::string response;
			serial_device.Read(response);
			Assert::IsTrue(response.compare("0") == 0);	// compare returns 0 on true
		}


		TEST_METHOD(SendEvRx)
		{
			volatile int signal = { 0 };
			auto HandleRxData = [&signal](std::string rx_data) -> void
			{
				Logger::WriteMessage(rx_data.c_str());
				signal = 1;
			};

			SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };
			serial_device.BaudRate(TestBuadRate);

			serial_device.UsingEvents(true);
			serial_device.ReceivedData += CoreZero::Create_MemberDelegate(
				HandleRxData,
				&decltype(HandleRxData)::operator()
			);

			serial_device.Write("ATE0\r");
			while (!signal)
				;
			Assert::IsTrue(signal);
		}
	};
}
