//	Copyright (c) 2019 LooUQ Incorporated.

//	Licensed under the GNU GPLv3. See LICENSE file in the project root for full license information.
#include "../include/Win32.Devices.SerialDevice.h"

#include <assert.h>

#ifdef DEBUG
#define DEBUG_ASSERT(ptr) assert(ptr)
#else
#define DEBUG_ASSERT(ptr)
#endif

#define NO_SHARING	NULL
#define NO_SECURITY	NULL
#define NON_OVERLAPPED_IO	NULL
#define NO_FLAGS	NULL



namespace Win32
{
	namespace Devices
	{		
		/**********************************************************************
		 *	Blank Constructor.
		 */
		SerialDevice::SerialDevice(std::nullptr_t)
		{			
		}


		/**********************************************************************
		 *	Construct from a provided SerialDevice that has an initialized comm
		 *		handle.
		 *
		 *	\param[in] A pointer to an available serial device.
		 */
		SerialDevice::SerialDevice(SerialDevice&& serialDevicePtr) noexcept			
			: m_portNum(serialDevicePtr.m_portNum)
			, m_pComm(serialDevicePtr.m_pComm)
		{
			m_pComm = nullptr;
			assert(m_pComm);

			config_settings();
			config_timeouts();
			clear_comm();
		}

		SerialDevice& SerialDevice::operator=(SerialDevice&& to_move) noexcept
		{			
			m_pComm = to_move.m_pComm;
			assert(m_pComm);
			m_portNum = to_move.m_portNum;

			to_move.m_pComm = nullptr;

			config_settings();
			config_timeouts();
			clear_comm();
			return *this;
		}



		/**********************************************************************
		 *	Destruct and close the comm handle.
		 */
		SerialDevice::~SerialDevice()
		{
			Close();
		}



		/**********************************************************************
		 *	Obtain a serial device from a specified COM Port number.
		 *		 
		 *	\param[in] COMPortNum The number of the COM Port. i.e. "COM10"
		 *		requires an input of 10.
		 *	\returns A serial device with an initalized comm handle.
		 */
		SerialDevice SerialDevice::FromPortNumber(uint16_t COMPortNum)
		{
			std::wstring port_dir = { L"\\\\.\\COM" + std::to_wstring(COMPortNum) };

			HANDLE h_sercom	= CreateFile(
				port_dir.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				NO_SHARING,
				NO_SECURITY,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NO_FLAGS
			);
			

			if (h_sercom == INVALID_HANDLE_VALUE)
			{
				std::cerr << "Could not open port: COM" << COMPortNum << "!" << std::endl;
				
			}

			return SerialDevice(h_sercom, COMPortNum);
		}



		/**********************************************************************
		 *	Close the serial device connection.		 		 
		 */
		void SerialDevice::Close()
		{
			m_continuePoll.clear();
			if (m_thCommEv.joinable()) m_thCommEv.join();

			if (m_pComm != nullptr)
			{
				CloseHandle(m_pComm);
				m_pComm = nullptr;
			}				
		}


		/**********************************************************************
		 *	Tell the serial device to use a separate thread for awaiting events
		 *		from the comm.
		 *
		 *	\param[in] usingCommEv Indicates whether to use serial events.
		 */
		void SerialDevice::UsingEvents(bool usingCommEv)
		{
			m_continuePoll.test_and_set();
			m_thCommEv = std::thread(&SerialDevice::interrupt_thread, this);			
		}



		/**********************************************************************
		 *	Write a stl string to the serial device.
		 *
		 *	\param[in] src_string The string containing source data.
		 */
		void SerialDevice::Write(const std::string& src_str)
		{
			win32_write(src_str.c_str(), src_str.length());
		}



		/**********************************************************************
		 *	Read data from the serial device and put it into an stl string.
		 *
		 *	\param[out] dest_str The string designating the received data.
		 */
		void SerialDevice::Read(std::string& dest_str)
		{
			char temp[128];

			size_t len = win32_read(temp, 128);
			dest_str = { temp, len };
		}



		/**********************************************************************
		 *	Indicates the data received and available in the Rx buffer.
		 *
		 *	\returns The number of bytes avaialbe in the Rx buffer.
		 */
		uint32_t SerialDevice::Available()
		{
			DWORD err_flags = { 0 };
			COMSTAT com_status = { 0 };
			DEBUG_ASSERT(m_pComm);
			ClearCommError(m_pComm, &err_flags, &com_status);
				
			return com_status.cbInQue;
		}



		/**********************************************************************
		 *	Sets the baudrate.
		 *
		 *	\param[in] baudrate The baud rate of the communication.
		 */
		void SerialDevice::BaudRate(uint32_t baudrate)
		{
			m_baudrate = baudrate;
			config_settings();
		}



		/**********************************************************************
		 *	Gets the baudrate
		 *
		 *	\returns The current baud rate of the communication.
		 */
		uint32_t SerialDevice::BaudRate() const
		{
			return m_baudrate;
		}



		/**********************************************************************
		 *	Sets the stop bits.
		 *
		 *	\param[in]	stopBits The number of stop bits to use for data
		 *		flow control.
		 */
		void SerialDevice::StopBits(uint8_t stopBits)
		{
			// \TODO: m_stopBits = stopBits;
			config_settings();
		}



		/**********************************************************************
		 *	Gets the number of stop bits.
		 *
		 *	\returns The number of stop bits currently used for flow control.
		 */
		uint8_t SerialDevice::StopBits() const
		{
			// \TODO: return m_stopBits
			return uint8_t();
		}



		/**********************************************************************
		 *	Set the size of a byte.
		 *
		 *	\param[in] byteSize The size of bytes in the communication.
		 */
		void SerialDevice::ByteSize(SerialByteSize byteSize)
		{
			m_byteSize = byteSize;
			config_settings();
		}



		/**********************************************************************
		 *	Gets the size of a byte.
		 *
		 *	\returns The current size of bytes in the communication.
		 */
		SerialByteSize SerialDevice::ByteSize() const
		{
			return m_byteSize;
		}



		/**********************************************************************
		 *	Basic write function that calls to the win32 api for serial
		 *		writing.
		 *
		 *	\param[in] _src The source of the data to write to the serial
		 *		device.
		 *	\param[in] len The length of the source data.
		 */
		void SerialDevice::win32_write(const void* _src, size_t len)
		{
			DWORD written;
			assert(m_pComm);
			std::lock_guard<std::mutex> lck(m_critical);
			if (!WriteFile(m_pComm, _src, len, &written, NULL))
			{
				if (GetLastError() != ERROR_IO_PENDING)
				{
					//	[error]: write failed.
					std::cerr << "Serial Error: Unable to write all bytes!" << std::endl;
				}
				else
				{
					//	write operation was successful
				}
			}
		}



		/**********************************************************************
		 *	Basic read function that calls to the win32 api for serial reading.
		 *
		 *	\param[out] _dest The destination buffer for holding Rx data.
		 *	\param[in] len The capacity of the destination buffer.
		 *	\returns The number of bytes read into the destination buffer.
		 */
		size_t SerialDevice::win32_read(void* _dest, size_t len)
		{
			DWORD read_;
			assert(m_pComm);

			if (!m_thCommEv.joinable())
			{
				//	if there isn't an interrupt thread, pend for data here
				DWORD comm_ev = 0;

				if (!SetCommMask(m_pComm, EV_RXCHAR))
				{
					//	[error]: could not set event.
				}				

				//	not polling on separate thread
				if (!WaitCommEvent(m_pComm, &comm_ev, NULL))
				{
					return 0;
				}
				len = Available();
			}
			std::lock_guard<std::mutex> lck(m_critical);		

			if (!ReadFile(m_pComm, _dest, len, &read_, NULL))
			{
				std::cerr << "Serial Error: Unable to read all bytes!" << std::endl;
			}
			return read_;
		}



		/**********************************************************************
		 *	Configure the settings of the serial device using the win32 api.
		 */
		void SerialDevice::config_settings()
		{
			DCB data_cntrl_blk = { 0 };

			assert(m_pComm);
			std::lock_guard<std::mutex> lck(m_critical);

			if (!GetCommState(m_pComm, &data_cntrl_blk))
			{
				std::cerr << "Serial Error: Unable to retrieve port settings!" << std::endl;
			}
			else
			{
				/*	configure new settings */

				//	Binary Mode
				data_cntrl_blk.fBinary = TRUE;

				//	Enable parity checking
				data_cntrl_blk.fParity = TRUE;

				//	DSR sensitivity
				data_cntrl_blk.fDsrSensitivity = FALSE;

				//	Disable error replacement
				data_cntrl_blk.fErrorChar = FALSE;

				//	No DSR output flow control
				data_cntrl_blk.fOutxDsrFlow = FALSE;

				//	Do not abort reads/writes on error
				data_cntrl_blk.fAbortOnError = FALSE;

				//	Disable null stripping
				data_cntrl_blk.fNull = FALSE;

				//	XOFF continues TX
				data_cntrl_blk.fTXContinueOnXoff = TRUE;

				//	Set BAUD rate
				data_cntrl_blk.BaudRate = m_baudrate;

				//	Set byte size
				data_cntrl_blk.ByteSize = m_byteSize;

				//	Set stop bits
				data_cntrl_blk.StopBits = ONESTOPBIT;

				//	Even Parity
				data_cntrl_blk.Parity = NOPARITY;

				/* --------------------------------------------------------------------- */
				/*						Hardware flow control							 */

				// CTS output flow control
				data_cntrl_blk.fOutxCtsFlow = TRUE;

				// DTR flow control type
				data_cntrl_blk.fDtrControl = DTR_CONTROL_ENABLE;

				// No XON/XOFF out flow control
				data_cntrl_blk.fOutX = FALSE;

				// No XON/XOFF in flow control
				data_cntrl_blk.fInX = FALSE;

				// RTS flow control
				data_cntrl_blk.fRtsControl = RTS_CONTROL_ENABLE;

				if (!SetCommState(m_pComm, &data_cntrl_blk))
				{
					std::cerr << "Serial Error: Unable to apply port settings!" << std::endl;
				}
			}
		}



		/**********************************************************************
		 *	Configure the serial timeouts for read/write operations by calling
		 *		the win32 api.
		 */
		void SerialDevice::config_timeouts()
		{
			COMMTIMEOUTS timeouts;

			assert(m_pComm);
			std::lock_guard<std::mutex> lck(m_critical);
			//	Get default timeout settings
			if (!GetCommTimeouts(m_pComm, &timeouts))
			{
				std::cerr << "Serial Error: Unable to retrieve timeout settings" << std::endl;
			}			
			else
			{					
				/*	All values are in milliseconds	*/

				// Max time between arrival of two bytes
				timeouts.ReadIntervalTimeout = 50;

				// Total for read operation -> ReadFile()
				timeouts.ReadTotalTimeoutConstant = 50;

				// Used to calculate total period of read operation
				timeouts.ReadTotalTimeoutMultiplier = 10;


				/*	Write timeouts	*/

				// Total for write operation -> WriteFile()
				timeouts.WriteTotalTimeoutConstant = 50;

				// Total for write operation
				timeouts.WriteTotalTimeoutMultiplier = 10;

				//	Set port timeouts
				if (!SetCommTimeouts(m_pComm, &timeouts))
				{
					std::cerr << "Serial Error: Unable to apply new timeouts!" << std::endl;
				}
			}
		}



		/**********************************************************************
		 *	Clear the comm handle via a call to the win32 api.
		 */
		void SerialDevice::clear_comm()
		{
			assert(m_pComm);
			std::lock_guard<std::mutex> lck(m_critical);
			//	Clear the port
			if (PurgeComm(m_pComm, PURGE_TXCLEAR | PURGE_RXCLEAR) == 0)
			{
				std::cerr << "Serial Error: Unable to clear the port!" << std::endl;
			}
		}



		/**********************************************************************
		 *	The background thread that awaits events on the comm. Using the
		 *		win32 api, this thread sets the comm mask to await any received
		 *		character. Upon characters, the thread checks for how many,
		 *		reads the characters into a buffer, and then calls the CoreZero
		 *		event, thus calling a user-defined handler.
		 */
		void SerialDevice::interrupt_thread()
		{
			//	set the comm mask to await received character events
			if (!SetCommMask(m_pComm, EV_RXCHAR))
			{				
				std::cerr << "Serial Error: Unable to set comm mask!" << std::endl;
				abort();
			}
			else
			{
				DWORD comm_ev = 0;
				DWORD bytes_read = 0;
				while (m_continuePoll.test_and_set())
				{

					//	await a received character event (win32)
					if (WaitCommEvent(m_pComm, &comm_ev, NULL))
					{
						size_t len = 0;
						do
						{
							//	check for how many characters are available
							len = Available();
							if (len)
							{

								//	create a destination buffer for rx chars
								uint8_t* buf = new uint8_t[len];

								//	read data into buffer
								bytes_read = win32_read(buf, len);

								//	trigger event, passing an stl string containing data
								ReceivedData(std::string((char*)buf, len));

								// recycle the buffer
								delete[] buf;
							}					
						} while (len && bytes_read);
					}
					else
					{

					}						
				}
			}
		}		
	}
}

