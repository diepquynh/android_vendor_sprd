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
 * $Id: ubx_udpServer.cpp 61901 2012-09-14 10:16:33Z jon.bowern $
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#include "ubx_log.h"
#include "ubx_mux.h"
#include "ubx_timer.h"
#include "ubx_udpServer.h"


bool CUdpServer::openLocalPort(int udpPort)
{
    struct sockaddr_in si_me; // Structures for me with socket addr info

    if (m_fd != -1)
		close(m_fd);
	/* open socket */
	if ((m_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        LOGW("CUdpServer::%s: unable to create socket: %s\n", __FUNCTION__, strerror(errno));
        return -1;
    }
    memset((char *) &si_me, 0, sizeof(si_me));
    memset((char *) m_udpConn, 0, sizeof(m_udpConn)); // set all connection info to zero
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(udpPort);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_fd, (struct sockaddr *)&si_me, sizeof(si_me))==-1)
    {
        LOGW("CUdpServer::%s: unable to bind socket: %s\n", __FUNCTION__, strerror(errno));
		close(m_fd);
		m_fd = -1;
    }
	else
		LOGV("Udp port opened, fd = %d", m_fd);
    return m_fd;
}

int CUdpServer::recvPort(char * pBuf,int buflen)
{
    // Enter here if select has indicated that we can read
    // from the local client
    // handle new connections from clients / ping replies
	
    struct sockaddr_in si_other; // Structures for me and others with socket addr info
    int slen = sizeof(si_other);
    int conn;
    int len = recvfrom(m_fd, pBuf, buflen, 0, (struct sockaddr *)&si_other, (socklen_t *)&slen);
	//LOGV("%s: Receiving data (%i)", __FUNCTION__, len);
    if (len < 0)
    {
        LOGW("CUdpServer::%s: recvfrom failed %s", __FUNCTION__, strerror(errno));
    }

    if (len == 0)
    {
        LOGW("CUdpServer::%s: zero length packet no more allowed... ", __FUNCTION__);
    }

    conn = findConnection(si_other);
    if (conn == -1) // not known connection
    {
        /* we check only if new connection request! */
        if ( (len == 3) && (pBuf[0] == UDP_MUX_PACKET_HEADER) && (pBuf[1] == UDP_MUX_START_SESSION) )
        {
            conn = findEmptySlot();
            if (conn == -1)
            {
                LOGW("No empty slot found!");
            }
            else
            {

                /* record the source IP and port */
                memcpy( (char *) &m_udpConn[conn].conn, (char *)&si_other,sizeof(si_other));

                /* set the feature of this connection */
                m_udpConn[conn].flags = (unsigned int) pBuf[2];

                /* update the time stamp */
                m_udpConn[conn].seen = getMonotonicMsCounter();

                LOGI("new connection %i (%s:%i), feature 0x%.2X",
                      conn,
                      inet_ntoa(m_udpConn[conn].conn.sin_addr), 
                      ntohs(m_udpConn[conn].conn.sin_port),
                      m_udpConn[conn].flags);
					  
                /* Send the notification to the clients that has requested to be informed */
                int a = activeConnections();
                unsigned char buf[4] = {'X', UDP_MUX_NUM_CLIENT, (a & 0xFF), ( (a>>8) & 0xFF) };
                sendPort(buf, sizeof(buf));
			}
        }
        else
        {
            /* UDP message not recognized!!! */
            LOGW("CUdpServer::%s: The message has not been recognized: len = %d, code = %d", __FUNCTION__, len, (len>0 ? pBuf[0] : 0) );
        }

        /* return 0, not forwarding the message! */
        return 0;
    }
  
    /* here we are only if the connection has been found! */
    m_udpConn[conn].seen = getMonotonicMsCounter(); // update timeout data, as we have received data from this client
    if (len > 0)
    {
        if ( !( pBuf[0] == UDP_MUX_PACKET_HEADER && len == 2 && pBuf[1] == UDP_MUX_PING ) )
        {
            /* exclude ping messagges */
            LOGI("from %i (%s:%i) len=%i",conn,
                  inet_ntoa(m_udpConn[conn].conn.sin_addr), 
                  ntohs(m_udpConn[conn].conn.sin_port),len);
        }
        
        if (pBuf[0] == UDP_MUX_PACKET_HEADER)
        {
        
            if (len == 2 && pBuf[1] == UDP_MUX_PING)
            {
                /* Ping replay!!!! keep silent! */
            }
            if (len == 3 && pBuf[1] == UDP_MUX_START_SESSION)
            {
                /* reset the feature of this connection */
                m_udpConn[conn].flags = (unsigned int) pBuf[2];

                LOGI("connection renegotiation %i (%s:%i), feature 0x%.2X",
                      conn,
                      inet_ntoa(m_udpConn[conn].conn.sin_addr), 
                      ntohs(m_udpConn[conn].conn.sin_port),
                      m_udpConn[conn].flags);

            }
            if ((len == 2) && (pBuf[1] == UDP_MUX_END_SESSION))
            {
                LOGI("end session for connection %i", conn);
                m_udpConn[conn].conn.sin_port = 0; // clear this entry
                // inform other clients
                int a = activeConnections();
                unsigned char buf[4] = {'X', UDP_MUX_NUM_CLIENT, (a & 0xFF), ( (a>>8) & 0xFF) };
                sendPort(buf, sizeof(buf));
            }

            /* the MUX packets are always not forwarded */
            len = 0;
        }

        /* forward all the packets for the clients in trasparent mode */
        int conn_1;

        for(conn_1=0; conn_1 < MAXUDPCONN; conn_1++)
        {
            /* it exclues the packets the clients itself has send */
            if ( (m_udpConn[conn_1].conn.sin_port) && (conn_1 != conn) && (m_udpConn[conn_1].flags & CLIENT_TRASPARENT_MODE))
            {
                ssize_t res;
                res = sendto(m_fd, pBuf, len, 0, (struct sockaddr *) &m_udpConn[conn_1].conn,sizeof(m_udpConn[conn_1].conn));
                if (res == -1)
                {
                    LOGW("Error in sending UDP packet, \"%s\"\n", strerror(errno));
                }
                else if (res != len)
                {
                    LOGW("Error in sending UDP packet, wrong lenght %d\n", (int) res);
                }
            }
        }
    }

    return len;
}

void CUdpServer::sendPort(unsigned char * pBuf, int len)
{
    int conn;
    if (m_fd < 0)
	{
        return;
	}
	//LOGV("CUdpServer::%s: Sending %i bytes", __FUNCTION__, len);
    for(conn = 0; conn < MAXUDPCONN; conn++)
    {
        if (m_udpConn[conn].conn.sin_port)
        {
            ssize_t res = 0;
            if (pBuf[0] == '$' && ((m_udpConn[conn].flags & CLIENT_NMEA_ENABLED)==CLIENT_NMEA_ENABLED))
            {
                /* send to non null client NMEA message */
                res = sendto(m_fd, pBuf, len, 0, (struct sockaddr *) &m_udpConn[conn].conn, sizeof(m_udpConn[conn].conn));
            }
            else if (pBuf[0] == (char) 0xB5 && ((m_udpConn[conn].flags & CLIENT_UBX_ENABLED) == CLIENT_UBX_ENABLED))
            {
                /* send to non null client UBX message */
                res = sendto(m_fd, pBuf, len, 0, (struct sockaddr *) &m_udpConn[conn].conn, sizeof(m_udpConn[conn].conn));
            }
            else if (pBuf[0] == 'X' && ((m_udpConn[conn].flags & CLIENT_MUX_ENABLED) == CLIENT_MUX_ENABLED))
            {
                /* send to non null client MUX message */
                res = sendto(m_fd, pBuf, len, 0, (struct sockaddr *) &m_udpConn[conn].conn, sizeof(m_udpConn[conn].conn));
            }
			else
			{
				//LOGV("%s: Nothing sent. Client on %i not showing any interest (0x%.2X) ", __FUNCTION__, conn, m_udpConn[conn].flags);
				// As this is not an error, stop error message being reported
				res = len;
			}
			
            if (res == -1)
            {
                LOGW("CUdpServer::%s: Error in sending UDP packet on %i, \"%s\"\n", __FUNCTION__, conn, strerror(errno));
            }
            else if (res != len)
            {
                LOGW("CUdpServer::%s: Error in sending UDP packet, wrong length %d (%d)\n", __FUNCTION__, (int) res, len);
            }
        }
    }
}

void CUdpServer::checkPort(int slaveOpen)
{
    int conn;
	//LOGV("%s: Checking port (%i)", __FUNCTION__, slaveOpen);
    for(conn=0; conn < MAXUDPCONN; conn++)
    {
        if (m_udpConn[conn].conn.sin_port)
        {
            int64_t now = getMonotonicMsCounter();
            
            if ((now - m_udpConn[conn].seen) >= SESSION_TIMEOUT_MS)
            {
                LOGI("timeout on connection %i (%s:%i): %lld\n",conn,
                      inet_ntoa(m_udpConn[conn].conn.sin_addr), 
                      ntohs(m_udpConn[conn].conn.sin_port),
                      (now -  m_udpConn[conn].seen) );
                m_udpConn[conn].conn.sin_port = 0; // clear this entry

                /* Send the notification to the clients that has requested to be informed */
                m_slaveStatus = slaveOpen;
                int a = activeConnections();
                unsigned char buf[4] = {'X', UDP_MUX_NUM_CLIENT, (a & 0xFF), ( (a>>8) & 0xFF) };
                sendPort(buf, sizeof(buf));
            } 
			else
			{
                /* send the ping package! */
                ssize_t res;
                char buf[] = {'X',0x00};
                res = sendto(m_fd, buf, sizeof(buf), 0, (struct sockaddr *) &m_udpConn[conn].conn, sizeof(m_udpConn[conn].conn)); // send next ping
                if (res == -1)
                {
                    LOGW("Error in sending UDP packet, \"%s\"\n",strerror(errno));
                }
                else if (res != sizeof(buf))
                {
                    LOGW("Error in sending UDP packet, wrong lenght %d\n", (int) res);
                }
            }
        }
    }

    if (m_slaveStatus != slaveOpen)
    {
        /* notification! for sure it has not been already sent */
        /* Send the notification to the clients that has requested to be informed */
        m_slaveStatus = slaveOpen;
        int a = activeConnections();
        unsigned char buf[4] = {'X', UDP_MUX_NUM_CLIENT, (a & 0xFF), ( (a>>8) & 0xFF) };
        sendPort(buf, sizeof(buf));
    }
}

int CUdpServer::findConnection (struct sockaddr_in si_other)
{
    int conn;
    // search this connection
    for(conn = 0; conn < MAXUDPCONN; conn++)
    {
        if ((m_udpConn[conn].conn.sin_port == si_other.sin_port) &&
            (m_udpConn[conn].conn.sin_addr.s_addr == si_other.sin_addr.s_addr))
            return conn;
    }

    /* failure... */
    return -1;
}

int CUdpServer::findEmptySlot (void)
{
    int conn;
    for(conn = 0; conn < MAXUDPCONN; conn++)
    {
        if (m_udpConn[conn].conn.sin_port == 0) {
            return conn;
        }
    }

    return -1;
}

int CUdpServer::activeConnections (void)
{
    int conn;
    int cnt = 0;
    for(conn = 0; conn < MAXUDPCONN; conn++)
    {
        if (m_udpConn[conn].conn.sin_port != 0) {
            cnt |= (0x01<<conn);
        }
    }

    if (m_slaveStatus)
    {
        cnt |= 0x8000;
    }

    return cnt;
}
