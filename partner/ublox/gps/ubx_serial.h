/*******************************************************************************
 *
 * Copyright (C) u-blox ag
 * u-blox ag, Thalwil, Switzerland
 *
 * All rights reserved.
 *
 * This  source  file  is  the  sole  property  of  u-blox  AG. Reproduction or
 * utilization of this source in whole or part is forbidden without the written 
 * consent of u-blox AG.
 *
 *******************************************************************************
 *
 * Project: PE_AND
 *
 ******************************************************************************/
/*!
  \file
  \brief  Serial utility functions

  Utility for accessing the serial port in a easier way
*/
/*******************************************************************************
 * $Id: ubx_serial.h 61295 2012-08-23 14:36:13Z jon.bowern $
 ******************************************************************************/

#ifndef __UBX_SERIAL_H__
#define __UBX_SERIAL_H__

#include <fcntl.h>
#include <malloc.h>
#include <string.h>

#define BAUDRATE_TABLE_SIZE 7

class CSerialPort
{
public:
    CSerialPort()
    {
        m_fd = -1;
		m_i2c = false;
    };
    ~CSerialPort(){};

    bool openSerial(const char * pTty, int ttybaud, int blocksize);

    int setbaudrate(int ttybaud)
    {
        return settermios(ttybaud, -1);
    };

    int changeBaudrate(char * ptty, int * pBaudrate, unsigned char * pBuf, int length);

    void baudrateIncrease(int *baudrate);

    int retrieveErrors();

    void closeSerial()
    {
        if (m_fd > 0)
            close(m_fd);
		m_fd = -1;
		m_i2c = false; 
    };

    bool fdSet(fd_set &rfds, int &rMaxFd)
    {
        if (m_fd <= 0)
            return false;
        if ((m_fd + 1) > rMaxFd)
            rMaxFd = m_fd + 1;
        FD_SET(m_fd, &rfds);
        return true;
    };

    bool fdIsSet(fd_set &rfds)
    {
        if ((m_fd > 0)  && FD_ISSET(m_fd, &rfds))
            return true;
        return false;
    };

    int readSerial(void *pBuffer, int size)
    {
        if (m_fd <= 0)
            return -1;
        return read(m_fd, pBuffer, size);
    };

    int writeSerial(void *pBuffer, int size)
    {
		char* p;
        
		if (m_fd <= 0)
            return -1;
		if (!m_i2c)
	        return write(m_fd, pBuffer, size);
	
		p = (char*)malloc(size+1);
		if (!p)
			return 0;
		p[0] = 0xFF; // allways address the stream register
		memcpy(p+1, pBuffer, size);
		size = write(m_fd, p, size+1);
		if (size > 0)
			size --;
		free(p);
		return size;
	};

	bool isFdOpen(void)
	{
		return m_fd > 0;
	}
	
private:
    int m_fd;
	bool m_i2c;

    int settermios(int ttybaud, int blocksize);

    static const int s_baudrateTable[BAUDRATE_TABLE_SIZE];

	//! container of error counters
#if defined serial_icounter_struct
    static struct serial_icounter_struct s_einfo;
#endif

};

#endif /* __UBX_SERIAL_H__ */

