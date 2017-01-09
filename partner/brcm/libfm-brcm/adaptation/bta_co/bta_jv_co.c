/*****************************************************************************
**
**  Name:           bta_jv_co.c
**
**  Description:    This file contains the data java interfacefunction
**                  implementation for Insight.
**
**  Copyright (c) 2007, Widcomm Inc., All Rights Reserved.
**  Widcomm Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>

#include "bta_api.h"
#include "bta_sys.h"
#include "bta_jv_api.h"
#include "bta_jv_co.h"

#if 0
static void dump(void *buffer_in, int length)
{
    UINT16  xx, yy;
    UINT8   *p = buffer_in;
    char    buff1[100], buff2[20];

    APPL_TRACE_DEBUG2( "bta_jv_co_rfc_data at %x for %d", buffer_in, length);
    memset (buff2, ' ', 16);
    buff2[16] = 0;

    yy = sprintf (buff1, "%04x: ", 0);
    for (xx = 0; xx < length; xx++)
    {
        if ( (xx) && ((xx & 15) == 0) )
        {
            APPL_TRACE_DEBUG2 ("    %s  %s", buff1, buff2);
            yy = sprintf(buff1, "%04x: ", xx);
            memset (buff2, ' ', 16);
        }
        yy += sprintf (&buff1[yy], "%02x ", *p);

        if ((*p >= ' ') && (*p <= 'z'))
            buff2[xx & 15] = *p;
        else
            buff2[xx & 15] = '.';

        p++;
    }

    /* Pad out the remainder */
    for ( ; ; xx++)
    {
        if ((xx & 15) == 0)
        {
            APPL_TRACE_DEBUG2 ("    %s  %s", buff1, buff2);
            break;
        }
        yy += sprintf (&buff1[yy], "   ");
    }
}
#endif

/*******************************************************************************
**
** Function         bta_jv_co_rfc_data
**
** Description      This function is called by JV to send data to the java glue
**                  code when the RX data path is configured to use a call-out
**
** Returns          void
**
*******************************************************************************/
void bta_jv_co_rfc_data(UINT32 handle, UINT8 *p_data, UINT16 len)
{
    /* must copy p_data out in the glue code
     * the data buffer will be released when this function returns */
    /*
    dump(p_data, len);
    Send_SPP_readResult(0, 0);
    */

  printf("bta_jv_co_rfc_data FIXME\n");

}


