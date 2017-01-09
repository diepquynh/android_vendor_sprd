/*
 * Authors:<jinglong.chen@spreadtrum.com>
 * Owner:
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include "wlnpi.h"

#include <netlink-types.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <netlink/netlink.h>
#include <endian.h>
#include <linux/types.h>

#include <android/log.h>
#include <utils/Log.h>


#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG     "WLNPI"

#define ENG_LOG  ALOGD

#define WLNPI_RESULT_BUF_LEN        (256)

#define IWNPI_EXEC_TMP_FILE         ("/productinfo/iwnpi_exec_data.log")

#define WLAN_EID_SUPP_RATES         (1)
#define WLAN_EID_HT_CAP             (45)
#define WLAN_EID_VENDOR_SPECIFIC    (221)
#define WPS_IE_VENDOR_TYPE          (0x0050f204)

#define IWNPI_CONN_MODE_MANAGED     ("Managed")
#define IWNPI_CONN_MODE_OTHER       ("Other Mode")
#define IWNPI_DEFAULT_CHANSPEC      ("2.4G")
#define IWNPI_DEFAULT_BANDWIDTH     ("20MHz")

static int wlnpi_ap_info_print_capa(const char *data);
static int wlnpi_ap_info_print_support_rate(const char *data);
static char *wlnpi_get_rate_by_phy(int phy);
static char *wlnpi_bss_get_ie(const char *bss, char ieid);
static char *iwnpi_bss_get_vendor_ie(const char *bss, int vendor_type);
static int wlnpi_ap_info_print_ht_capa(const char *data);
static int wlnpi_ap_info_print_ht_mcs(const char *data);
static int wlnpi_ap_info_print_wps(const char *data);
static short iwnpi_get_be16(const char *a);
static short iwnpi_get_le16(const char *a);
static int iwnpi_get_be32(const char *a);
static int iwnpi_hex_dump(unsigned char *name, unsigned short nLen, unsigned char *pData, unsigned short len);

static iwnpi_rate_table g_rate_table[] =
{
    {0x82, "1[b]"},
    {0x84, "2[b]"},
    {0x8B, "5.5[b]"},
    {0x0C, "6"},
    {0x12, "9"},
    {0x96, "11[b]"},
    {0x18, "12"},
    {0x24, "18"},
    {0x30, "24"},
    {0x48, "36"},
    {0x60, "48"},
    {0x6C, "54"},

    /* 11g */
    {0x02, "1[b]"},
    {0x04, "2[b]"},
    {0x0B, "5.5[b]"},
    {0x16, "11[b]"},
};


int mac_addr_a2n(unsigned char *mac_addr, char *arg)
{
    int i;

    for (i = 0; i < ETH_ALEN; i++) 
    {
        int temp;
        char *cp = strchr(arg, ':');
        if (cp) {
            *cp = 0;
            cp++;
        }
        if (sscanf(arg, "%x", &temp) != 1)
            return -1;
        if (temp < 0 || temp > 255)
            return -1;

        mac_addr[i] = temp;
        if (!cp)
            break;
        arg = cp;
    }
    if (i < ETH_ALEN - 1)
        return -1;

    return 0;

}
int wlnpi_cmd_start(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    memcpy(s_buf,  &(g_wlnpi.mac[0]),  6 );
    *s_len = 6;
    return 0;   
}

int wlnpi_cmd_set_rate(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    if(1 != argc)
        return -1;

    *((unsigned int *)s_buf) = strtol(argv[0], NULL, 10);
	*s_len = 1;
	return 0;
}

int wlnpi_cmd_set_channel(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    if(1 != argc)
        return -1;

    if(*((unsigned int *)s_buf) = strtol(argv[0], NULL, 10))
	{
		*s_len = 1;
		return 0;
	}
	else
		return -1;
}

int wlnpi_cmd_set_tx_power(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    if(1 != argc)
        return -1;

    *((unsigned int *)s_buf) = strtol(argv[0], NULL, 10);
	*s_len = 1;
	return 0;
}
int wlnpi_cmd_set_tx_count(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    if(1 != argc)
        return -1;

	*((unsigned int *)s_buf) = strtol(argv[0], NULL, 10);
	*s_len = 4;
	return 0;
}

int wlnpi_cmd_set_wlan_cap(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    if(1 != argc)
        return -1;

   if(*s_buf = strtol(argv[0], NULL, 10))
   {
		*s_len = 4;
		return 0;
   }
   else
	   return -1;
}
int wlnpi_cmd_set_mac(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    if(1 != argc)
        return -1;
    mac_addr_a2n(s_buf, argv[0]);
    *s_len = 6;
    return 0;
}

int wlnpi_cmd_set_reg(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    unsigned char  *type  = s_buf;
    unsigned int   *addr  = (unsigned int *)(s_buf + 1);
    unsigned int   *value = (unsigned int *)(s_buf + 5);
	char *endp = NULL;
    if((argc < 2)|| (argc > 3) || (!argv) )
        return -1;
    if(0 == strncmp(argv[0], "mac", 3) )
    {
        *type = 0;
    }
    else if(0 == strncmp(argv[0], "phy0", 4) )
    {
        *type = 1;
    }
    else if(0 == strncmp(argv[0], "phy1", 4) )
    {
        *type = 2;
    }
    else if(0 == strncmp(argv[0], "rf",  2) )
    {
        *type = 3;
    }
    else
    {
        return -1;
    }

    *addr = strtol(argv[1], &endp, 16);
	if (*endp != '\0')
	{
		printf(" non hexadecimal character encountered \n");
		return -1;
	}

	endp = NULL;

	*value =  strtoul(argv[2], &endp, 16);

	if (*endp != '\0')
	{
		printf(" non hexadecimal character encountered \n");
		return -1;
	}

    *s_len = 9;
    return 0;
}

/********************************************************************
*   name   wlnpi_cmd_set_band 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_band(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        return -1;
    }

    *((unsigned int *)s_buf) = strtol(argv[0], NULL, 10);
	*s_len = 1;
	ENG_LOG("ADL leaving %s()", __func__);
	return 0;
}

/********************************************************************
*   name   wlnpi_cmd_set_bandwidth 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_bandwidth(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{

    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        return -1;
    }

	*((unsigned int *)s_buf) = strtol(argv[0], NULL, 10);
	*s_len = 1;

	ENG_LOG("ADL leaving %s()", __func__);
	return 0;

}

/********************************************************************
*   name   wlnpi_cmd_set_preamble 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_preamble(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        return -1;
    }

	*((unsigned int *)s_buf) = strtol(argv[0], NULL, 10);
	*s_len = 1;

	ENG_LOG("ADL leaving %s()", __func__);
	return 0;
}

/********************************************************************
*   name   wlnpi_cmd_set_payload 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_payload(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        return -1;
    }

	*((unsigned int *)s_buf) = strtol(argv[0], NULL, 10);
	*s_len = 1;

	ENG_LOG("ADL leaving %s()", __func__);
	return 0;
}

/********************************************************************
*   name   wlnpi_cmd_set_pkt_length 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_pkt_length(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);

    if(1 != argc)
    {
        return -1;
    }

    if(*((unsigned int *)s_buf) = strtol(argv[0], NULL, 10))
	{
	    *s_len = 2;

		ENG_LOG("ADL leaving %s()", __func__);

		return 0;
	}
	else
		return -1;
}

/********************************************************************
*   name   wlnpi_cmd_set_guard_interval 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_guard_interval(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        ENG_LOG("ADL %s(), argc is ERROR, return -1", __func__);
        return -1;
    }
    
	*((unsigned int *)s_buf) = strtol(argv[0], NULL, 10);
	*s_len = 1;

	ENG_LOG("ADL leaving %s()", __func__);
	return 0;
}

/********************************************************************
*   name   wlnpi_cmd_set_tx_mode 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_tx_mode(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        ENG_LOG("ADL %s(), argc is ERROR, return -1", __func__);
        return -1;
    }
    
    if(*((unsigned int *)s_buf) = strtol(argv[0], NULL, 10))
	{
		*s_len = 1;

		ENG_LOG("ADL leaving %s(), s_buf[0] = %d, s_len = %d", __func__, s_buf[0], *s_len);
		return 0;
	}
	else
		return -1;
}

/********************************************************************
*   name   wlnpi_cmd_set_macfilter 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_macfilter(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    ENG_LOG("ADL entry %s(), argc = %d, argv[0] = %s", __func__, argc, argv[0]);
    if(1 != argc)
    {
        ENG_LOG("ADL %s(), argc is ERROR, return -1", __func__);
        return -1;
    }

    if(*((unsigned int *)s_buf) = strtol(argv[0], NULL, 10))
	{
		*s_len = 1;

		ENG_LOG("ADL leaving %s(), s_buf[0] = %d, s_len = %d", __func__, s_buf[0], *s_len);
	    return 0;
	}
	else
		return -1;
}

int wlnpi_cmd_get_reg(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    unsigned char  *type  = s_buf;
    unsigned int   *addr  = (unsigned int *)(s_buf + 1);
    unsigned int   *count = (unsigned int *)(s_buf + 5);
	char *endp = NULL;

    if((argc < 2)|| (argc > 3) || (!argv) )
        return -1;
    if(0 == strncmp(argv[0], "mac", 3) )
    {
        *type = 0;
    }
    else if(0 == strncmp(argv[0], "phy0", 4) )
    {
        *type = 1;
    }
    else if(0 == strncmp(argv[0], "phy1", 4) )
    {
        *type = 2;
    }
    else if(0 == strncmp(argv[0], "rf",  2) )
    {
        *type = 3;
    }
    else
    {
        return -1;
    }

    *addr = strtol(argv[1], &endp, 16);

	if(*endp != '\0')
	{
		printf((" non hexadecimal character encountered \n"));
		return -1;
	}

    ENG_LOG("ADL %s(), argv[2] addr = %p", __func__, argv[2]);
    if (NULL == argv[2] || 0 == strlen(argv[2]))
    {
        /* if argv[2] is NULL or null string, set count to default value, which value is 1 */
        ENG_LOG("ADL %s(), argv[2] is null, set count to 1", __func__);
        *count = 1;
    }
    else
        *count = strtol(argv[2], NULL, 10);

	if (*count >= 5)
    {
        ENG_LOG("ADL %s(), *count is too large, *count = %d, set count to 5", __func__, *count);
        *count = 5;
    }
    ENG_LOG("ADL %s(), *count = %d", __func__, *count);

    *s_len = 9;
    return 0;
}

/********************************************************************
*   name   wlnpi_cmd_set_ar
*   ---------------------------
*   description: set autorate's flag and index
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_ar(int argc, char **argv, unsigned char *s_buf, int *s_len )
{
    unsigned char   *ar_flag  = (unsigned char *)(s_buf + 0);
    unsigned char   *ar_index = (unsigned char *)(s_buf + 1);

    ENG_LOG("ADL entry %s(), argc = %d", __func__, argc);

    if((argc < 1)|| (argc > 3) || (!argv))
    {
        ENG_LOG("ADL leaving %s(), argc ERR.", __func__);
        return -1;
    }

    *ar_flag = strtoul(argv[0], NULL, 10);

    if (1 == *ar_flag && argc)
    {
        *ar_index = strtoul(argv[1], NULL, 10);
    }
    else if (0 == *ar_flag)
    {
        *ar_index = 0;
    }
    else
    {
        ENG_LOG("ADL leaving %s(), ar_flag is invalid. *ar_flag = %d", __func__, *ar_flag);
        return -1;
    }

    *s_len = 2;

    ENG_LOG("ADL leaving %s(), ar_flag = %d, ar_index = %d, *s_len = %d", __func__, *ar_flag, *ar_index, *s_len);
    return 0;
}

/********************************************************************
*   name   wlnpi_cmd_set_ar_pktcnt
*   ---------------------------
*   description: set autorate's pktcnt
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_ar_pktcnt(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    ENG_LOG("ADL entry %s(), argc = %d", __func__, argc);

    if(1 != argc)
    {
        ENG_LOG("ADL leaving %s(), argc error. return -1", __func__);
        return -1;
    }

    *((unsigned int *)s_buf) = strtoul(argv[0], NULL, 10);

    *s_len = 4;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

/********************************************************************
*   name   wlnpi_cmd_set_ar_retcnt
*   ---------------------------
*   description: set autorate's retcnt
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_cmd_set_ar_retcnt(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    ENG_LOG("ADL entry %s(), argc = %d", __func__, argc);

    if(1 != argc)
    {
        ENG_LOG("ADL leaving %s(), argc error. return -1", __func__);
        return -1;
    }

    *((unsigned int *)s_buf) = strtoul(argv[0], NULL, 10);

    *s_len = 4;

    ENG_LOG("ADL leaving %s()", __func__);
    return 0;
}

int wlnpi_show_get_channel(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{   
    unsigned char ch = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_channel err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);
        return -1;
    }

    ch =  *( (unsigned char *)(r_buf+0) );
    printf("ret: %d :end\n", ch);

    ENG_LOG("ADL leaving %s(), ch = %d, return 0", __func__, ch);

    return 0;
}

int wlnpi_show_get_rx_ok(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{   
    FILE *fp = NULL;
    char ret_buf[WLNPI_RESULT_BUF_LEN] = {0x00};
    unsigned int rx_end_count = 0;
    unsigned int rx_err_end_count = 0;
    unsigned int fcs_faiil_count = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(12 != r_len)
    {
        printf("get_rx_ok err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    rx_end_count = *((unsigned int *)(r_buf+0));
    rx_err_end_count = *((unsigned int *)(r_buf+4));
    fcs_faiil_count = *((unsigned int *)(r_buf+8));

    snprintf(ret_buf, WLNPI_RESULT_BUF_LEN, "ret: reg value: rx_end_count=%d rx_err_end_count=%d fcs_fail_count=%d :end\n", rx_end_count, rx_err_end_count, fcs_faiil_count);

    if(NULL != (fp = fopen(IWNPI_EXEC_TMP_FILE, "w+")))
    {
        int write_cnt = 0;

        write_cnt = fputs(ret_buf, fp);
        ENG_LOG("ADL %s(), callED fputs(%s), write_cnt = %d", __func__, ret_buf, write_cnt);

        fclose(fp);
    }

    printf("%s", ret_buf);
    ENG_LOG("ADL leaving %s(), rx_end_count=%d rx_err_end_count=%d fcs_fail_count=%d, return 0", __func__, rx_end_count, rx_err_end_count, fcs_faiil_count);

    return 0;
}

int wlnpi_show_get_rssi(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    char rssi = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_rssi err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    rssi = *( (char *)r_buf );
    printf("ret: %d:end\n", rssi );

    ENG_LOG("ADL leaving %s(), rssi = %d, return 0", __func__, rssi);

    return 0;
}
int wlnpi_show_get_rate(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char rate = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_rate err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    rate = *( (unsigned char *)r_buf );
    printf("ret: %d :end\n", rate);

    ENG_LOG("ADL leaving %s(), rate = %d, return 0", __func__, rate);

    return 0;
}

int wlnpi_show_get_tx_power(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char level_a,level_b;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(2 != r_len)
    {
        printf("get_tx_power err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    level_a = r_buf[0];
    level_b = r_buf[2];
    printf("ret: level_a:%d,level_b:%d:end",  level_a, level_b );

    ENG_LOG("ADL leaving %s(), level_a = %d, level_b = %d, return 0", __func__, level_a, level_b);

    return 0;   
}

int wlnpi_show_get_lna_status(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    FILE *fp = NULL;
    char ret_buf[WLNPI_RESULT_BUF_LEN+1] = {0x00};
    unsigned char status = 0;

    ENG_LOG("ADL entry %s(), r_eln = %d", __func__, r_len);
    status = r_buf[0];

    snprintf(ret_buf, WLNPI_RESULT_BUF_LEN, "ret: %d :end\n", status);
    printf("%s", ret_buf);

    ENG_LOG("ADL %s(), line = %d", __func__, __LINE__);
    if(NULL != (fp = fopen(IWNPI_EXEC_TMP_FILE, "w+")))
    {
        int write_cnt = 0;

        write_cnt = fputs(ret_buf, fp);
        ENG_LOG("ADL %s(), callED fputs(%s), write_cnt = %d", __func__, ret_buf, write_cnt);

        fclose(fp);
    }

    ENG_LOG("ADL leaving %s(), status = %d", __func__, status);
    return 0;   
}

int wlnpi_show_get_wlan_cap(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int cap = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(4 != r_len)
    {
        printf("get_rssi err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    cap = *(unsigned int *)r_buf;
    printf("ret: %d:end\n", cap );

    ENG_LOG("ADL leaving %s(), cap = %d, return 0", __func__, cap);

    return 0;
}
#define WLNPI_GET_REG_MAX_COUNT            (20)
int wlnpi_show_get_reg(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    int i, ret, p;
    FILE *fp = NULL;
    unsigned char str[256] = {0};
    char ret_buf[WLNPI_RESULT_BUF_LEN] = {0x00};

	ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    for(i=0, p =0; (i < r_len/4) && (i < WLNPI_GET_REG_MAX_COUNT); i++)
    {
        if (!(i % 4) && i) {
			ret = sprintf( (str +  p), "\n");
			p = p + ret;
		}
		ret = sprintf((str +  p),  "0x%08X\t",  *((int *)(r_buf + i*4)));

        p = p + ret;
    }

    snprintf(ret_buf, WLNPI_RESULT_BUF_LEN, "reg values is :%s\n", str);

    if(NULL != (fp = fopen(IWNPI_EXEC_TMP_FILE, "w+")))
    {
        int write_cnt = 0;

        write_cnt = fputs(ret_buf, fp);
        ENG_LOG("ADL %s(), callED fputs(%s), write_cnt = %d", __func__, ret_buf, write_cnt);

        fclose(fp);
    }

    printf("%s", ret_buf);

    ENG_LOG("ADL leaving %s(), str = %s, return 0", __func__, str);

    return 0;
}

int wlnpi_show_get_mac(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    int i, ret, p;

    ENG_LOG("ADL entry %s(), r_eln = %d", __func__);

    printf("ret: mac: %02x:%02x:%02x:%02x:%02x:%02x :end\n", r_buf[0], r_buf[1], r_buf[2],r_buf[3],r_buf[4], r_buf[5]);

    ENG_LOG("ADL leaving %s(), mac = %02x:%02x:%02x:%02x:%02x:%02x, return 0", __func__, r_buf[0], r_buf[1], r_buf[2],r_buf[3],r_buf[4], r_buf[5]);

    return 0;   
}

/********************************************************************
*   name   wlnpi_show_get_band 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_band(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int band = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_band err \n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    band = *( (unsigned char *)(r_buf+0) ) ;
    printf("ret: %d :end\n", band);

    ENG_LOG("ADL leaving %s(), band = %d, return 0", __func__, band);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_bandwidth 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_bandwidth(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int bandwidth = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_bandwidth err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    bandwidth = *( (unsigned char *)(r_buf+0) ) ;
    printf("ret: %d :end\n", bandwidth);

    ENG_LOG("ADL leaving %s(), bandwidth = %d, return 0", __func__, bandwidth);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_payload 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_payload(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int result = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_payload err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    result = *( (unsigned char *)(r_buf+0) ) ;
    printf("ret: %d :end\n", result);

    ENG_LOG("ADL leaving %s(), result = %d, return 0", __func__, result);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_pkt_length 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_pkt_length(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned short pktlen = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(2 != r_len)
    {
        printf("get_pkt_length err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    pktlen = *( (unsigned short *)(r_buf+0) ) ;
    printf("ret: %d :end\n", pktlen);

    ENG_LOG("ADL leaving %s(), pktlen = %d, return 0", __func__, pktlen);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_guard_interval 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_guard_interval(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char gi = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_payload err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    gi = *( (unsigned char *)(r_buf+0) ) ;
    printf("ret: %d :end\n", gi);

    ENG_LOG("ADL leaving %s(), gi = %d, return 0", __func__, gi);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_tx_mode 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_tx_mode(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char tx_mode = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get_tx_mode err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    tx_mode = *( (unsigned char *)(r_buf+0) ) ;
    printf("ret: %d :end\n", tx_mode);

    ENG_LOG("ADL leaving %s(), tx_mode = %d, return 0", __func__, tx_mode);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_macfilter 
*   ---------------------------
*   description: 
*   ----------------------------
*   para        IN/OUT      type            note        
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_macfilter(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{   
    unsigned char macfilter = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);
    
    if(1 != r_len)
    {
        printf("get_macfilter err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    macfilter = *( (unsigned char *)(r_buf+0) ) ;
    printf("ret: %d :end\n", macfilter);

    ENG_LOG("ADL leaving %s(), macfilter = %d, return 0", __func__, macfilter);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_mcs_index
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_mcs_index(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int mcs_index = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(1 != r_len)
    {
        printf("get msc index err \n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);

        return -1;
    }

    mcs_index = *( (unsigned char *)(r_buf+0) ) ;
    printf("mcs index: %d\n", mcs_index);

    ENG_LOG("ADL leaving %s(), mcs_index = %d, return 0", __func__, mcs_index);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_ar
*   ---------------------------
*   description: set autorate's flag and autorate index
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_ar(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned char ar_flag = 0;
    unsigned char ar_index = 0;

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    ar_flag  = *(unsigned char *)(r_buf + 0);
    ar_index = *(unsigned char *)(r_buf + 1);

    printf("ret: %d, %d :end\n", ar_flag, ar_index);

    ENG_LOG("ADL leaving %s(), ar_flag = %d, ar_index= %d, return 0", __func__, ar_flag, ar_index);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_ar_pktcnt
*   ---------------------------
*   description: get autorate's pktcnt
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_ar_pktcnt(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int pktcnt = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(4 != r_len)
    {
        printf("get_ar_pktcnt err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);
        return -1;
    }

    pktcnt =  *((unsigned int *)(r_buf+0));
    printf("ret: %d :end\n", pktcnt);

    ENG_LOG("ADL leaving %s(), pktcnt = %d, return 0", __func__, pktcnt);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_ar_retcnt
*   ---------------------------
*   description: get autorate's retcnt
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_ar_retcnt(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    unsigned int retcnt = 0;
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(4 != r_len)
    {
        printf("get_ar_retcnt err\n");
        ENG_LOG("ADL leaving %s(), r_len is ERROR, return -1", __func__);
        return -1;
    }

    retcnt = *((unsigned int *)(r_buf+0));
    printf("ret: %d :end\n", retcnt);

    ENG_LOG("ADL leaving %s(), retcnt = %d, return 0", __func__, retcnt);

    return 0;
}

/********************************************************************
*   name   wlnpi_show_get_connected_ap_info
*   ---------------------------
*   description: get connected ap info
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
int wlnpi_show_get_connected_ap_info(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    assoc_resp resp_ies = {0x00};

    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    //iwnpi_hex_dump((unsigned char *)"r_buf:\n", strlen("r_buf:\n"), (unsigned char *)r_buf, r_len);

    if (NULL != r_buf)
    {
        memcpy(&resp_ies, r_buf, r_len);
    }
    else
    {
        ENG_LOG("ADL levaling %s(), resp_ies == NULL!!!", __func__);
        return -1;
    }

    if (!resp_ies.connect_status)
    {
        printf("not connected AP.\n");
        ENG_LOG("ADL levaling %s(), connect_status = %d", __func__, resp_ies.connect_status);

        return 0;
    }

    printf("Current Connected Ap info: \n");

    /* SSID */
    printf("SSID: %s\n", resp_ies.ssid);

    /* connect mode */
    printf("Mode: %s\t", (resp_ies.conn_mode ? IWNPI_CONN_MODE_OTHER : IWNPI_CONN_MODE_MANAGED));

    /* RSSI */
    printf("RSSI: %d dBm\t", resp_ies.rssi);

    /* SNR */
    printf("SNR: %d dB\t", resp_ies.snr);

    /* noise */
    printf("noise: %d dBm\n", resp_ies.noise);

    /* Flags: FromBcn RSSI on-channel Channel */
    printf("Flags: FromBcn RSSI on-channel Channel: %d\n", resp_ies.channel);

    /* BSSID */
    printf("BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n", resp_ies.bssid[0], resp_ies.bssid[1], resp_ies.bssid[2], resp_ies.bssid[3], resp_ies.bssid[4], resp_ies.bssid[5]);

    /* capability */
    wlnpi_ap_info_print_capa(resp_ies.assoc_resp_info);

    /* Support Rates */
    wlnpi_ap_info_print_support_rate(resp_ies.assoc_resp_info);

    /* HT Capable: */
    {
        printf("\n");
        printf("HT Capable:\n");
        printf("Chanspec: %s\t", IWNPI_DEFAULT_CHANSPEC);
        printf("Channel: %d Primary channel: %d\t", resp_ies.channel, resp_ies.channel);
        printf("BandWidth: %s\n", IWNPI_DEFAULT_BANDWIDTH);

        /* HT Capabilities */
        {
            wlnpi_ap_info_print_ht_capa(resp_ies.assoc_resp_info);
            wlnpi_ap_info_print_ht_mcs(resp_ies.assoc_resp_info);
        }
    }

    /* wps */
    wlnpi_ap_info_print_wps(resp_ies.assoc_resp_info);

    printf("\n");

    ENG_LOG("ADL leaving %s(), return 0", __func__);

    return 0;
}

/********************************************************************
*   name   wlnpi_ap_info_print_capa
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wlnpi_ap_info_print_capa(const char *data)
{
    short capability = data[2];

    ENG_LOG("ADL entry %s()", __func__);

    capability = iwnpi_get_le16(&data[2]);
    ENG_LOG("ADL %s(), capability = 0x%x", __func__, capability);

    printf("\n");
    printf("Capability:\n");
    if (capability & 1)
    {
        printf("ESS Type Network.\t");
    }
    else if (capability >> 1 & 1)
    {
        printf("IBSS Type Network.\t");
    }

    if (capability >> 2 & 1)
    {
        printf("CF Pollable.\t");
    }

    if (capability >> 3 & 1)
    {
        printf("CF Poll Requested.\t");
    }

    if (capability >> 4 & 1)
    {
        printf("Privacy Enabled.\t");
    }

    if (capability >> 5 & 1)
    {
        printf("Short Preamble.\t");
    }

    if (capability >> 6 & 1)
    {
        printf("PBCC Allowed.\t");
    }

    if (capability >> 7 & 1)
    {
        printf("Channel Agility Used.\t");
    }

    if (capability >> 8 & 1)
    {
        printf("Spectrum Mgmt Enabled.\t");
    }

    if (capability >> 9 & 1)
    {
        printf("QoS Supported.\t");
    }

    if (capability >> 10 & 1)
    {
        printf("G Mode Short Slot Time.\t");
    }

    if (capability >> 11 & 1)
    {
        printf("APSD Supported.\t");
    }

    if (capability >> 12 & 1)
    {
        printf("Radio Measurement.\t");
    }

    if (capability >> 13 & 1)
    {
        printf("DSSS-OFDM Allowed.\t");
    }

    if (capability >> 14 & 1)
    {
        printf("Delayed Block Ack Allowed.\t");
    }

    if (capability >> 15 & 1)
    {
        printf("Immediate Block Ack Allowed.\t");
    }

    printf("\n");
    ENG_LOG("ADL levaling %s()", __func__);
    return 0;
}

/********************************************************************
*   name   wlnpi_ap_info_print_wps
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wlnpi_ap_info_print_wps(const char *data)
{
    char    *vendor_ie = NULL;
    char    wps_version = 0;
    char    wifi_protected_setup = 0;

    ENG_LOG("ADL entry %s()", __func__);

    vendor_ie = iwnpi_bss_get_vendor_ie(data, WPS_IE_VENDOR_TYPE);
    if (NULL == vendor_ie)
    {
        ENG_LOG("ADL %s(), get vendor failed. return 0", __func__);
        return 0;
    }

    //iwnpi_hex_dump("vendor:", strlen("vendor:"), (unsigned char *)vendor_ie, 0x16);

    ENG_LOG("ADL %s(), length = %d\n", __func__, vendor_ie[1]);
    wps_version = vendor_ie[10];
    wifi_protected_setup = vendor_ie[15];

    printf("\nWPS:\t");
    printf("0x%02x \t", wps_version);

    if (2 == wifi_protected_setup)
    {
        printf("Configured.");
    }
    else if (3 == wifi_protected_setup)
    {
        printf("AP.");
    }

    printf("\n");

    ENG_LOG("ADL levaling %s()", __func__);
    return 0;
}

/********************************************************************
*   name   wlnpi_ap_info_print_ht_mcs
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wlnpi_ap_info_print_ht_mcs(const char *data)
{
    char    *ht_capa_ie = NULL;
    char    spatial_stream1 = 0;
    char    spatial_stream2 = 0;
    char    spatial_stream3 = 0;
    char    spatial_stream4 = 0;

    ht_capa_ie = wlnpi_bss_get_ie(data, WLAN_EID_HT_CAP);
    if (NULL == ht_capa_ie)
    {
        printf("error. get mcs failed.\n");
        ENG_LOG("ADL %s(), get mcs failed. return -1", __func__);

        return -1;
    }

    //iwnpi_hex_dump("ht_capability:", strlen("ht_capability:"), (unsigned char *)ht_capa_ie, 0x16);

    spatial_stream1 = ht_capa_ie[5];
    spatial_stream2 = ht_capa_ie[6];
    spatial_stream3 = ht_capa_ie[7];
    spatial_stream4 = ht_capa_ie[8];

    //printf("spatial1 = 0x%x, spatial2 = 0x%x\n", spatial_stream1, spatial_stream2);

    printf("\nSupported MCS:\n");

    if (spatial_stream1 >> 0 & 1)
    {
        printf("0 ");
    }

    if (spatial_stream1 >> 1 & 1)
    {
        printf("1 ");
    }

    if (spatial_stream1 >> 2 & 1)
    {
        printf("2 ");
    }

    if (spatial_stream1 >> 3 & 1)
    {
        printf("3 ");
    }

    if (spatial_stream1 >> 4 & 1)
    {
        printf("4 ");
    }

    if (spatial_stream1 >> 5 & 1)
    {
        printf("5 ");
    }

    if (spatial_stream1 >> 6 & 1)
    {
        printf("6 ");
    }

    if (spatial_stream1 >> 7 & 1)
    {
        printf("7");
    }

    /* spatial2 */
    if (spatial_stream2 >> 0 & 1)
    {
        printf("8 ");
    }

    if (spatial_stream2 >> 1 & 1)
    {
        printf("9 ");
    }

    if (spatial_stream2 >> 2 & 1)
    {
        printf("10 ");
    }

    if (spatial_stream2 >> 3 & 1)
    {
        printf("11 ");
    }

    if (spatial_stream2 >> 4 & 1)
    {
        printf("12 ");
    }

    if (spatial_stream2 >> 5 & 1)
    {
        printf("13 ");
    }

    if (spatial_stream2 >> 6 & 1)
    {
        printf("14 ");
    }

    if (spatial_stream2 >> 7 & 1)
    {
        printf("15");
    }

    /* spatial3 */
    if (spatial_stream3 >> 0 & 1)
    {
        printf("16 ");
    }

    if (spatial_stream3 >> 1 & 1)
    {
        printf("17 ");
    }

    if (spatial_stream3 >> 2 & 1)
    {
        printf("18 ");
    }

    if (spatial_stream3 >> 3 & 1)
    {
        printf("19 ");
    }

    if (spatial_stream3 >> 4 & 1)
    {
        printf("20 ");
    }

    if (spatial_stream3 >> 5 & 1)
    {
        printf("21 ");
    }

    if (spatial_stream3 >> 6 & 1)
    {
        printf("22 ");
    }

    if (spatial_stream3 >> 7 & 1)
    {
        printf("23");
    }

    /* spatial4 */
    if (spatial_stream4 >> 0 & 1)
    {
        printf("24 ");
    }

    if (spatial_stream4 >> 1 & 1)
    {
        printf("25 ");
    }

    if (spatial_stream4 >> 2 & 1)
    {
        printf("26 ");
    }

    if (spatial_stream4 >> 3 & 1)
    {
        printf("27 ");
    }

    if (spatial_stream4 >> 4 & 1)
    {
        printf("28 ");
    }

    if (spatial_stream4 >> 5 & 1)
    {
        printf("29 ");
    }

    if (spatial_stream4 >> 6 & 1)
    {
        printf("30 ");
    }

    if (spatial_stream4 >> 7 & 1)
    {
        printf("31 ");
    }

    printf("\n");

    return 0;
}

/********************************************************************
*   name   wlnpi_ap_info_print_ht_capa
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wlnpi_ap_info_print_ht_capa(const char *data)
{
    char    *ht_capa_ie = NULL;
    short   ht_capa = 0;

    ENG_LOG("ADL entry %s()", __func__);

    ht_capa_ie = wlnpi_bss_get_ie(data, WLAN_EID_HT_CAP);
    if (NULL == ht_capa_ie)
    {
        printf("error. get support_rate failed.\n");
        ENG_LOG("ADL %s(), get support_rate failed. return -1", __func__);

        return -1;
    }

    //iwnpi_hex_dump("ht_capability:", strlen("ht_capability:"), (unsigned char *)ht_capa_ie, 16);

    ENG_LOG("ADL %s(), len = %d\n", __func__, ht_capa_ie[1]);

    ht_capa = iwnpi_get_le16(&ht_capa_ie[2]);

    ENG_LOG("ADL %s(), HT Capabilities = 0x%x\n", __func__, ht_capa);

    printf("\n");
    printf("HT Capabilities:\n");

    if (ht_capa >> 0 & 1)
    {
        printf("LDPC Coding Capability.\n");
    }

    if (ht_capa >> 1 & 1)
    {
        printf("20MHz and 40MHz Operation is Supported.\n");
    }

    if (ht_capa >> 2 & 3)
    {
        printf("Spatial Multiplexing Enabled.\n");
    }

    if (ht_capa >> 4 & 1)
    {
        printf("Can receive PPDUs with HT-Greenfield format.\n");
    }

    if (ht_capa >> 5 & 1)
    {
        printf("Short GI for 20MHz.\n");
    }

    if (ht_capa >> 6 & 1)
    {
        printf("Short GI for 40MHz.\n");
    }

    if (ht_capa >> 7 & 1)
    {
        printf("Transmitter Support Tx STBC.\n");
    }

    if (ht_capa >> 8 & 3)/*2 bit*/
    {
        printf("Rx STBC Support.\n");
    }

    if (ht_capa >> 10 & 1)
    {
        printf("Support HT-Delayed BlockAck Operation.\n");
    }

    if (ht_capa >> 11 & 1)
    {
        printf("Maximal A-MSDU size.\n");
    }

    if (ht_capa >> 12 & 1)
    {
        printf("BSS Allow use of DSSS/CCK Rates @40MHz\n");
    }

    if (ht_capa >> 13 & 1)
    {
        printf("Device/BSS Support use of PSMP.\n");
    }

    if (ht_capa >> 14 & 1)
    {
        printf("AP allow use of 40MHz Transmissions In Neighboring BSSs.\n");
    }

    if (ht_capa >> 15 & 1)
    {
        printf("L-SIG TXOP Protection Support.\n");
    }

    ENG_LOG("ADL leval %s()", __func__);
    return 0;
}

/********************************************************************
*   name   wlnpi_ap_info_print_support_rate
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int wlnpi_ap_info_print_support_rate(const char *data)
{
    char i = 0;
    char length = 0;
    char *support_rate_ie = NULL;

    ENG_LOG("ADL entry %s()", __func__);

    //iwnpi_hex_dump("data:", strlen("data:"), (unsigned char *)data, 100);
    support_rate_ie = wlnpi_bss_get_ie(data, WLAN_EID_SUPP_RATES);
    if (NULL == support_rate_ie)
    {
        printf("error. get support_rate failed.\n");
        ENG_LOG("ADL %s(), get support_rate failed. return -1", __func__);

        return -1;
    }

    //iwnpi_hex_dump("support rate:", strlen("support rate:"), (unsigned char *)support_rate_ie, 10);

    length = support_rate_ie[1];
    ENG_LOG("ADL %s(), length = %d\n", __func__, length);

    printf("\n");
    printf("Support Rates:\n");

    while (i < length)
    {
        printf("%s  ", wlnpi_get_rate_by_phy(support_rate_ie[2 + i]));
        i++;
    }

    printf("\n");

    ENG_LOG("ADL levaling %s()", __func__);

    return 0;
}

/********************************************************************
*   name   wlnpi_get_rate_by_phy
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static char *wlnpi_get_rate_by_phy(int phy)
{
    unsigned char i = 0;
    char id_num = sizeof(g_rate_table) / sizeof(iwnpi_rate_table);

    for (i = 0; i < id_num; i++)
    {
        if (phy == g_rate_table[i].phy_rate)
        {
            ENG_LOG("\nADL %s(), rate: phy = 0x%2x, str = %s\n", __func__, phy, g_rate_table[i].str_rate);
            return g_rate_table[i].str_rate;
        }
    }

    ENG_LOG("ADL %s(), not match rate, phy = 0x%x", __func__, phy);
    return "";
}

/********************************************************************
*   name   iwnpi_get_be16
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static short iwnpi_get_be16(const char *a)
{
    return (a[0] << 8) | a[1];
}

/********************************************************************
*   name   iwnpi_get_le16
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static short iwnpi_get_le16(const char *a)
{
    return (a[1] << 8) | a[0];
}

/********************************************************************
*   name   iwnpi_get_be32
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int iwnpi_get_be32(const char *a)
{
    return (a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];
}

/********************************************************************
*   name   wlnpi_bss_get_ie
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static char *wlnpi_bss_get_ie(const char *bss, char ieid)
{
    short ie_len = 0;
    const char *end, *pos;

    /* get length of ies */
    ie_len = bss[0] + bss[1];
    ENG_LOG("%s(), ie_len = %d\n", __func__, ie_len);

    pos = (const char *) (bss + 2 + 6);/* 6 is capability's length + status code length + AID length */
    end = pos + ie_len - 6;

    while (pos + 1 < end)
    {
        if (pos + 2 + pos[1] > end)
        {
            break;
        }

        if (pos[0] == ieid)
        {
            return pos;
        }

        pos += 2 + pos[1];
    }

    return NULL;
}

/********************************************************************
*   name   iwnpi_bss_get_vendor_ie
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static char *iwnpi_bss_get_vendor_ie(const char *bss, int vendor_type)
{
    short ie_len = 0;
    const char *end, *pos;

    /* get length of ies */
    ie_len = bss[0] + bss[1];

    pos = (const char *)(bss + 2+6);
    end = pos + ie_len - 6;

    while (pos + 1 < end)
    {
        if (pos + 2 + pos[1] > end)
        {
            break;
        }

        if (pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 && vendor_type == iwnpi_get_be32(&pos[2]))
        {
            return pos;
        }
        pos += 2 + pos[1];
    }

    return NULL;
}

int wlnpi_cmd_no_argv(int argc, char **argv,  unsigned char *s_buf, int *s_len )
{
    if(argc > 0)
        return -1;
    *s_len = 0;
    return 0;
}

int wlnpi_show_only_status(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    return 0;
}

int wlnpi_show_only_int_ret(struct wlnpi_cmd_t *cmd, unsigned char *r_buf, int r_len)
{
    ENG_LOG("ADL entry %s(), r_len = %d", __func__, r_len);

    if(4 != r_len)
    {
        printf("%s() err\n", __func__);
        return -1;
    }
    printf("ret: %d :end\n",  *( (unsigned int *)(r_buf+0) )  );

    ENG_LOG("ADL leaving %s(), return 0", __func__);

    return 0;
}


struct wlnpi_cmd_t g_cmd_table[] = 
{
    {
        .id    = WLNPI_CMD_GET_LNA_STATUS,
        .name  = "lna_status",
        .help  = "lna_status",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_lna_status,
    },
    {
        .id    = WLNPI_CMD_LNA_OFF,
        .name  = "lna_off",
        .help  = "lna_off",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_LNA_ON,
        .name  = "lna_on",
        .help  = "lna_on",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_GET_REG,
        .name  = "get_reg",
        .help  = "get_reg <mac/phy0/phy1/rf> <addr_offset> [count]",
        .parse = wlnpi_cmd_get_reg,
        .show  = wlnpi_show_get_reg,
    },
    {
        .id    = WLNPI_CMD_GET_MAC,
        .name  = "get_mac",
        .help  = "get_mac",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_mac,
    },
    {
        .id    = WLNPI_CMD_GET_TX_POWER,
        .name  = "get_tx_power",
        .help  = "get_tx_power",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_tx_power,
    },
    {
        .id    = WLNPI_CMD_GET_RATE,
        .name  = "get_rate",
        .help  = "get_rate",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_rate,
    },
    {
        .id    = WLNPI_CMD_GET_RSSI,
        .name  = "get_rssi",
        .help  = "get_rssi",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_rssi,
    },
    {
        .id    = WLNPI_CMD_GET_RX_OK_COUNT,
        .name  = "get_rx_ok",
        .help  = "get_rx_ok",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_rx_ok,
    },
    {
        .id    = WLNPI_CMD_GET_CHANNEL,
        .name  = "get_channel",
        .help  = "get_channel",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_channel,
    },  
    {
        .id    = WLNPI_CMD_SET_REG,
        .name  = "set_reg",
        .help  = "set_reg <mac/phy0/phy1/rf> <addr_offset> <value>",
        .parse = wlnpi_cmd_set_reg,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_SET_MAC,
        .name  = "set_mac",
        .help  = "set_mac <xx:xx:xx:xx:xx:xx>",
        .parse = wlnpi_cmd_set_mac,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_SET_TX_COUNT,
        .name  = "set_tx_count",
        .help  = "set_tx_count <NUM>",
        .parse = wlnpi_cmd_set_tx_count,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_SET_TX_POWER,
        .name  = "set_tx_power",
        .help  = "set_tx_power <NUM>",
        .parse = wlnpi_cmd_set_tx_power,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_SET_CHANNEL,
        .name  = "set_channel",
        .help  = "set_channel <NUM>",
        .parse = wlnpi_cmd_set_channel,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_SET_RATE,
        .name  = "set_rate",
        .help  = "set_rate <NUM>",
        .parse = wlnpi_cmd_set_rate,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_SIN_WAVE,
        .name  = "sin_wave",
        .help  = "wlnpi sin_wave",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_RX_STOP,
        .name  = "rx_stop",
        .help  = "rx_stop",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_RX_START,
        .name  = "rx_start",
        .help  = "rx_start",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_TX_STOP,
        .name  = "tx_stop",
        .help  = "tx_stop",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_TX_START,
        .name  = "tx_start",
        .help  = "tx_start",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_STOP,
        .name  = "stop",
        .help  = "stop",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_only_status,
    },  
    {
        .id    = WLNPI_CMD_START,
        .name  = "start",
        .help  = "start",
        .parse = wlnpi_cmd_start,
        .show  = wlnpi_show_only_status,
    },
    {
        /* set_band */
        .id    = WLNPI_CMD_SET_BAND,
        .name  = "set_band",
        .help  = "set_band <NUM>",
        .parse = wlnpi_cmd_set_band,
        .show  = wlnpi_show_only_status,
    },
    {
        /* get_band */
        .id    = WLNPI_CMD_GET_BAND,
        .name  = "get_band",
        .help  = "get_band",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_band,
    },  

    {
        /* set_bandwidth */
        .id    = WLNPI_CMD_SET_BW,
        .name  = "set_bandwidth",
        .help  = "set_bandwidth <NUM>",
        .parse = wlnpi_cmd_set_bandwidth,
        .show  = wlnpi_show_only_status,
    },
    {
        /* get_bandwidth */
        .id    = WLNPI_CMD_GET_BW,
        .name  = "get_bandwidth",
        .help  = "get_bandwidth",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_bandwidth,
    },  

    {
        /* set_preamble */
        .id    = WLNPI_CMD_SET_PREAMBLE,
        .name  = "set_preamble",
        .help  = "set_preamble <NUM>",
        .parse = wlnpi_cmd_set_preamble,
        .show  = wlnpi_show_only_status,
    },

    {
        /* set_payload */
        .id    = WLNPI_CMD_SET_PAYLOAD,
        .name  = "set_payload",
        .help  = "set_payload <NUM>",
        .parse = wlnpi_cmd_set_payload,
        .show  = wlnpi_show_only_status,
    },
    {
        /* get_payload */
        .id    = WLNPI_CMD_GET_PAYLOAD,
        .name  = "get_payload",
        .help  = "get_payload",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_payload,
    },  

    {
        /* set_pkt_length */
        .id    = WLNPI_CMD_SET_PKTLEN,
        .name  = "set_pkt_length",
        .help  = "set_pkt_length <NUM>",
        .parse = wlnpi_cmd_set_pkt_length,
        .show  = wlnpi_show_only_status,
    },
    {
        /* get_pkt_length */
        .id    = WLNPI_CMD_GET_PKTLEN,
        .name  = "get_pkt_length",
        .help  = "get_pkt_length",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_pkt_length,
    },  

    {
        /* set_guard_interval */
        .id    = WLNPI_CMD_SET_GUARD_INTERVAL,
        .name  = "set_guard_interval",
        .help  = "set_guard_interval <NUM>",
        .parse = wlnpi_cmd_set_guard_interval,
        .show  = wlnpi_show_only_status,
    },
    {
        /* get_guard_interval */
        .id    = WLNPI_CMD_GET_GUARD_INTERVAL,
        .name  = "get_guard_interval",
        .help  = "get_guard_interval",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_guard_interval,
    },  

    {
        /* set_tx_mode */
        .id    = WLNPI_CMD_SET_TX_MODE,
        .name  = "set_tx_mode",
        .help  = "set_tx_mode <NUM>",
        .parse = wlnpi_cmd_set_tx_mode,
        .show  = wlnpi_show_only_status,
    },
    {
        /* get_tx_mode */
        .id    = WLNPI_CMD_GET_TX_MODE,
        .name  = "get_tx_mode",
        .help  = "get_tx_mode",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_tx_mode,
    },  

    {
        /* set_macfilter */
        .id    = WLNPI_CMD_SET_MAC_FILTER,
        .name  = "set_macfilter",
        .help  = "set_macfilter <NUM>",
        .parse = wlnpi_cmd_set_macfilter,
        .show  = wlnpi_show_only_status,
    },
    {
        /* get_macfilter */
        .id    = WLNPI_CMD_GET_MAC_FILTER,
        .name  = "get_macfilter",
        .help  = "get_macfilter",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_macfilter,
    },
    {
        .id    = WLNPI_CMD_SET_WLAN_CAP,
        .name  = "set_wlan_cap",
        .help  = "set_wlan_cap <NUM>",
        .parse = wlnpi_cmd_set_wlan_cap,
        .show  = wlnpi_show_only_status,
    },
    {
        .id    = WLNPI_CMD_GET_WLAN_CAP,
        .name  = "get_wlan_cap",
        .help  = "get_wlan_cap",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_wlan_cap,
    },
    {
        /* get_current_connected_ap_info */
        .id    = WLNPI_CMD_GET_CONN_AP_INFO,
        .name  = WLNPI_CMD_CONN_STATUS_STR,
        .help  = "get current connected ap's info",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_connected_ap_info,
    },
    {
        /* get current mcs index in use */
        .id    = WLNPI_CMD_GET_MCS_INDEX,
        .name  = WLNPI_CMD_MSC_INDEX_STR,
        .help  = "get mcs index in use.",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_mcs_index,
    },

    {
        /* set auto rate flag(on or off) and index */
        .id    = WLNPI_CMD_SET_AUTORATE_FLAG,
        .name  = WLNPI_CMD_SET_AUTORATE_STR,
        .help  = "set auto rate flag(on or off) and index.",
        .parse = wlnpi_cmd_set_ar,
        .show  = wlnpi_show_only_status,
    },
    {
        /* get auto rate flag(on or off) and index */
        .id    = WLNPI_CMD_GET_AUTORATE_FLAG,
        .name  = WLNPI_CMD_GET_AUTORATE_STR,
        .help  = "get auto rate flag(on or off) and index.",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_ar,
    },

    {
        /* set auto rate pktcnt */
        .id    = WLNPI_CMD_SET_AUTORATE_PKTCNT,
        .name  = WLNPI_CMD_SET_AUTORATE_PKTCNT_STR,
        .help  = "set auto rate pktcnt.",
        .parse = wlnpi_cmd_set_ar_pktcnt,
        .show  = wlnpi_show_only_status,
    },
    {
        /* get auto rate pktcnt */
        .id    = WLNPI_CMD_GET_AUTORATE_PKTCNT,
        .name  = WLNPI_CMD_GET_AUTORATE_PKTCNT_STR,
        .help  = "get auto rate pktcnt.",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_ar_pktcnt,
    },

    {
        /* set auto rate retcnt */
        .id    = WLNPI_CMD_SET_AUTORATE_RETCNT,
        .name  = WLNPI_CMD_SET_AUTORATE_RETCNT_STR,
        .help  = "set auto rate retcnt.",
        .parse = wlnpi_cmd_set_ar_retcnt,
        .show  = wlnpi_show_only_status,
    },
    {
        /* get auto rate retcnt */
        .id    = WLNPI_CMD_GET_AUTORATE_RETCNT,
        .name  = WLNPI_CMD_GET_AUTORATE_RETCNT_STR,
        .help  = "get auto rate retcnt.",
        .parse = wlnpi_cmd_no_argv,
        .show  = wlnpi_show_get_ar_retcnt,
    },

};

struct wlnpi_cmd_t *match_cmd_table(char *name)
{
    size_t          i;
    struct wlnpi_cmd_t *cmd = NULL;
    for(i=0; i< sizeof(g_cmd_table)/sizeof(struct wlnpi_cmd_t) ; i++)
    {
        if( 0 != strcmp(name, g_cmd_table[i].name) )
        {
            continue;
        }

        cmd = &g_cmd_table[i];
        break;
    }   
    return cmd;
}

void do_help(void)
{
    int i, max;
    max = sizeof(g_cmd_table)/sizeof(struct wlnpi_cmd_t);
    for(i=0; i<max; i++)
    {
        printf("wlnpi %s\n", g_cmd_table[i].help);
    }
    return;
}

/********************************************************************
*   name   iwnpi_hex_dump
*   ---------------------------
*   description:
*   ----------------------------
*   para        IN/OUT      type            note
*   ----------------------------------------------------
*   return
*   0:exec successful
*   -1:error
*   ------------------
*   other:
*
********************************************************************/
static int iwnpi_hex_dump(unsigned char *name, unsigned short nLen, unsigned char *pData, unsigned short len)
{
    unsigned char *str;
    int i, p, ret;

    ENG_LOG("ADL entry %s(), len = %d", __func__, len);

    if (len > 1024)
        len = 1024;
    str = malloc(((len + 1) * 3 + nLen));
    memset(str, 0, (len + 1) * 3 + nLen);
    memcpy(str, name, nLen);
    if ((NULL == pData) || (0 == len))
    {
        printf("%s\n", str);
        free(str);
        return 0;
    }

    p = 0;
    for (i = 0; i < len; i++)
    {
        ret = sprintf((str + nLen + p), "%02x ", *(pData + i));
        p = p + ret;
    }
    printf("%s\n\n", str);
    free(str);

    ENG_LOG("ADL levaling %s()", __func__);
    return 0;
}

