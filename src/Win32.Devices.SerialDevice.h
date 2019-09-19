/******************************************************************************
*	Serial Device class implmentation utilizing the win32 api
*
*	\file Win32.Devices.SerialDevice.h
*	\author Jensen Miller
*
*	Copyright (c) 2019 LooUQ Incorporated.
*
*	License: The GNU License
*
*	This file is part of CoreZero.
*
*   CoreZero is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   CoreZero is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with CoreZero.  If not, see <https://www.gnu.org/licenses/>.
*
******************************************************************************/
#ifndef WIN32_DEVICES_SERIALDEVICE_H_
#define WIN32_DEVICES_SERIALDEVICE_H_

#ifdef WIN32
#include <windows.h>
#endif // WIN32

#include <iostream>
#include <string>
#include <array>
#include <thread>
#include <atomic>

#include <CoreZero.Event.hpp>

namespace Win32
{
	namespace Devices
	{
		typedef enum
		{
			Byte_Size7b = 7,
			Byte_Size8b = 8
		} SerialByteSize;

		typedef enum
		{			
			StopBits_1 = ONESTOPBIT,
			StopBits_1_5 = ONE5STOPBITS,
			StopBits_2 = TWOSTOPBITS
		} SerialStopBits;

		using OnRxData = CoreZero::Delegate<void(std::string)>;

		struct SerialDevice	final
		{				
			SerialDevice(std::nullptr_t);
			SerialDevice(SerialDevice* serialDevicePtr);

			virtual ~SerialDevice();

			static SerialDevice* FromPortNumber(uint16_t COMPortNum);

			void Close();
			void UsingEvents(bool usingCommEv);

			template <typename T, unsigned N>
			void Write(const std::array<T, N>& src_ary);
			void Write(const std::string& src_str);

			template <typename T, unsigned N>
			void Read(std::array<T, N>& dest_ary);
			void Read(std::string& dest_str);

			uint32_t Available();

			void BaudRate(uint32_t baudrate);
			uint32_t BaudRate() const;

			void StopBits(uint8_t stopBits);
			uint8_t StopBits() const;

			void ByteSize(SerialByteSize byteSize);
			SerialByteSize ByteSize() const;

			CoreZero::Event<OnRxData> ReceivedData;

		private:
			SerialDevice(HANDLE pSercom, uint16_t comPortNum) : m_pComm(pSercom), m_portNum(comPortNum) {}

			void write(const void* _src, size_t len);
			size_t read(void* _dest, size_t len);
			void config_settings();
			void config_timeouts();
			void clear_comm();

			void interrupt_thread();

		private:
			///	Native handle for sercom.
			std::atomic<HANDLE> m_pComm = nullptr;

			///	COM port number.
			uint16_t m_portNum = (uint16_t)-1;

			///	The BaudRate.
			uint32_t m_baudrate = 9600U;

			/// The size of a byte.
			SerialByteSize m_byteSize = Byte_Size8b;

			/// Handle for a thread to await comm events
			std::thread* m_thCommEv = nullptr;
		};


		template<typename T, unsigned N>
		inline void SerialDevice::Write(const std::array<T, N>& src_ary)
		{
			write(src_ary.data, N);
		}


		template<typename T, unsigned N>
		inline void SerialDevice::Read(std::array<T, N>& dest_ary)
		{

		}
	}
}

#endif	// !WIN32_DEVICES_SERIALDEVICE_H_