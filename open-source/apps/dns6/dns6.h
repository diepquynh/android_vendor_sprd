/*
 * Bump in the Host (BIH)
 * http://code.google.com/p/bump-in-the-host/source/
 * ----------------------------------------------------------
 *
 *  Copyrighted (C) 2010,2011 by the China Mobile Communications
 *  Corporation <bih.cmcc@gmail.com>;
 *  See the COPYRIGHT file for full details.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 *
 */

#ifndef _DNS6_H
#define _DNS6_H
typedef enum __ns_type {
        ns_t_invalid = 0,       /*%< Cookie. */
        ns_t_a = 1,             /*%< Host address. */
        ns_t_ns = 2,            /*%< Authoritative server. */
        ns_t_md = 3,            /*%< Mail destination. */
        ns_t_mf = 4,            /*%< Mail forwarder. */
        ns_t_cname = 5,         /*%< Canonical name. */
        ns_t_soa = 6,           /*%< Start of authority zone. */
        ns_t_mb = 7,            /*%< Mailbox domain name. */
        ns_t_mg = 8,            /*%< Mail group member. */
        ns_t_mr = 9,            /*%< Mail rename name. */
        ns_t_null = 10,         /*%< Null resource record. */
        ns_t_wks = 11,          /*%< Well known service. */
        ns_t_ptr = 12,          /*%< Domain name pointer. */
        ns_t_hinfo = 13,        /*%< Host information. */
        ns_t_minfo = 14,        /*%< Mailbox information. */
        ns_t_mx = 15,           /*%< Mail routing information. */
        ns_t_txt = 16,          /*%< Text strings. */
        ns_t_rp = 17,           /*%< Responsible person. */
        ns_t_afsdb = 18,        /*%< AFS cell database. */
        ns_t_x25 = 19,          /*%< X_25 calling address. */
        ns_t_isdn = 20,         /*%< ISDN calling address. */
        ns_t_rt = 21,           /*%< Router. */
        ns_t_nsap = 22,         /*%< NSAP address. */
        ns_t_nsap_ptr = 23,     /*%< Reverse NSAP lookup (deprecated). */
        ns_t_sig = 24,          /*%< Security signature. */
        ns_t_key = 25,          /*%< Security key. */
        ns_t_px = 26,           /*%< X.400 mail mapping. */
        ns_t_gpos = 27,         /*%< Geographical position (withdrawn). */
        ns_t_aaaa = 28,         /*%< Ip6 Address. */
        ns_t_loc = 29,          /*%< Location Information. */
        ns_t_nxt = 30,          /*%< Next domain (security). */
        ns_t_eid = 31,          /*%< Endpoint identifier. */
        ns_t_nimloc = 32,       /*%< Nimrod Locator. */
        ns_t_srv = 33,          /*%< Server Selection. */
        ns_t_atma = 34,         /*%< ATM Address */
        ns_t_naptr = 35,        /*%< Naming Authority PoinTeR */
        ns_t_kx = 36,           /*%< Key Exchange */
        ns_t_cert = 37,         /*%< Certification record */
        ns_t_a6 = 38,           /*%< IPv6 address (deprecated, use ns_t_aaaa) */
        ns_t_dname = 39,        /*%< Non-terminal DNAME (for IPv6) */
        ns_t_sink = 40,         /*%< Kitchen sink (experimentatl) */
        ns_t_opt = 41,          /*%< EDNS0 option (meta-RR) */
        ns_t_apl = 42,          /*%< Address prefix list (RFC3123) */
        ns_t_tkey = 249,        /*%< Transaction key */
        ns_t_tsig = 250,        /*%< Transaction signature. */
        ns_t_ixfr = 251,        /*%< Incremental zone transfer. */
        ns_t_axfr = 252,        /*%< Transfer zone of authority. */
        ns_t_mailb = 253,       /*%< Transfer mailbox records. */
        ns_t_maila = 254,       /*%< Transfer mail agent records. */
        ns_t_any = 255,         /*%< Wildcard match. */
        ns_t_zxfr = 256,        /*%< BIND-specific, nonstandard. */
        ns_t_max = 65536
} ns_type;

typedef enum __ns_class {
        ns_c_invalid = 0,       /*%< Cookie. */
        ns_c_in = 1,            /*%< Internet. */
        ns_c_2 = 2,             /*%< unallocated/unsupported. */
        ns_c_chaos = 3,         /*%< MIT Chaos-net. */
        ns_c_hs = 4,            /*%< MIT Hesiod. */
        /* Query class values which do not appear in resource records */
        ns_c_none = 254,        /*%< for prereq. sections in update requests */
        ns_c_any = 255,         /*%< Wildcard match. */
        ns_c_max = 65536
} ns_class;

struct dns_quest_node
{
        struct dns_quest_node *next;
        struct dns_quest_node *prev;
        char *name;
        unsigned short type;
        unsigned short class;
        unsigned int addr;
        unsigned short offset;
};

struct dns_rr_node
{
        struct dns_rr_node *next;
        struct dns_rr_node *prev;
        char *name;
        unsigned short type;
        unsigned short class;
        unsigned int ttl;
        unsigned short rrlen;
        unsigned char *rr;
        char *rrascii;
};

struct dns_node
{
        unsigned short id;
        unsigned short flag;
        unsigned char qr;
        unsigned char opcode;
        unsigned char aa;
        unsigned char tc;
        unsigned char rd;
        unsigned char ra;
        unsigned char rcode;
        unsigned short qdnum;
        unsigned short annum;
        unsigned short nsnum;
        unsigned short arnum;
        struct dns_quest_node *qd;
        struct dns_rr_node *an;
        struct dns_rr_node *ns;
        struct dns_rr_node *ar;
};

#define PKG_GET_STRING(val, cp) \
do \
{ \
        while(*(cp)) \
                *(val)++=*(cp)++; \
        *(val)++=0; \
} \
while(0)

#define        PKG_GET_BYTE(val, cp)        ((val) = *(cp)++)

#define        PKG_GET_SHORT(val, cp) \
do \
{ \
        unsigned short Xv; \
        Xv = (*(cp)++) << 8; \
        Xv |= *(cp)++; \
        (val) = Xv; \
} \
while(0)

#define        PKG_GET_NETSHORT(val, cp) \
do \
{ \
        unsigned char *Xvp; \
        unsigned short Xv; \
        Xvp = (unsigned char *) &Xv; \
        *Xvp++ = *(cp)++; \
        *Xvp++ = *(cp)++; \
        (val) = Xv; \
} \
while(0)

#define        PKG_GET_LONG(val, cp) \
do \
{ \
        unsigned long Xv; \
        Xv = (*(cp)++) << 24; \
        Xv |= (*(cp)++) << 16; \
        Xv |= (*(cp)++) << 8; \
        Xv |= *(cp)++; \
        (val) = Xv; \
} \
while(0)

#define        PKG_GET_NETLONG(val, cp) \
do \
{ \
        unsigned char *Xvp; \
        unsigned long Xv; \
        Xvp = (unsigned char *) &Xv; \
        *Xvp++ = *(cp)++; \
        *Xvp++ = *(cp)++; \
        *Xvp++ = *(cp)++; \
        *Xvp++ = *(cp)++; \
        (val) = Xv; \
} \
while(0)

#define PKG_PUT_BUF(val, len, cp) \
do \
{ \
        memcpy((cp),(val),(len)); \
        (cp)+=(len); \
} \
while(0)

#define PKG_PUT_STRING(val, cp) \
do \
{ \
        while(*(val)) \
                *(cp)++=*(val)++; \
        *(cp)++=0; \
} \
while(0)

#define        PKG_PUT_BYTE(val, cp)         (*(cp)++ = (unsigned char)(val))

#define        PKG_PUT_SHORT(val, cp) \
do \
{ \
        unsigned short Xv; \
        Xv = (unsigned short)(val); \
        *(cp)++ = (unsigned char)(Xv >> 8); \
        *(cp)++ = (unsigned char)Xv; \
} \
while(0)

#define        PKG_PUT_NETSHORT(val, cp) \
do \
{ \
        unsigned char *Xvp; \
        unsigned short Xv = (unsigned short)(val); \
        Xvp = (unsigned char *)&Xv; \
        *(cp)++ = *Xvp++; \
        *(cp)++ = *Xvp++; \
} \
while(0)

#define        PKG_PUT_LONG(val, cp) \
do \
{ \
        unsigned long Xv; \
        Xv = (unsigned long)(val); \
        *(cp)++ = (unsigned char)(Xv >> 24); \
        *(cp)++ = (unsigned char)(Xv >> 16); \
        *(cp)++ = (unsigned char)(Xv >>  8); \
        *(cp)++ = (unsigned char)Xv; \
} \
while(0)

#define        PKG_PUT_NETLONG(val, cp) \
do \
{ \
        unsigned char *Xvp; \
        unsigned long Xv = (unsigned long)(val); \
        Xvp = (unsigned char *)&Xv; \
        *(cp)++ = *Xvp++; \
        *(cp)++ = *Xvp++; \
        *(cp)++ = *Xvp++; \
        *(cp)++ = *Xvp++; \
} \
while(0)

#define vmalloc malloc
#define vrealloc(p,newsize,oldsize,freeflag) realloc((p),(newsize))
#define vfree free

#define lprintf(level,format,...) \
do \
{ \
        if((level)>loglevel) \
                break; \
        if(level==0) \
        { \
                if(daemonflag) \
                        syslog(4,"%s(%d)%s: "format,__FILE__,__LINE__,(char *)__FUNCTION__,##__VA_ARGS__ ); \
                else \
                        fprintf(stderr,"%s(%d)%s: "format,__FILE__,__LINE__,(char *)__FUNCTION__,##__VA_ARGS__ ); \
        } \
        else \
        { \
                if(daemonflag) \
                        syslog(4,(format),##__VA_ARGS__ ); \
                else \
                        fprintf(stderr,(format),##__VA_ARGS__ ); \
        } \
} \
while(0)
#endif
