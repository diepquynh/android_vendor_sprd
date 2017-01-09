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
  \brief  Local UDP Port Handling

  This file provides the implementation of local ports. A master (i.e. what is 
  implemented here) broadcasts data it has received to local clients which wait 
  for UDP traffic on a defined port.

  If clients talk, the traffic is received in here and delivered to the 
  upper layers.

  Connection handling, keep-alive  timeouting is also being done in here
*/
/*******************************************************************************
 * $Id: ubx_udpServer.h 57451 2012-04-12 12:05:33Z jon.bowern $
 ******************************************************************************/

#ifndef __UBX_UDPSERVERL_H__
#define __UBX_UDPSERVERL_H__

#include <arpa/inet.h>
#include <fcntl.h>

#ifndef MAXUDPCONN
#define MAXUDPCONN 2 //!< Maximum number of UDP connected clients
#endif

#define SESSION_TIMEOUT    4            //<! in seconds is the timeout for UDP connection
#define SESSION_TIMEOUT_MS (4 * 1000)   //<! in milliseconds is the timeout for UDP connection


class CUdpServer
{
public:
    CUdpServer()
    {
        m_fd = -1;
        m_slaveStatus = 0;
    };
    ~CUdpServer(){};

    bool openLocalPort(int port);
    int recvPort(char * pBuf, int buflen);
    void checkPort(int slaveOpen);
    void sendPort(unsigned char * pBuf, int len);

    bool fdSet(fd_set &rfds, int &rMaxFd)
    {
        if (m_fd <= 0)
            return false;
        if ((m_fd + 1) > rMaxFd)
            rMaxFd = m_fd + 1;
        FD_SET(m_fd, &rfds);
        return true;
    };

    void closeUdp()
    {
        close(m_fd);
		m_fd = -1;
    };

    bool fdIsSet(fd_set &rfds)
    {
        if ((m_fd > 0) && (FD_ISSET(m_fd, &rfds)))
            return true;
        return false;
    };

private:
    int m_fd;	// file descriptor for UDP socket
	
    int findConnection (struct sockaddr_in si_other);
    int findEmptySlot (void);
    int activeConnections (void);

    typedef struct udpConn_s 
    {
        int64_t seen;
        struct sockaddr_in conn;
        unsigned int flags;
    } udpConn_t;

	// Array where all connection-related info is stored:
    udpConn_t m_udpConn[MAXUDPCONN];

    int m_slaveStatus;
};

#endif /* __UBX_UDPSERVERL_H__ */
