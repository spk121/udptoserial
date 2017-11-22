#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include "serial.h"

#ifdef WIN32
static serial_port_t *serial_port_new_win32(const char *port_name);
static void serial_perror(const char* str, int code);
#else
static serial_port_t *serial_port_new_linux(const char *port_name);
static void serial_port_send_linux(const serial_port_t *sp, const char *buf, size_t len);
#endif

/* Open a serial port and find its output baud rate. */
serial_port_t *serial_port_new(const char *port_name)
{
#ifdef WIN32
	return serial_port_new_win32(port_name);
#else
	return serial_port_new_linux(port_name);
#endif
}

void serial_port_info(const serial_port_t *sp)
{
	if (sp == NULL)
	{
		printf("NO SERIAL PORT\n");
		return;
	}

	printf("Serial port %s\n", sp->ttyname);
	printf("    Baud rate = %d\n", sp->baud_rate);
	printf("    Byte size = %d\n", sp->byte_size);
	printf("    Stop bits = %d\n", sp->stop_bits);
	if (sp->parity == SP_NOPARITY)
		printf("    Parity = NONE\n");
	else if (sp->parity == SP_EVENPARITY)
		printf("    Parity = EVEN\n");
	else if (sp->parity == SP_ODDPARITY)
		printf("    Parity = ODD\n");
}

#ifdef WIN32
static serial_port_t *serial_port_new_win32(const char *port_name)
{
	serial_port_t *sp;
	HANDLE h_serial;
	BOOL Status;

	sp = (serial_port_t *)malloc(sizeof(serial_port_t));
	memset(sp, 0, sizeof(serial_port_t));

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
	sp->baud_rate = dcbSerialParams.BaudRate;
	sp->byte_size = dcbSerialParams.ByteSize;
	sp->handle = h_serial;
	if (dcbSerialParams.Parity == 0)
		sp->parity = SP_NOPARITY;
	if (dcbSerialParams.Parity == 1)
		sp->parity = SP_ODDPARITY;
	if (dcbSerialParams.Parity == 2)
		sp->parity = SP_EVENPARITY;
	if (dcbSerialParams.StopBits == 0)
		sp->stop_bits = 1;
	if (dcbSerialParams.StopBits == 2)
		sp->stop_bits = 2;
	sp->ttyname = strdup(port_name);
	return sp;
}
#endif

#ifndef WIN32
static char *make_full_port_name_linux(const char* port_name)
{
	char *full_port_name = NULL;
	if (port_name == NULL || strlen(port_name) == 0)
	{
		/* No port name was given, so try this default. */
		full_port_name = strdup("/dev/ttyS1");
	}
	else if (port_name[0] == '/')
	{
		/* Assume this already is a full port name. */
		full_port_name = strdup(port_name);
	}
	else if (strlen(port_name) == 1 || isdigit(port_name[0]))
	{
		/* If it is just a number, try it as a ttyS style port. */
		full_port_name = strdup("/dev/ttyS0");
		full_port_name[9] = port_name[0];
	}
	else
	{
		full_port_name = (char *) malloc (strlen(port_name) + 5);
		/* Eh, try this. */
		strncpy(full_port_name, "/dev/", 5);
		strncpy(full_port_name + 5, port_name, strlen(port_name));
	}
	return full_port_name;
}

serial_port_t *serial_port_new_linux(const char *port_name)
{
	char *full_port_name = NULL;
	serial_port_t *sp = NULL;
	struct termios tinfo;
	#define TTY_NAME_MAX_LEN_LINUX 13
   	char tty_name_buf[TTY_NAME_MAX_LEN_LINUX];

	sp = (serial_port_t *)malloc(sizeof(serial_port_t));
	memset (sp, 0, sizeof(serial_port_t));
	memset (&tinfo, 0, sizeof(tinfo));

	/* Create a fully qualified port name, such as "/dev/ttyS0". */
	full_port_name = make_full_port_name_linux(port_name);
	sp->handle = open(full_port_name, O_RDWR | O_NOCTTY);
	free (full_port_name);

  	if (sp->handle == -1 || isatty(sp->handle) != 1)
    {
		perror("Opening serial port");
		free (sp);
		return NULL;	
    }

	/* OK, we've opened a TTY. */
	/* Longest TTY name on Linux is 12 bytes "/dev/ttyS99" */	
    if (ttyname_r(sp->handle, tty_name_buf, TTY_NAME_MAX_LEN_LINUX) != 0)
	{
		perror("Get tty name");
		goto err_tty;
	}
	sp->ttyname = strdup(tty_name_buf);

	if (tcgetattr(sp->handle, &tinfo) == -1)
	{
		perror("get serial port attributes");
		goto err_tty;
	}
    
    /* Put the serial port info into raw mode. */
    cfmakeraw(&tinfo);
	if (tcsetattr (sp->handle, TCSANOW, &tinfo) < 0)
	{
		perror("set serial port to raw mode");
		goto err_tty;
	}

	/* Gather serial port information. */
	sp->baud_rate = speed_to_bps(cfgetospeed(&tinfo));
	if ((tinfo.c_cflag & CSIZE) == CS5)
		sp->byte_size = 5;
	else if ((tinfo.c_cflag & CSIZE) == CS6)
		sp->byte_size = 6;
	else if ((tinfo.c_cflag & CSIZE) == CS7)
		sp->byte_size = 7;
	else if ((tinfo.c_cflag & CSIZE) == CS8)
		sp->byte_size = 8;
	if (tinfo.c_cflag & CSTOPB)
		sp->stop_bits = 2;
	else
		sp->stop_bits = 1;
	if ((tinfo.c_cflag & PARENB) == 0)
		sp->parity = SP_NOPARITY;
	else if ((tinfo.c_cflag & PARENB) && (tinfo.c_cflag & PARODD))
		sp->parity = SP_ODDPARITY;
	else
		sp->parity = SP_EVENPARITY;
  
	return sp;	

err_tty:
	if (sp)
	{
		free (sp->ttyname);
		if (sp->handle > 0)
			close (sp->handle);
	}
	free (sp);
	return NULL;
}
#endif

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

#ifdef WIN32
static void serial_port_send_win32(const serial_port_t *sp, const char *buf, size_t len)
{
	DWORD  dNoOFBytestoWrite = len;              // No of bytes to write into the port
	DWORD  dNoOfBytesWritten = 0;          // No of bytes written to the port
	DWORD Status;

	if (sp == NULL)
	{
		printf("NO SERIAL PORT...\n");
		for (size_t i = 0; i < len; i++)
			putc(buf[i], stdout);
		printf("\n");
	}
	else
	{

		Status = WriteFile(sp->handle,               // Handle to the Serialport
			buf,            // Data to be written to the port 
			dNoOFBytestoWrite,   // No of bytes to write into the port
			&dNoOfBytesWritten,  // No of bytes written to the port
			NULL);

		if (Status != TRUE)
		{
			COMSTAT comStat;
			DWORD   dwErrors;
			BOOL    fOOP, fOVERRUN, fPTO, fRXOVER, fRXPARITY, fTXFULL;
			BOOL    fBREAK, fDNS, fFRAME, fIOE, fMODE;

			serial_perror("serial_port_send()", GetLastError());

			// Get and clear current errors on the port.
			if (!ClearCommError(sp->handle, &dwErrors, &comStat))
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
		}
	}

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
#else
static void serial_port_send_linux(const serial_port_t *sp, const char *buf, size_t len)
{
	ssize_t bytes_written;

	/* Write the specified number of bytes to the serial port. */
	if (sp == NULL)
	{
		printf("NO SERIAL PORT...\n");
		for (size_t i = 0; i < len; i ++)
			putc(buf[i], stdout);
		printf("\n");
	}
	else
	{
		bytes_written = write (sp->handle, buf, len);
		if (bytes_written < 0)
		{
			perror ("Writing to serial port");
		}
	}
}
#endif

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

void serial_port_send(const serial_port_t *sp, const char *buf, size_t len)
{
#ifdef WIN32
	serial_port_send_win32(sp, buf, len);
#else
	serial_port_send_linux(sp, buf, len);
#endif
}

