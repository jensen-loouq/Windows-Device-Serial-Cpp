//	Copyright (c) 2019 LooUQ Incorporated.

//	Licensed under the GNU GPLv3. See LICENSE file in the project root for full license information.
#include "Win32.Devices.SerialDevice.hpp"

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

#define SW_BUFFER_SIZE		(0xFFul)



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
			serialDevicePtr.m_pComm = nullptr;
			serialDevicePtr.m_portNum = 0;
			assert(m_pComm);

			config_settings();
			config_timeouts();
			clear_comm();
		}



		/**********************************************************************
		 *	Move assignment, for runtime purposes.
		 *
		 *	\paramp[in] to_move The SerialDevice instantiated at runtime.
		 */
		SerialDevice& SerialDevice::operator=(SerialDevice&& to_move) noexcept
		{	
			if (&to_move != this)
			{
				m_portNum = to_move.m_portNum;
				to_move.m_portNum = 0;

				m_pComm = to_move.m_pComm;
				to_move.m_pComm = nullptr;
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
			//	prepend COM directory
			std::wstring port_dir = { L"\\\\.\\COM" + std::to_wstring(COMPortNum) };

			//	create the handle to the COM port
			HANDLE h_sercom	= CreateFile(
				port_dir.c_str(),
				GENERIC_READ | GENERIC_WRITE,	// Open for reading and writing
				NO_SHARING,						// No Sharing of the COM port
				NO_SECURITY,					// No security necessary on COM port
				OPEN_EXISTING,					// Open an existing port
				FILE_FLAG_OVERLAPPED,			// Use overlapped operations
				NO_FLAGS						// No other flags
			);
			

			if (h_sercom == INVALID_HANDLE_VALUE)
			{
				std::cerr << "Could not open port: COM" << COMPortNum << "!" << std::endl;			
				throw std::exception("No COM HANDLE");
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
		 *	Defer operations to allow the working thread to process any pending
		 *		data coming in. Useful only when $UsingEvents.
		 *
		 *	\param[in] deferMillis The amount of milliseconds to defer the
		 *		ending of the worker thread.
		 */
		void SerialDevice::Defer(std::chrono::milliseconds deferMillis)
		{
			std::this_thread::sleep_for(deferMillis);
			m_continuePoll.clear();
		}



		/**********************************************************************
		 *	Write a stl string to the serial device.
		 *
		 *	\param[in] src_string The string containing source data.
		 */
		size_t SerialDevice::Write(const std::string& src_str)
		{
			return win32_write(src_str.c_str(), src_str.length());
		}



		/**********************************************************************
		 *	Read data from the serial device and put it into an stl string.
		 *
		 *	\param[out] dest_str The string designating the received data.
		 */
		size_t SerialDevice::Read(std::string& dest_str)
		{
			char temp[128];

			size_t len = win32_read(temp, 128);
			dest_str = { temp, len };
			return len;
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
		size_t SerialDevice::win32_write(const void* _src, size_t len)
		{
			OVERLAPPED os_writer = { 0 };
			DWORD bytes_written = 0;
			
			os_writer.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			assert(os_writer.hEvent != NULL);

			if (!WriteFile(m_pComm, _src, len, &bytes_written, &os_writer))
			{
				if (GetLastError() != ERROR_IO_PENDING)
				{
					//	[error]: write operation has failed
					return 0;
				}
				else
				{
					//	a write operation has been issued
					if (!GetOverlappedResult(m_pComm, &os_writer, &bytes_written, TRUE))
					{
						//	[error]: write operation has failed
						return 0;
					}
					else
					{	//	immediate
						//	the write operation has completed successfully
						return bytes_written;
					}
				}
			}
			else
			{
				//	immediate success
				//	the write operation has completed successfully
				return bytes_written;
			}
		}



		/**********************************************************************
		 *	Basic read function that calls to the win32 api for serial reading.
		 *
		 *	\param[out] _dest The destination buffer for holding Rx data.
		 *	\param[in] len The capacity of the destination buffer.
		 *	\returns The number of bytes read into the destination buffer.
		 */
		size_t SerialDevice::win32_read(void* _dest, size_t len, DWORD readTimeout)
		{
			OVERLAPPED os_reader = { 0 };
			DWORD bytes_read = 0;

			os_reader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			assert(os_reader.hEvent != NULL);

			if (!m_ReadOpPending)
			{
				if (!ReadFile(m_pComm, _dest, len, &bytes_read, &os_reader))
				{
					if (GetLastError() != ERROR_IO_PENDING)
					{
						//	[error]: could not issue read operation
						return 0;
					}
					else
					{
						//	read operation issued
						m_ReadOpPending = TRUE;
					}
				}
				else
				{
					//	read operation finished
					m_ReadOpPending = FALSE;
					return bytes_read;
				}
			}


			DWORD object_result;

			if (m_ReadOpPending)
			{
				object_result = WaitForSingleObject(os_reader.hEvent, readTimeout);
				switch (object_result)
				{
				case WAIT_OBJECT_0:
					if (!GetOverlappedResult(m_pComm, &os_reader, &bytes_read, FALSE))
					{
						//	[error]: in communications
						return 0;
					}
					else
					{						
						//	read operation finished
						m_ReadOpPending = FALSE;
						return bytes_read;
					}
					break;

				case WAIT_TIMEOUT:
					break;

				default:
					break;
				}
			}
			return 0;
		}



		/**********************************************************************
		 *	Configure the settings of the serial device using the win32 api.
		 */
		void SerialDevice::config_settings()
		{
			DCB data_cntrl_blk = { 0 };

			assert(m_pComm);			

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
				data_cntrl_blk.ByteSize = (BYTE)m_byteSize;

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
#ifdef DEBUG
				DWORD err = GetLastError();
				std::cerr << std::to_string(err) << std::endl;
#endif // DEBUG

				abort();
			}
			else
			{				
				uint8_t* input_buffer = new uint8_t[SW_BUFFER_SIZE];
				OVERLAPPED serial_status = { 0 };
				BOOL stat_check_issued = FALSE;
				DWORD comm_event = { 0 };
				DWORD pending_object;
				DWORD ov_res;

				serial_status.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
				assert(serial_status.hEvent != NULL);

				while (m_continuePoll.test_and_set())
				{
					//	check for a previously issued status check
					if (!stat_check_issued)
					{
						//	issue a check for status
						if (!WaitCommEvent(m_pComm, &comm_event, &serial_status))
						{
							// did not return immediately, check for pending check
							if (GetLastError() == ERROR_IO_PENDING)
							{
								stat_check_issued = TRUE;
							}
							else
							{
								//	could not issue a status check
								stat_check_issued = FALSE;
							}
						}
						else
						{
							// returned immediately
							handle_data();
						}
					}

					//	handle an issued status check
					if (stat_check_issued)
					{
						pending_object = WaitForSingleObject(serial_status.hEvent, 500);

						switch (pending_object)
						{
						case WAIT_OBJECT_0:
							if (!GetOverlappedResult(m_pComm, &serial_status, &ov_res, FALSE))
							{
								//	[error]: in overlapped operation
							}
							else
							{
								handle_data();
							}
							stat_check_issued = FALSE;
							break;


						case WAIT_TIMEOUT:
							//	operation pending
							break;


						default:
							break;
						}
					}
				}

				//	recycle the buffer
				delete[] input_buffer;
			}
		}



		/**********************************************************************
		 *	Handle data received on the port.
		 *
		 *	This method is to be called by the worker thread for checking the
		 *		RX data, reading it into a buffer, and raising an event.
		 */
		void SerialDevice::handle_data()
		{
			size_t _available = Available();
			if (_available)
			{
				uint8_t* _buf = new uint8_t[_available];
				win32_read(_buf, _available);
				ReceivedData(std::string((char*)_buf, _available));
				delete[] _buf;
			}
		}
	}
}

