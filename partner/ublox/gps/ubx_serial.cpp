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
 * $Id: ubx_serial.cpp 58858 2012-05-24 12:33:07Z jon.bowern $
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#if defined (ANDROID_BUILD)
#include <termios.h>
#else
#include <sys/termios.h>
#endif

#if defined serial_icounter_struct
#include <linux/serial.h>
#endif
#include <linux/i2c.h>

#include "std_types.h"
#include "std_lang_def.h"
#include "std_macros.h"
#include "ubx_log.h"
#include "ubx_serial.h"

int CSerialPort::settermios(int ttybaud, int blocksize)
{
    struct termios  termios;
	if (m_i2c)	return 0;
	
	if (tcgetattr(m_fd, &termios) < 0)
    {
        return (-1);
    }
    cfmakeraw (&termios);
    termios.c_cflag |= CLOCAL;
/*    {
        int cnt;

        for (cnt = 0; cnt < NCCS; cnt++)
        {
            termios.c_cc[cnt] = -1;
        }
    } */
    
	if (blocksize >= 0)
		termios.c_cc[VMIN] = blocksize;
    termios.c_cc[VTIME] = 0;

#if (B4800 != 4800)
    /*
     * Only paleolithic systems need this.
     */

    switch (ttybaud)
    {
    case 300:
        ttybaud = B300;
        break;
    case 1200:
        ttybaud = B1200;
        break;
    case 2400:
        ttybaud = B2400;
        break;
    case 4800:
        ttybaud = B4800;
        break;
    case 9600:
        ttybaud = B9600;
        break;
    case 19200:
        ttybaud = B19200;
        break;
    case 38400:
        ttybaud = B38400;
        break;
    case 57600:
        ttybaud = B57600;
        break;
    case 115200:
        ttybaud = B115200;
        break;
    default:
        ttybaud = B4800;
        break;
    }
#endif

    if (cfsetispeed(&termios, ttybaud) != 0)
    {
        return (-1);
    }
#ifdef BIDIRECTIONAL
    if (cfsetospeed(&termios, ttybaud) != 0)
    {
        return (-1);
    }
#endif
    if (tcsetattr(m_fd, TCSANOW, &termios) < 0) 
    {
        return (-1);
	}
	return 0;
}

bool CSerialPort::openSerial(const char * pTty, int ttybaud, int blocksize)
{
	m_i2c = strncmp(pTty, "/dev/i2c", 8) == 0;
	m_fd = open(pTty, (m_i2c ? 0 : O_NOCTTY)
              | O_RDWR 
#ifdef O_EXLOCK                 /* for Linux */
              | O_EXLOCK
#endif
        );
    if (m_fd < 0)
    {
        LOGE("Cannot open serial port %s, return %d %d", pTty, m_fd, m_i2c);
        return false;
    }

	if (m_i2c)
	{
#ifndef UNIX_API
		int io = ioctl(m_fd, I2C_SLAVE, 0x42);
		char adr = 0xFF;
		if (io < 0)
		{
			LOGE("no i2c device");
	        close (m_fd);
			m_fd = -1;
			return false;
		}
		LOGV("GPS detected on I2C %s", pTty);
    	if (write(m_fd, &adr, 1) != 1)
		{
			LOGE("set register failed");
	        close (m_fd);
			m_fd = -1;
			return false;
		}
#endif		
	}
	else
	{
		if (settermios(ttybaud, blocksize) == -1)
		{
			LOGE("Cannot invoke settermios");
			close (m_fd);
			m_fd = -1;
			return false;
		}
#ifdef BIDIRECTIONAL
		tcflush(m_fd, TCIOFLUSH);
#else
	    tcflush(m_fd, TCIFLUSH);
#endif
		fcntl(m_fd, F_SETFL, 0);

#if defined serial_icounter_struct
		/* Initialize the error counters */
		if (!ioctl(m_fd,TIOCGICOUNT, &einfo))
		{
			/* Nothing to do... */
		}
		else
		{
			/* error, cannot read the error counters */
			LOGW("unable to read error counters!!!");
		}
#endif
	}
	LOGV("Serial port opened, fd = %d", m_fd);
    return true;
}

int CSerialPort::changeBaudrate(char * pTty, int * pBaudrate, unsigned char * pBuf, int length)
{
    unsigned long newbaudrate = 0;
	if (m_i2c)			  
	{
		*pBaudrate = (int)newbaudrate;
        return 1;
	}
	if (length != 28)	  return 0;
    if (pBuf[0] != 0xb5)  return 0;
    if (pBuf[1] != 0x62)  return 0;
    if (pBuf[2] != 0x06)  return 0;
    if (pBuf[3] != 0x00)  return 0;
    if (pBuf[4] != 0x14)  return 0;
    if (pBuf[5] != 0x00)  return 0;
    if ((pBuf[6] != 0x01) && (pBuf[6] != 0xff))  return 0;

    memcpy(&newbaudrate, pBuf+8+6,4);
  
    if (newbaudrate != (unsigned long) *pBaudrate)
    {
        int tmp = *pBaudrate;
        
        close (m_fd);

        LOGW("Attempting to change baudrate from %d to %lu on %s)",tmp,newbaudrate, pTty);

        /* Wait a while... */
        usleep(100000);
      
        if (!openSerial(pTty, newbaudrate, 2))
        {
            // failed ?!?
            LOGE("%s: %s", pTty, strerror(errno));
            openSerial(pTty, *pBaudrate, 2);
            return 0;
        } else {

            *pBaudrate = (int)newbaudrate;
            LOGW("Changed baudrate from to %i on %s)",*pBaudrate, pTty);
            return 1;
        }
    }
    return 0;
}

int CSerialPort::retrieveErrors()
{
    int res = 0;
	if (m_i2c)
		return res;
#if defined serial_icounter_struct
    struct serial_icounter_struct einfo_tmp;

    /* read the counters */
    if (!ioctl(m_fd,TIOCGICOUNT, &einfo_tmp))
    {
        /* check if something has changed */
        if (einfo_tmp.frame != einfo.frame)
        {
            LOGW("Frame error occurred!!!! (%d)",einfo_tmp.frame - einfo.frame);

            /* return the number of frame errors */
            res = einfo_tmp.frame - einfo.frame;
        }

        else if (einfo_tmp.brk != einfo.brk)
        {
            LOGW("Breaks occurred!!!! (%d)",einfo_tmp.brk - einfo.brk);

            /* return the number of frame errors */
            res = einfo_tmp.brk - einfo.brk;
        }

        /* update the stored counters */
        memcpy(&einfo, &einfo_tmp, sizeof(einfo));
    }
    else
    {
        /* cannot read the counters! ignore, as we might be talking to a device without error counters */
    }
#endif
    return res;
}

void CSerialPort::baudrateIncrease(int *pBaudrate)
{
    int i;
    int oldBaudrate = *pBaudrate;

    /* check next baudrate */
    for (i = 0; i< BAUDRATE_TABLE_SIZE; i++)
    {
        if ( *pBaudrate < s_baudrateTable[i])
        {
            *pBaudrate = s_baudrateTable[i];
            break;
        }
    }
    if (i == BAUDRATE_TABLE_SIZE)
    {
        *pBaudrate = s_baudrateTable[0];
    }
    
    LOGW("Attempting to change baudrate from %d to %d", oldBaudrate, *pBaudrate);

    return;
}


const int CSerialPort::s_baudrateTable[BAUDRATE_TABLE_SIZE] =
{
    4800,
    9600,
    19200,
    38400,
    57600,
    115200,
    230400,
};

