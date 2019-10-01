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
			if (&to_move != this)
			{
				std::exchange(m_pComm, to_move.m_pComm);
				std::exchange(m_portNum, to_move.m_portNum);
				assert(m_pComm);

				config_settings();
				config_timeouts();
				clear_comm();
			}

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
				FILE_FLAG_OVERLAPPED,
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

			size_t len = win32_read_async(temp, 128);
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
			WriteFile(m_pComm, _src, len, &written, NULL);
			if (written <= 0)
			{
				std::cerr << "Serial Error: Unable to write all bytes!" << std::endl;
			}
			else
			{
				//if (SetCommMask(m_pComm, EV_RXCHAR) == FALSE)
				//{
				//	std::cerr << "Serial Error: Unable to set comm mask!" << std::endl;
				//}
			}
		}

		LPOVERLAPPED_COMPLETION_ROUTINE read_completion_routine;




		/**********************************************************************
		 *	Basic read function that calls to the win32 api for serial reading.
		 *
		 *	\param[out] _dest The destination buffer for holding Rx data.
		 *	\param[in] len The capacity of the destination buffer.
		 *	\returns The number of bytes read into the destination buffer.
		 */
		size_t SerialDevice::win32_read(void* _dest, size_t len)
		{
			DWORD _read;
			BOOL waiting_on_read = FALSE;

			m_OSReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

			assert(m_OSReader.hEvent == NULL);
			if (waiting_on_read)
			{
				if (!ReadFile(m_pComm, _dest, len, &_read, &m_OSReader))
				{
					if (GetLastError() != ERROR_IO_PENDING)
					{
						//	[error]: read error.
						return;
					}
					else
					{
						waiting_on_read = TRUE;
					}
				}
				else
				{
					return _read;
				}
			}
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
			if (!SetCommMask(m_pComm, EV_RXCHAR))
			{
				//	[error]: could not set mask
				abort();
			}
			else
			{
				m_SerialStatus.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
				assert(m_SerialStatus.hEvent);
				
				DWORD comm_ev;
				BOOL waiting_for_status = FALSE;
				while (m_continuePoll.test_and_set())
				{
					if (!waiting_for_status)
					{
						//	issue a status check
						if (!WaitCommEvent(m_pComm, &comm_ev, &m_SerialStatus))
						{
							if (GetLastError() == ERROR_IO_PENDING)
							{

							}
							else
							{
								throw std::exception("ERROR: IO Pending Issue");
								break;
							}
						}
						else
						{
							//	Handle EV_RXCHAR

							waiting_for_status = FALSE;
						}
					}

					if (waiting_for_status)
					{
						DWORD num_of_bytes_transferred = 0;
						DWORD _object = WaitForSingleObject(m_SerialStatus.hEvent, 500);
						switch (_object)
						{
						case WAIT_OBJECT_0:
							if (!GetOverlappedResult(m_pComm, &m_SerialStatus, &num_of_bytes_transferred, FALSE))
							{
								throw std::exception("ERROR: overlapped operation issue.");
							}
							else
							{
								//	Handle EV_RXCHAR

							}
							waiting_for_status = FALSE;
							break;

						case WAIT_TIMEOUT:
							//	do nothing
							break;

						default:
							//	[error]: in waitforsingleobject call
							abort();
						}
					}
				}
			}
		}		
	}
}

