//	Copyright (c) 2019 LooUQ Incorporated.

//	Licensed under the GNU GPLv3. See LICENSE file in the project root for full license information.
#include "Win32.Devices.SerialDevice.h

#define NO_SHARING	NULL
#define NO_SECURITY	NULL
#define NON_OVERLAPPED_IO	NULL
#define NO_FLAGS	NULL



namespace Win32
{
	namespace Devices
	{		
		SerialDevice::SerialDevice(std::nullptr_t)
		{
			throw std::exception("Serial Device was unavailable or not found!");
		}



		SerialDevice::SerialDevice(SerialDevice* serialDevicePtr)
			: m_pComm(serialDevicePtr->m_pComm)
			, m_portNum(serialDevicePtr->m_portNum)
		{
			serialDevicePtr->m_pComm = nullptr;
			delete serialDevicePtr;

			config_settings();
			config_timeouts();
			clear_comm();
		}



		SerialDevice::~SerialDevice()
		{
			Close();
		}



		SerialDevice* SerialDevice::FromPortNumber(uint16_t COMPortNum)
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
				return nullptr;
			}
			else
			{										
				return new SerialDevice(h_sercom, COMPortNum);
			}
		}



		void SerialDevice::Close()
		{
			if (m_pComm != nullptr)
			{
				CloseHandle(m_pComm);
				m_pComm = nullptr;
			}				

			if (m_thCommEv != nullptr)
			{
				delete m_thCommEv;
				m_thCommEv = nullptr;
			}
		}



		void SerialDevice::UsingEvents(bool usingCommEv)
		{
			m_thCommEv = new std::thread(&SerialDevice::interrupt_thread, this);
			m_thCommEv->detach();
		}



		void SerialDevice::Write(const std::string& src_str)
		{
			write(src_str.c_str(), src_str.length());
		}



		void SerialDevice::Read(std::string& dest_str)
		{
			char temp[128];
			size_t len = read(temp, 128);
			dest_str = { temp, len };
		}

		uint32_t SerialDevice::Available()
		{
			DWORD err_flags = { 0 };
			COMSTAT com_status = { 0 };
			ClearCommError(m_pComm, &err_flags, &com_status);
				
			return com_status.cbInQue;
		}



		void SerialDevice::BaudRate(uint32_t baudrate)
		{
			m_baudrate = baudrate;
			config_settings();
		}



		uint32_t SerialDevice::BaudRate() const
		{
			return m_baudrate;
		}



		void SerialDevice::StopBits(uint8_t stopBits)
		{
			//m_stopBits = stopBits;
			config_settings();
		}



		uint8_t SerialDevice::StopBits() const
		{				
			return uint8_t();
		}



		void SerialDevice::ByteSize(SerialByteSize byteSize)
		{
			m_byteSize = byteSize;
			config_settings();
		}



		SerialByteSize SerialDevice::ByteSize() const
		{
			return m_byteSize;
		}



		void SerialDevice::write(const void* _src, size_t len)
		{
			DWORD written;
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



		size_t SerialDevice::read(void* _dest, size_t len)
		{
			//DWORD event_mask = { 0 };
			//WaitCommEvent(m_pComm, &event_mask, NULL);

			DWORD read_;
			if (!ReadFile(m_pComm, _dest, len, &read_, NULL))
			{
				std::cerr << "Serial Error: Unable to read all bytes!" << std::endl;
			}
			return read_;
		}



		void SerialDevice::config_settings()
		{
			DCB data_cntrl_blk = { 0 };
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



		void SerialDevice::config_timeouts()
		{
			COMMTIMEOUTS timeouts;
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


		void SerialDevice::clear_comm()
		{
			//	Clear the port
			if (PurgeComm(m_pComm, PURGE_TXCLEAR | PURGE_RXCLEAR) == 0)
			{
				std::cerr << "Serial Error: Unable to clear the port!" << std::endl;
			}
		}


		void SerialDevice::interrupt_thread()
		{
			if (SetCommMask(m_pComm, EV_RXCHAR) == FALSE)
			{
				std::cerr << "Serial Error: Unable to set comm mask!" << std::endl;
			}
			else
			{
				while (true)
				{
					DWORD event_mask = { 0 };
					WaitCommEvent(m_pComm, &event_mask, NULL);
					size_t len = Available();
					uint8_t* buf = new uint8_t[len];
					read(buf, len);
					ReceivedData(std::string((char*)buf, len));
					delete[] buf;
				}
			}
		}		
	}
}

