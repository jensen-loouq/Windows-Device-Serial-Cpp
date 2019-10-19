#include <gtest/gtest.h>
#include <Win32.Devices.SerialDevice.hpp>

using namespace Win32::Devices;
using namespace std::chrono_literals;

constexpr int TestPort = 22;
constexpr uint32_t TestBuadRate = CBR_115200;

namespace tests
{
	TEST(SerialDeviceTest, OpenSerialPort)
	{
		SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };

		serial_device.Close();
	}


	//TEST(SerialDeviceTest, SendAndRecv)
	//{
	//	SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };

	//	serial_device.BaudRate(TestBuadRate);
	//	size_t chars_written = serial_device.Write("ATE0\r");
	//	ASSERT_EQ(5u, chars_written);
	//		
	//	std::string response;
	//	serial_device.Read(response);
	//	//Logger::WriteMessage(response.c_str());
	//	ASSERT_TRUE(response.compare("0\r") == 0);	// compare returns 0 on true
	//}

	volatile int signal = { 0 };

	void HandleRxData(std::string rx_data)
	{
		//Logger::WriteMessage(rx_data.c_str());
		signal = 1;
	}


	TEST(SerialDeviceTest, SendEvRx)
	{
		SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };
		serial_device.BaudRate(TestBuadRate);

		serial_device.UsingEvents(true);
		serial_device.ReceivedData += HandleRxData;

		std::this_thread::sleep_for(1000ms);

		ASSERT_EQ(5u, serial_device.Write("ATE0\r"));
		while (!signal)
			;
		ASSERT_TRUE(signal);

		signal = 0;
		ASSERT_EQ(5u, serial_device.Write("ATV0\r"));
		while (!signal)
			;
		ASSERT_TRUE(signal);
	}


	TEST(SerialDeviceTest, Send_Multiple)
	{
		SerialDevice serial_device = { SerialDevice::FromPortNumber(TestPort) };
		serial_device.BaudRate(TestBuadRate);

		serial_device.ReceivedData += HandleRxData;
		serial_device.UsingEvents(true);

		ASSERT_EQ(5u, serial_device.Write("ATE0\r"));
		ASSERT_EQ(5u, serial_device.Write("ATV0\r"));
		ASSERT_EQ(7u, serial_device.Write("AT+GSN\r"));
		ASSERT_EQ(4u, serial_device.Write("ATI\r"));

		serial_device.Defer(1000ms);
	}
}
