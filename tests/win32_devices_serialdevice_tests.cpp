#include "CppUnitTest.h"
#include <Win32.Devices.SerialDevice.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Win32::Devices;
using namespace std::chrono_literals;

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
			size_t chars_written = serial_device.Write("ATE0\r");
			Assert::AreEqual(5u, chars_written);
			
			std::string response;
			serial_device.Read(response);
			Logger::WriteMessage(response.c_str());
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

			std::this_thread::sleep_for(1000ms);

			Assert::AreEqual(5u, serial_device.Write("ATE0\r"));
			while (!signal)
				;
			Assert::IsTrue(signal);

			signal = 0;
			Assert::AreEqual(5u, serial_device.Write("ATV0\r"));
			while (!signal)
				;
			Assert::IsTrue(signal);
		}


		TEST_METHOD(Send_Multiple)
		{
			SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };
			serial_device.BaudRate(TestBuadRate);

			serial_device.ReceivedData += CoreZero::Create_MemberDelegate(
				this,
				&Test_SerialDevice::HandleRxData
			);
			serial_device.UsingEvents(true);

			Assert::AreEqual(5u, serial_device.Write("ATE0\r"));
			Assert::AreEqual(5u, serial_device.Write("ATV0\r"));
			Assert::AreEqual(7u, serial_device.Write("AT+GSN\r"));
			Assert::AreEqual(4u, serial_device.Write("ATI\r"));

			serial_device.Defer(1000ms);
		}
	};
}
