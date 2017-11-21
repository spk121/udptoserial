#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <termios.h>
#endif
#include "serial.h"

#ifdef WIN32
static void serial_perror(const char* str, int code);
#endif

serial_port_t *serial_port_new(const char *port_name)
{
	serial_port_t *sp = (serial_port_t *)malloc(sizeof(serial_port_t));
#ifdef WIN32
	HANDLE h_serial;
	BOOL Status;

	h_serial = CreateFile(port_name,
		GENERIC_READ | GENERIC_WRITE,
		0,
		0,
		OPEN_EXISTING,
		0,
		0);
	if (h_serial == INVALID_HANDLE_VALUE)
		printf("\n   Error! - Port %s can't be opened", port_name);
	else
		printf("\n   Port %s Opened\n ", port_name);

	DCB dcbSerialParams = { 0 };                        // Initializing DCB structure
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	Status = GetCommState(h_serial, &dcbSerialParams);     //retreives  the current settings
	if (Status == FALSE)
		printf("\n   Error! in GetCommState()");
	else
	{
		printf("\n   Current serial port settings\n");
		printf("\n       Baudrate = %d", dcbSerialParams.BaudRate);
		printf("\n       ByteSize = %d", dcbSerialParams.ByteSize);
		if (dcbSerialParams.StopBits == 0)
			printf("\n       StopBits = 1\n");
		if (dcbSerialParams.StopBits == 1)
			printf("\n       StopBits = 1.5\n");
		if (dcbSerialParams.StopBits == 2)
			printf("\n       StopBits = 2\n");
		if (dcbSerialParams.Parity == 0)
			printf("\n       Parity   = NONE\n");
		if (dcbSerialParams.Parity == 1)
			printf("\n       Parity   = ODD\n");
		if (dcbSerialParams.Parity == 2)
			printf("\n       Parity   = EVEN\n");
	}

#if 0
	dcbSerialParams.BaudRate = CBR_9600;      // Setting BaudRate = 9600
	dcbSerialParams.ByteSize = 8;             // Setting ByteSize = 8
	dcbSerialParams.StopBits = ONESTOPBIT;    // Setting StopBits = 1
	dcbSerialParams.Parity = NOPARITY;      // Setting Parity = None 

	Status = SetCommState(h_serial, &dcbSerialParams);  //Configuring the port according to settings in DCB 

	if (Status == FALSE)
	{
		printf("\n   Error! in Setting DCB Structure");
	}
	printf("\n   Setting DCB Structure Successfull\n");

	Status = GetCommState(h_serial, &dcbSerialParams);     //retreives  the current settings
	if (Status == FALSE)
		printf("\n   Error! in GetCommState()");
	else
	{
		printf("\n   Current serial port settings\n");
		printf("\n       Baudrate = %d", dcbSerialParams.BaudRate);
		printf("\n       ByteSize = %d", dcbSerialParams.ByteSize);
		if (dcbSerialParams.StopBits == 0)
			printf("\n       StopBits = 1\n");
		if (dcbSerialParams.StopBits == 1)
			printf("\n       StopBits = 1.5\n");
		if (dcbSerialParams.StopBits == 2)
			printf("\n       StopBits = 2\n");
		if (dcbSerialParams.Parity == 0)
			printf("\n       Parity   = NONE\n");
		if (dcbSerialParams.Parity == 1)
			printf("\n       Parity   = ODD\n");
		if (dcbSerialParams.Parity == 2)
			printf("\n       Parity   = EVEN\n");
	}
#endif
	/*------------------------------------ Setting Timeouts --------------------------------------------------*/
#if 0
	COMMTIMEOUTS timeouts = { 0 };

	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;

	if (SetCommTimeouts(h_serial, &timeouts) == FALSE)
		printf("\n   Error! in Setting Time Outs");
	else
		printf("\n\n   Setting Serial Port Timeouts Successfull");
#endif

#else
#endif
	sp->bps = dcbSerialParams.BaudRate;
	sp->handle = h_serial;
	return sp;
}

#ifdef WIN32
int speed_to_bps(DWORD speed)
{
	return speed;
}
#else
int speed_to_bps(speed_t speed)
{
	if (speed == 0)
		return 0;
	else if (speed == B50)
		return 50;
	else if (speed == B75)
		return 75;
	else if (speed == B110)
		return 110;
	else if (speed == B134)
		return 134;
	else if (speed == B150)
		return 150;
	else if (speed == B200)
		return 200;
	else if (speed == B300)
		return 300;
	else if (speed == B600)
		return 600;
	else if (speed == B1200)
		return 1200;
	else if (speed == B1800)
		return 1800;
	else if (speed == B2400)
		return 2400;
	else if (speed == B4800)
		return 4800;
	else if (speed == B9600)
		return 9600;
	else if (speed == B19200)
		return 19200;
	else if (speed == B38400)
		return 38400;
	else if (speed == B57600)
		return 57600;
	else if (speed == B115200)
		return 115200;
	else if (speed == B230400)
		return 230400;

	return -1;
}
#endif

void serial_port_send(serial_port_t *port, char *lpBuffer)
{
	if (port == NULL)
	{
		printf("NO SERIAL PORT...\n");
		puts(lpBuffer);
		printf("\n");
	}
	else
	{
		DWORD  dNoOFBytestoWrite;              // No of bytes to write into the port
		DWORD  dNoOfBytesWritten = 0;          // No of bytes written to the port
		DWORD Status;

		dNoOFBytestoWrite = strlen(lpBuffer); // Calculating the no of bytes to write into the port

		Status = WriteFile(port->handle,               // Handle to the Serialport
			lpBuffer,            // Data to be written to the port 
			dNoOFBytestoWrite,   // No of bytes to write into the port
			&dNoOfBytesWritten,  // No of bytes written to the port
			NULL);

		if (Status == TRUE)
			printf("\n\n    %s - Written to port", lpBuffer);
		else
		{
			COMSTAT comStat;
			DWORD   dwErrors;
			BOOL    fOOP, fOVERRUN, fPTO, fRXOVER, fRXPARITY, fTXFULL;
			BOOL    fBREAK, fDNS, fFRAME, fIOE, fMODE;

			serial_perror("serial_port_send()", GetLastError());

			// Get and clear current errors on the port.
			if (!ClearCommError(port->handle, &dwErrors, &comStat))
				// Report error in ClearCommError.
				return;

			// Get error flags.
			fDNS = dwErrors & CE_DNS;
			fIOE = dwErrors & CE_IOE;
			fOOP = dwErrors & CE_OOP;
			fPTO = dwErrors & CE_PTO;
			fMODE = dwErrors & CE_MODE;
			fBREAK = dwErrors & CE_BREAK;
			fFRAME = dwErrors & CE_FRAME;
			fRXOVER = dwErrors & CE_RXOVER;
			fTXFULL = dwErrors & CE_TXFULL;
			fOVERRUN = dwErrors & CE_OVERRUN;
			fRXPARITY = dwErrors & CE_RXPARITY;

#if 0
			// COMSTAT structure contains information regarding
			// communications status.
			if (comStat.fCtsHold)
				// Tx waiting for CTS signal

				if (comStat.fDsrHold)
					// Tx waiting for DSR signal

					if (comStat.fRlsdHold)
						// Tx waiting for RLSD signal

						if (comStat.fXoffHold)
							// Tx waiting, XOFF char rec'd

							if (comStat.fXoffSent)
								// Tx waiting, XOFF char sent

								if (comStat.fEof)
									// EOF character received

									if (comStat.fTxim)
										// Character waiting for Tx; char queued with TransmitCommChar

										if (comStat.cbInQue)
											// comStat.cbInQue bytes have been received, but not read

											if (comStat.cbOutQue)
												// comStat.cbOutQue bytes are awaiting transfer
#endif
		}
	}
}

#ifdef WIN32
static void serial_perror(const char* str, int code)
{
	static char msgbuf[256];
	msgbuf[0] = L'\0';

	if (code != -1)
	{
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,   // flags
			NULL,                // lpsource
			code,                 // message id
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),    // languageid
			msgbuf,              // output buffer
			256,     // size of msgbuf, bytes
			NULL);               // va_list of arguments

		if (!*msgbuf)
			printf("error #%d", code);
		else
			printf("%s: %s\n", str, msgbuf);
	}
}
#endif