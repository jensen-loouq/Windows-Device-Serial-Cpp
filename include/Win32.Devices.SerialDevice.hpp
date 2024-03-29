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

#include <corezero/event.hpp>

namespace Win32
{
	namespace Devices
	{
		/// The number of bits per byte.
		enum class SerialByteSize
		{
			Byte_Size7b = 7,	///< 7 bits per byte.
			Byte_Size8b = 8		///< 8 bits per byte.
		};


		/// The number of stop bits per transaction.
		enum class SerialStopBits
		{			
			StopBits_1 = ONESTOPBIT,		///< 1 stop bit.
			StopBits_1_5 = ONE5STOPBITS,	///< 1.5 stop bits.
			StopBits_2 = TWOSTOPBITS		///< 2 stop bits.
		};

		///	Handler signature for data in reciever.
		using OnRxData = corezero::Delegate<void(std::string)>;		



		///	A windows serial device.
		///	A modern c++ wrapper for the win32 api calls
		///		for serial communication.
		struct SerialDevice	final
		{				
			SerialDevice(std::nullptr_t);
			SerialDevice(SerialDevice&& to_move) noexcept;
			SerialDevice& operator=(SerialDevice&& to_move) noexcept;

			virtual ~SerialDevice();

			static SerialDevice FromPortNumber(uint16_t COMPortNum);



			void Close();
			void UsingEvents(bool usingCommEv);
			void Defer(std::chrono::milliseconds deferMillis);

			template <typename T, unsigned N>
			size_t Write(const std::array<T, N>& src_ary);
			size_t Write(const std::string& src_str);

			template <typename T, unsigned N>
			size_t Read(std::array<T, N>& dest_ary);
			size_t Read(std::string& dest_str);

			uint32_t Available();

			void BaudRate(uint32_t baudrate);
			uint32_t BaudRate() const;

			void StopBits(uint8_t stopBits);
			uint8_t StopBits() const;

			void ByteSize(SerialByteSize byteSize);
			SerialByteSize ByteSize() const;

			corezero::Event<OnRxData> ReceivedData;

		private:
			SerialDevice(HANDLE pSercom, uint16_t comPortNum) : m_pComm(pSercom), m_portNum(comPortNum) {}

			size_t win32_write(const void* _src, size_t len);
			size_t win32_read(void* _dest, size_t len, DWORD readTimeout = INFINITE);

			void config_settings();
			void config_timeouts();
			void clear_comm();

			void interrupt_thread();
			void handle_data();

		private:
			///	Native handle for sercom.
			HANDLE volatile m_pComm = nullptr;

			BOOL m_ReadOpPending = FALSE;

			///	COM port number.
			uint16_t m_portNum = (uint16_t)-1;

			///	The BaudRate.
			uint32_t m_baudrate = 9600U;

			/// The size of a byte.
			SerialByteSize m_byteSize = SerialByteSize::Byte_Size8b;

			/// Handle for a thread to await comm events
			std::thread m_thCommEv;

			///
			std::atomic_flag m_continuePoll = ATOMIC_FLAG_INIT;
		};


		template<typename T, unsigned N>
		inline size_t SerialDevice::Write(const std::array<T, N>& src_ary)
		{
			return win32_write(src_ary.data, N);
		}


		template<typename T, unsigned N>
		inline size_t SerialDevice::Read(std::array<T, N>& dest_ary)
		{
			return 0;
		}
	}
}

#endif	// !WIN32_DEVICES_SERIALDEVICE_H_