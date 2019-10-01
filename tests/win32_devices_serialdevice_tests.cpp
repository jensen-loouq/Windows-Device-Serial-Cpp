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
		}


		TEST_METHOD(SendAndRecv)
		{
			SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };

			serial_device.BaudRate(TestBuadRate);
			serial_device.Write("ATE0\r");
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			std::string response;
			serial_device.Read(response);
			Assert::IsTrue(response.compare("0\r") == 0);	// compare returns 0 on true
		}

		volatile int signal = { 0 };

		void HandleRxData(std::string rx_data)
		{
			Logger::WriteMessage(rx_data.c_str());
			signal = 1;
		}


		TEST_METHOD(SendEvRx)
		{
			SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };
			serial_device.BaudRate(TestBuadRate);

			serial_device.UsingEvents(true);
			serial_device.ReceivedData += CoreZero::Create_MemberDelegate(
				this,
				&Test_SerialDevice::HandleRxData
			);

			serial_device.Write("ATE0\r");
			while (!signal)
				;
			Assert::IsTrue(signal);			

			signal = 0;
			serial_device.Write("ATV0\r");
			while (!signal)
				;
			Assert::IsTrue(signal);
		}


		TEST_METHOD(Send_Multiple)
		{
			SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };
			serial_device.BaudRate(TestBuadRate);

			serial_device.UsingEvents(true);
			serial_device.ReceivedData += CoreZero::Create_MemberDelegate(
				this,
				&Test_SerialDevice::HandleRxData
			);

			serial_device.Write("ATE0\r");
			serial_device.Write("ATV0\r");
			serial_device.Write("AT+GSN\r");
			serial_device.Write("ATI\r");
		}
	};
}
