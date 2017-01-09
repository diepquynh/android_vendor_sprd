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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <net/if.h>
#include <linux/in6.h>
#include <linux/in.h>
#include <netinet/ip6.h>
#include <arpa/inet.h>
#include "dns6.h"

#include <utils/Log.h>
#ifdef ANDROID_CHANGES
#include <cutils/properties.h>
#endif

#ifdef ANDROID_CHANGES
#undef lprintf(a, b...)
#define LOG_TAG "DNS6"
#define lprintf(a, b...) android_printLog(a, LOG_TAG, b)
#define DNS6_DEBUG(a, b...) android_printLog(a, LOG_TAG, b)
#endif
#define NIPQUAD(addr) \
        ((unsigned char *)&addr)[0], \
        ((unsigned char *)&addr)[1], \
        ((unsigned char *)&addr)[2], \
        ((unsigned char *)&addr)[3]
#ifdef ANDROID_CHANGES
char DNSSERVER[256]="192.168.1.34",loglevel=0;
char pidpropname[PROPERTY_KEY_MAX];
char pidpropval[PROPERTY_VALUE_MAX];
#else
char DNSSERVER[256]="192.168.2.25",CONFFILE[256]="/system/etc/resolv.conf",loglevel=0;
#endif
fd_set sockreadset,sockwriteset,sockreadsettmp,sockwritesettmp;
int daemonflag=0,BIHMODE=0,NETWORKTYPE=0,BIHSTATUS=0,PRIVATE=0;
int buffPrintf(char **fbuff,const char *format, ...);

char *shell(char *cmd)
{
        FILE *fp;
        int bufflen;
        static char buff[4096];
        DNS6_DEBUG(0, "enter shell");
        if(cmd==NULL)
        {
                lprintf(0,"argument error");
                return(NULL);
        }
        buff[0]=0;
        fp=popen(cmd,"r");
        if(fp==NULL)
        {
                lprintf(0,"popen error");
                return(NULL);
        }
        bufflen=fread(buff,1,sizeof(buff)-1,fp);
        if(bufflen)
                buff[bufflen]=0;
        else
                buff[0]=0;
        pclose(fp);
        if(strlen(buff)>0)
                return(buff);
        else
                return(NULL);
}

int fileExist(char *filename)
{
        struct        stat        fstat;
        DNS6_DEBUG(0, "enter fileExist");
        if(filename==NULL)
        {
                lprintf(0,"arguments error\n");
                return(-1);
        }
        if(stat(filename, &fstat) != 0)
        {
                lprintf(0,"filename %s is not exist", filename);
                return(-1);
        }
        return(0);
}

int bih_turn(char on)
{
        int ret;
        lprintf(0, "enter bih_turn on=%c",on);
        if(fileExist("/proc/bih/mode")!=0)
        {
                DNS6_DEBUG(0, "/proc/bih/mode is not exist");
                return(-1);
        }
        if(on)
        {
                ret=shell("echo \"BIS\" > /proc/bih/mode")?-1:0;
                if(ret==0)
                {
                        BIHSTATUS=1;
                        lprintf(5,"BIH is opened\n");
                }
        }
        else
        {
                ret=shell("echo \"NULL\" > /proc/bih/mode")?-1:0;
                if(ret==0)
                {
                        BIHSTATUS=0;
                        lprintf(5,"BIH is closed\n");
                }
        }
        return(ret);
}

int map_clear(unsigned int addr)
{
        int ret;
        char buff[128];
        lprintf(0, "enter map_clear addr=%u.%u.%u.%u",NIPQUAD(addr));
        if(fileExist("/proc/bih/map")!=0)
        {
                DNS6_DEBUG(0, "/proc/bih/map is not exist");
                return(-1);
        }
        if(addr==0)
                ret=shell("echo \"CLEAR\" > /proc/bih/map")?-1:0;
        else
        {
                snprintf(buff,sizeof(buff)-1,"echo \"DEL %u.%u.%u.%u\" > /proc/bih/map",NIPQUAD(addr));
                ret=shell(buff)?-1:0;
        }
        return(ret);
}

unsigned char netdev_mask2bit(unsigned int mask)
{
        int i,num=0x80000000;

        lprintf(0, "enter netdev_mask2bit mask=%u.%u.%u.%u",NIPQUAD(mask));
        mask=ntohl(mask);
        for(i=0;i<32;i++)
        {
                if(!(mask&num))
                        return(i);
                mask<<=1;
        }
        return(i);
}

int netdev_getip6(char *name,unsigned char *addr,int *bits)
{
        FILE *f;
        char addr6[40], devname[20],addr6p[8][5];
        struct sockaddr_in6 sap;
        int plen, scope, dad_status, if_idx;
#define IPV6_ADDR_ANY           0x0000U
#define IPV6_ADDR_UNICAST       0x0001U
#define IPV6_ADDR_MULTICAST     0x0002U
#define IPV6_ADDR_ANYCAST       0x0004U
#define IPV6_ADDR_LOOPBACK      0x0010U
#define IPV6_ADDR_LINKLOCAL     0x0020U
#define IPV6_ADDR_SITELOCAL     0x0040U
#define IPV6_ADDR_COMPATv4      0x0080U
#define IPV6_ADDR_SCOPE_MASK    0x00f0U
#define IPV6_ADDR_MAPPED        0x1000U
#define IPV6_ADDR_RESERVED      0x2000U

        lprintf(0, "enter netdev_getip6 name %s", name);
        if(name==NULL)
        {
                lprintf(0,"arguments error\n");
                return(-1);
        }
        if(addr)
                memset(addr,0,16);
        if(bits)
                *bits=0;
        f = fopen("/proc/net/if_inet6", "r");
        if(f==NULL)
        {
                DNS6_DEBUG(0, "/proc/net/if_inet6 is not exist");
                return(-1);
        }
        while(fscanf(f, "%4s%4s%4s%4s%4s%4s%4s%4s %08x %02x %02x %02x %20s\n",
                                        addr6p[0],addr6p[1],addr6p[2],addr6p[3],addr6p[4],addr6p[5],addr6p[6],addr6p[7],
                                        &if_idx, &plen, &scope,&dad_status, devname) != EOF)
        {
                if(!strcmp(devname, name)&&(scope&IPV6_ADDR_SCOPE_MASK)==0)
                {
                        sprintf(addr6,"%s:%s:%s:%s:%s:%s:%s:%s",addr6p[0], addr6p[1], addr6p[2], addr6p[3],
                                addr6p[4], addr6p[5], addr6p[6], addr6p[7]);
                        if(inet_pton(AF_INET6, addr6,(struct sockaddr *) &sap.sin6_addr)<=0)
    		{
                        DNS6_DEBUG(0, "inet_pton failed.");
                        return (-1);
                }
                        if(addr)
                                memcpy(addr,sap.sin6_addr.s6_addr,16);
                        if(bits)
                                *bits=plen;
                        fclose(f);
                        return(0);
                }
        }
        fclose(f);
        DNS6_DEBUG(0, "netdev_getip6 failed");
        return(-1);
}

int netdev_getip(char *name,unsigned int *addr,int *bits)
{
        int sockid,maskbit;
        struct ifreq ifr;
        struct sockaddr_in *sin;
        unsigned int ipaddr,ipmask;

        DNS6_DEBUG(0, "enter netdev_getip name:%s",name);
        sockid=socket( AF_INET, SOCK_DGRAM, 0 );
        if(sockid<0)
        {
                lprintf(0,"socket error(%s)\n",strerror(errno));
                return(-1);
        }
        memset(&ifr,0,sizeof(ifr));
        strcpy(ifr.ifr_name,name);
        if(ioctl(sockid,SIOCGIFADDR,&ifr)!=0)
        {
                lprintf(0,"ioctl error(%s)\n",strerror(errno));
                close(sockid);
                return(-1);
        }
        sin=(struct sockaddr_in *)&(ifr.ifr_addr);
        ipaddr=sin->sin_addr.s_addr;
        if(ipaddr==PRIVATE)
        {
                DNS6_DEBUG(0, "ip addr is a private");
                close(sockid);
                return(-1);
        }
        if(ioctl(sockid,SIOCGIFNETMASK,&ifr)!=0)
        {
                lprintf(0,"ioctl error(%s)\n",strerror(errno));
                close(sockid);
                return(-1);
        }
        close(sockid);
        sin=(struct sockaddr_in *)&(ifr.ifr_addr);
        ipmask=sin->sin_addr.s_addr;
        maskbit=netdev_mask2bit(ipmask);
        if(addr)
                *addr=ipaddr;
        if(bits)
                *bits=maskbit;
        return(0);
}

int networktype(void)
{
        char buff[256],*p1,*p2;
        FILE *fp;
        int type4=0,type6=0;

        DNS6_DEBUG(0, "enter networktype");
        if(fileExist("/proc/net/dev")!=0)
        {
                DNS6_DEBUG(0, "/proc/net/dev is not exist");
                return(-1);
        }
        fp=popen("cat /proc/net/dev","r");
        if(fp==NULL)
        {
                DNS6_DEBUG(0, "/proc/net/dev popen error");
                fprintf(stderr,"popen error\n");
                return(-1);
        }
        while(fgets(buff,sizeof(buff)-1,fp)!=NULL)
        {
                for(p1=buff;p1&&*p1!=0;p1++)
                {
                        if(*p1!=' '&&*p1!='\t')
                                break;
                }
                for(p2=p1;p2&&*p2!=0;p2++)
                {
                        if(*p2==' '||*p2=='\t')
                        {
                                *p2=0;
                                break;
                        }
                        else if(*p2==':')
                        {
                                *p2=0;
                                if(strncmp(p1,"veth_s", 6)!=0)
                                        continue;
                                if(netdev_getip(p1,NULL,NULL)==0)
                                        type4=1;
                                if(netdev_getip6(p1,NULL,NULL)==0)
                                        type6=1;
                                break;
                        }
                }
        }
        pclose(fp);

        if(type4&&type6)
        {
                if(fileExist("/proc/bih/network")==0)
                {
                        DNS6_DEBUG(0, "/proc/bih/network DS");
                        shell("echo \"DS\" > /proc/bih/network");
                }
                return(3);
        }
        else if(type4)
        {
                if(fileExist("/proc/bih/network")==0)
                {
                        DNS6_DEBUG(0, "/proc/bih/network V4");
                        shell("echo \"V4\" > /proc/bih/network");
                }
                return(1);
        }
        else if(type6)
        {
                if(fileExist("/proc/bih/network")==0)
                {
                        DNS6_DEBUG(0, "/proc/bih/network V6");
                        shell("echo \"V6\" > /proc/bih/network");
                }
                return(2);
        }
        else
        {
                DNS6_DEBUG(0, "/proc/bih/network type error");
                return(0);
        }
}

char *networktype_name(int code)
{
        static char buff[32];
        lprintf(0, "enter networktype_name code=%d", code);
        switch(code)
        {
                case 0:
                        strcpy(buff,"NULL");
                        break;
                case 1:
                        strcpy(buff,"IPV4");
                        break;
                case 2:
                        strcpy(buff,"IPV6");
                        break;
                case 3:
                        strcpy(buff,"DS");
                        break;
                default:
                        strcpy(buff,"NULL");
                        break;
        }
        return(buff);
}



int dnsNameEncode(char *name,unsigned char *buff)
{
        int len;
        char *nameend,*p1,*p2;
        unsigned char *pos;

        DNS6_DEBUG(0, "enter dnsNameEncode");
        if(name==NULL||buff==NULL)
                return(-1);
        len=strlen(name);
        nameend=name+len;
        p1=name;
        pos=buff;
        while(1)
        {
                for(p2=p1;p2<nameend&&p2&&*p2!='.';p2++);
                len=p2-p1;
                *pos++=len;
                memcpy(pos,p1,len);
                pos+=len;
                if(p2==nameend)
                        break;
                else
                {
                        p1=p2+1;
                }
        }
        *pos++=0;
        return(pos-buff);
}

int dnsNameLen(unsigned char *buff,int bufflen,int offset)
{
        int len,ret;
        unsigned short index;
        unsigned char *buffend,*p;

        DNS6_DEBUG(0, "enter dnsNameLen");
        if(buff==NULL||bufflen<=0||offset>=bufflen)
        {
                lprintf(0,"arguments error\n");
                return(-1);
        }
        buffend=buff+bufflen;
        for(p=buff+offset,len=0;p&&p!=buffend;p++,len++)
        {
                if(*p==0)
                        return(len);
                else if((*p&0xc0)==0xc0)
                {
                        PKG_GET_SHORT(index,p);
                        index&=0x3fff;
                        ret=dnsNameLen(buff,bufflen,index);
                        if(ret<=0)
                        {
                                lprintf(0,"dnsNameLen error\n");
                                return(-1);
                        }
                        return(len+ret);
                }
        }
        return(len);
}

int dnsNameParse(unsigned char *buff,int bufflen,int offset,char **name)
{
        int ret,len,n;
        unsigned short index;
        unsigned char *buffend,*p;
        char *tmpbuf,*newbuff=NULL;

        DNS6_DEBUG(0, "enter dnsNameParse");
        if(buff==NULL||bufflen<=0||offset>=bufflen||name==NULL)
        {
                lprintf(0,"arguments error\n");
                return(-1);
        }
        len=dnsNameLen(buff,bufflen,offset);
        if(len<=0)
        {
                lprintf(0,"dnsNameLen error\n");
                return(-1);
        }
        buffend=buff+bufflen;
        newbuff=vmalloc(len+1);
        if(newbuff)
        {
                for(p=buff+offset,len=0;p&&p!=buffend;)
                {
                        if(*p==0)
                        {
                                p++;
                                break;
                        }
                        else if((*p&0xc0)==0xc0)
                        {
                                if(len)
                                        newbuff[len++]='.';
                                PKG_GET_SHORT(index,p);
                                index&=0x3fff;
                                ret=dnsNameParse(buff,bufflen,index,&tmpbuf);
                                if(ret<=0)
                                {
                                        lprintf(0,"dnsNameParse error\n");
                                        vfree(newbuff);
                                        return(-1);
                                }
                                n=strlen(tmpbuf);
                                memcpy(newbuff+len,tmpbuf,n);
                                vfree(tmpbuf);
                                len+=n;
                                break;
                        }
                        if(len)
                                newbuff[len++]='.';
                        n=*p++;
                        memcpy(newbuff+len,p,n);
                        len+=n;
                        p+=n;
                }
                newbuff[len]=0;
        }
        else
        {
                lprintf(0,"vmalloc failed\n");
                return(-1);
        }
        *name=newbuff;
        return(p-buff-offset);
}

int dnsNodeFree(struct dns_node *node)
{
        struct dns_quest_node *questnode;
        struct dns_rr_node *rrnode;
        DNS6_DEBUG(0, "enter dnsNodeFree");
        if(node==NULL)
                return(-1);
        for(questnode=node->qd;questnode;questnode=node->qd)
        {
                node->qd=node->qd->next;
                if(questnode->name)
                        vfree(questnode->name);
                vfree(questnode);
        }
        for(rrnode=node->an;rrnode;rrnode=node->an)
        {
                node->an=node->an->next;
                if(rrnode->name)
                        vfree(rrnode->name);
                if(rrnode->rrlen)
                        vfree(rrnode->rr);
                if(rrnode->rrascii)
                        vfree(rrnode->rrascii);
                vfree(rrnode);
        }
        for(rrnode=node->ns;rrnode;rrnode=node->ns)
        {
                node->ns=node->ns->next;
                if(rrnode->name)
                        vfree(rrnode->name);
                if(rrnode->rrlen)
                        vfree(rrnode->rr);
                if(rrnode->rrascii)
                        vfree(rrnode->rrascii);
                vfree(rrnode);
        }
        for(rrnode=node->ar;rrnode;rrnode=node->ar)
        {
                node->ar=node->ar->next;
                if(rrnode->name)
                        vfree(rrnode->name);
                if(rrnode->rrlen)
                        vfree(rrnode->rr);
                if(rrnode->rrascii)
                        vfree(rrnode->rrascii);
                vfree(rrnode);
        }
        vfree(node);
        return(0);
}

int dnsNodeShow(struct dns_node *node)
{
        struct dns_quest_node *questnode;
        struct dns_rr_node *rrnode;
        DNS6_DEBUG(0, "enter dnsNodeShow");
        if(node==NULL)
                return(-1);
        lprintf(5,"id(%d) flag(0x%04x) qr(%d) opcode(%d) aa(%d) tc(%d) rd(%d) ra(%d) rcode(%d) "
                "qdnum(%d) annum(%d) nsnum(%d) arnum(%d)\n",
                node->id,node->flag,node->qr,node->opcode,node->aa,node->tc,node->rd,node->ra,node->rcode,
                node->qdnum,node->annum,node->nsnum,node->arnum);
        if(node->qdnum)
        {
                lprintf(5,"\tQD[%d]:",node->qdnum);
                for(questnode=node->qd;questnode;questnode=questnode->next)
                {
                        lprintf(5," <%s %d %d %u.%u.%u.%u>",questnode->name,questnode->type,questnode->class,NIPQUAD(questnode->addr));
                }
                lprintf(5,"\n");
        }
        if(node->annum)
        {
                lprintf(5,"\tAN[%d]:",node->annum);
                for(rrnode=node->an;rrnode;rrnode=rrnode->next)
                {
                        lprintf(5," <%s %d %d %u %d %s>",rrnode->name,rrnode->type,rrnode->class,rrnode->ttl,rrnode->rrlen,rrnode->rrascii?rrnode->rrascii:"");
                }
                lprintf(5,"\n");
        }
        if(node->nsnum)
        {
                lprintf(5,"\tNS[%d]:",node->nsnum);
                for(rrnode=node->ns;rrnode;rrnode=rrnode->next)
                {
                        lprintf(5," <%s %d %d %u %d %s>",rrnode->name,rrnode->type,rrnode->class,rrnode->ttl,rrnode->rrlen,rrnode->rrascii?rrnode->rrascii:"");
                }
                lprintf(5,"\n");
        }
        if(node->arnum)
        {
                lprintf(5,"\tAR[%d]:",node->arnum);
                for(rrnode=node->ar;rrnode;rrnode=rrnode->next)
                {
                        lprintf(5," <%s %d %d %u %d %s>",rrnode->name,rrnode->type,rrnode->class,rrnode->ttl,rrnode->rrlen,rrnode->rrascii?rrnode->rrascii:"");
                }
                lprintf(5,"\n");
        }
        return(0);
}

int dnsEncode(struct dns_node *dnsnode,unsigned char **buff,int *bufflen)
{
        struct dns_quest_node *questnode;
        unsigned char *newbuf,*pos;
        int num,size,len;
        DNS6_DEBUG(0, "enter dnsEncode");
        for(num=0,size=14,questnode=dnsnode->qd;questnode;questnode=questnode->next)
        {
                if(questnode->addr)
                {
                        num++;
                        len=strlen(questnode->name)+22;
                        size+=len;
                }
        }
        if(num==0)
        {
                DNS6_DEBUG(0, "dnsEncode num is zero");
                return(-1);
        }
        newbuf=vmalloc(size);
        if(newbuf==NULL)
        {
                lprintf(0,"vmalloc error\n");
                return(-1);
        }
        memset(newbuf,0,size);
        pos=newbuf;
        PKG_PUT_SHORT(size-2,pos);
        PKG_PUT_SHORT(dnsnode->id,pos);
        PKG_PUT_SHORT(dnsnode->flag,pos);
        PKG_PUT_SHORT(num,pos);
        PKG_PUT_SHORT(num,pos);
        PKG_PUT_SHORT(0,pos);
        PKG_PUT_SHORT(0,pos);
        for(questnode=dnsnode->qd;questnode;questnode=questnode->next)
        {
                if(questnode->addr)
                {
                        len=dnsNameEncode(questnode->name,pos);
                        if(len<=0)
                        {
                                lprintf(0,"dnsNameEncode error\n");
                                vfree(newbuf);
                                return(-1);
                        }
                        questnode->offset=pos-newbuf-2;
                        pos+=len;
                        PKG_PUT_SHORT(questnode->type,pos);
                        PKG_PUT_SHORT(questnode->class,pos);
                }
        }
        for(questnode=dnsnode->qd;questnode;questnode=questnode->next)
        {
                if(questnode->addr)
                {
                        PKG_PUT_SHORT(questnode->offset|0xc000,pos);
                        PKG_PUT_SHORT(questnode->type,pos);
                        PKG_PUT_SHORT(questnode->class,pos);
                        PKG_PUT_LONG(86400,pos);
                        PKG_PUT_SHORT(4,pos);
                        PKG_PUT_NETLONG(questnode->addr,pos);
                }
        }
        len=pos-newbuf;
        *buff=newbuf;
        *bufflen=len;
        return(0);
}

int dnsParse(unsigned char *buff,int bufflen,struct dns_node **dnsnode)
{
        int i,len;
        struct dns_node *node;
        struct dns_quest_node *questtail=NULL,*questnode;
        struct dns_rr_node *rrtail=NULL,*rrnode;
        unsigned char *pos;

        DNS6_DEBUG(0, "enter dnsParse");
        if(buff==NULL||bufflen<12)
        {
                lprintf(0,"arguments error\n");
                return(-1);
        }
        pos=buff;
        node=(struct dns_node *)vmalloc(sizeof(struct dns_node));
        if(node==NULL)
        {
                lprintf(0,"vmalloc error\n");
                return(-1);
        }
        memset(node,0,sizeof(struct dns_node));
        PKG_GET_SHORT(node->id,pos);
        PKG_GET_SHORT(node->flag,pos);
        PKG_GET_SHORT(node->qdnum,pos);
        PKG_GET_SHORT(node->annum,pos);
        PKG_GET_SHORT(node->nsnum,pos);
        PKG_GET_SHORT(node->arnum,pos);
        node->qr=(node->flag&0x8000)>>15;
        node->opcode=(node->flag&0x7800)>>11;
        node->aa=(node->flag&0x0400)>>10;
        node->tc=(node->flag&0x0200)>>9;
        node->rd=(node->flag&0x0100)>>8;
        node->ra=(node->flag&0x0080)>>7;
        node->rcode=(node->flag&0x000f);
        if(node->qdnum)
        {
                len=pos-buff;
                if(bufflen<(len+7*node->qdnum))
                {
                        dnsNodeFree(node);
                        lprintf(0,"bufflen(%d)|qdnum(%d) error\n",bufflen,node->qdnum);
                        return(-1);
                }
                for(i=0;i<node->qdnum;i++)
                {
                        questnode=(struct dns_quest_node *)vmalloc(sizeof(struct dns_quest_node));
                        if(questnode==NULL)
                        {
                                dnsNodeFree(node);
                                lprintf(0,"vmalloc error\n");
                                return(-1);
                        }
                        memset(questnode,0,sizeof(struct dns_quest_node));
                        if(questtail==NULL)
                        {
                                node->qd=questnode;
                        }
                        else
                        {
                                questtail->next=questnode;
                        }
                        questtail=questnode;

                        len=dnsNameParse(buff,bufflen,pos-buff,&questnode->name);
                        if(len<=0)
                        {
                                dnsNodeFree(node);
                                lprintf(0,"dnsNameParse error\n");
                                return(-1);
                        }
                        pos+=len;
                        PKG_GET_SHORT(questnode->type,pos);
                        PKG_GET_SHORT(questnode->class,pos);
                }
        }
        if(node->annum)
        {
                len=pos-buff;
                if(bufflen<(len+7*node->annum))
                {
                        dnsNodeFree(node);
                        lprintf(0,"bufflen(%d)|annum(%d) error\n",bufflen,node->annum);
                        return(-1);
                }
                rrtail=NULL;
                for(i=0;i<node->annum;i++)
                {
                        rrnode=(struct dns_rr_node *)vmalloc(sizeof(struct dns_rr_node));
                        if(rrnode==NULL)
                        {
                                dnsNodeFree(node);
                                lprintf(0,"vmalloc error\n");
                                return(-1);
                        }
                        memset(rrnode,0,sizeof(struct dns_rr_node));
                        if(rrtail==NULL)
                        {
                                node->an=rrnode;
                        }
                        else
                        {
                                rrtail->next=rrnode;
                        }
                        rrtail=rrnode;

                        len=dnsNameParse(buff,bufflen,pos-buff,&rrnode->name);
                        if(len<=0)
                        {
                                dnsNodeFree(node);
                                lprintf(0,"dnsNameParse error\n");
                                return(-1);
                        }
                        pos+=len;
                        PKG_GET_SHORT(rrnode->type,pos);
                        PKG_GET_SHORT(rrnode->class,pos);
                        PKG_GET_LONG(rrnode->ttl,pos);
                        PKG_GET_SHORT(rrnode->rrlen,pos);
                        rrnode->rr=vmalloc(rrnode->rrlen+1);
                        if(rrnode->rr)
                        {
                                memcpy(rrnode->rr,pos,rrnode->rrlen);
                                rrnode->rr[rrnode->rrlen]=0;
                                switch(rrnode->type)
                                {
                                        case 5:
                                        case 12:
                                                dnsNameParse(buff,bufflen,pos-buff,&rrnode->rrascii);
                                                break;
                                        case 1:
                                                buffPrintf((char **)&rrnode->rrascii,"%d.%d.%d.%d",rrnode->rr[0],rrnode->rr[1],rrnode->rr[2],rrnode->rr[3]);
                                                break;
                                        case 28:
                                                buffPrintf((char **)&rrnode->rrascii,"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",rrnode->rr[0],rrnode->rr[1],rrnode->rr[2],rrnode->rr[3],rrnode->rr[4],rrnode->rr[5],rrnode->rr[6],rrnode->rr[7],rrnode->rr[8],rrnode->rr[9],rrnode->rr[10],rrnode->rr[11],rrnode->rr[12],rrnode->rr[13],rrnode->rr[14],rrnode->rr[15]);
                                                break;
                                }
                        }
                        pos+=rrnode->rrlen;
                }
        }
        if(node->nsnum)
        {
                len=pos-buff;
                if(bufflen<(len+7*node->nsnum))
                {
                        dnsNodeFree(node);
                        lprintf(0,"bufflen(%d)|nsnum(%d) error\n",bufflen,node->nsnum);
                        return(-1);
                }
                rrtail=NULL;
                for(i=0;i<node->nsnum;i++)
                {
                        rrnode=(struct dns_rr_node *)vmalloc(sizeof(struct dns_rr_node));
                        if(rrnode==NULL)
                        {
                                dnsNodeFree(node);
                                lprintf(0,"vmalloc error\n");
                                return(-1);
                        }
                        memset(rrnode,0,sizeof(struct dns_rr_node));
                        if(rrtail==NULL)
                        {
                                node->ns=rrnode;
                        }
                        else
                        {
                                rrtail->next=rrnode;
                        }
                        rrtail=rrnode;

                        len=dnsNameParse(buff,bufflen,pos-buff,&rrnode->name);
                        if(len<=0)
                        {
                                dnsNodeFree(node);
                                lprintf(0,"dnsNameParse error\n");
                                return(-1);
                        }
                        pos+=len;
                        PKG_GET_SHORT(rrnode->type,pos);
                        PKG_GET_SHORT(rrnode->class,pos);
                        PKG_GET_LONG(rrnode->ttl,pos);
                        PKG_GET_SHORT(rrnode->rrlen,pos);
                        rrnode->rr=vmalloc(rrnode->rrlen+1);
                        if(rrnode->rr)
                        {
                                memcpy(rrnode->rr,pos,rrnode->rrlen);
                                rrnode->rr[rrnode->rrlen]=0;
                                switch(rrnode->type)
                                {
                                        case 5:
                                        case 12:
                                                dnsNameParse(buff,bufflen,pos-buff,&rrnode->rrascii);
                                                break;
                                        case 1:
                                                buffPrintf((char **)&rrnode->rrascii,"%d.%d.%d.%d",rrnode->rr[0],rrnode->rr[1],rrnode->rr[2],rrnode->rr[3]);
                                                break;
                                        case 28:
                                                buffPrintf((char **)&rrnode->rrascii,"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",rrnode->rr[0],rrnode->rr[1],rrnode->rr[2],rrnode->rr[3],rrnode->rr[4],rrnode->rr[5],rrnode->rr[6],rrnode->rr[7],rrnode->rr[8],rrnode->rr[9],rrnode->rr[10],rrnode->rr[11],rrnode->rr[12],rrnode->rr[13],rrnode->rr[14],rrnode->rr[15]);
                                                break;
                                }
                        }
                        pos+=rrnode->rrlen;
                }
        }
        if(node->arnum)
        {
                len=pos-buff;
                if(bufflen<(len+7*node->arnum))
                {
                        lprintf(0,"bufflen(%d)|arnum(%d) error\n",bufflen,node->arnum);
                        dnsNodeFree(node);
                        return(-1);
                }
                rrtail=NULL;
                for(i=0;i<node->arnum;i++)
                {
                        rrnode=(struct dns_rr_node *)vmalloc(sizeof(struct dns_rr_node));
                        if(rrnode==NULL)
                        {
                                dnsNodeFree(node);
                                lprintf(0,"vmalloc error\n");
                                return(-1);
                        }
                        memset(rrnode,0,sizeof(struct dns_rr_node));
                        if(rrtail==NULL)
                        {
                                node->ar=rrnode;
                        }
                        else
                        {
                                rrtail->next=rrnode;
                        }
                        rrtail=rrnode;

                        len=dnsNameParse(buff,bufflen,pos-buff,&rrnode->name);
                        if(len<=0)
                        {
                                dnsNodeFree(node);
                                lprintf(0,"dnsNameParse error\n");
                                return(-1);
                        }
                        pos+=len;
                        PKG_GET_SHORT(rrnode->type,pos);
                        PKG_GET_SHORT(rrnode->class,pos);
                        PKG_GET_LONG(rrnode->ttl,pos);
                        PKG_GET_SHORT(rrnode->rrlen,pos);
                        rrnode->rr=vmalloc(rrnode->rrlen+1);
                        if(rrnode->rr)
                        {
                                memcpy(rrnode->rr,pos,rrnode->rrlen);
                                rrnode->rr[rrnode->rrlen]=0;
                                switch(rrnode->type)
                                {
                                        case 5:
                                        case 12:
                                                dnsNameParse(buff,bufflen,pos-buff,&rrnode->rrascii);
                                                break;
                                        case 1:
                                                buffPrintf((char **)&rrnode->rrascii,"%d.%d.%d.%d",rrnode->rr[0],rrnode->rr[1],rrnode->rr[2],rrnode->rr[3]);
                                                break;
                                        case 28:
                                                buffPrintf((char **)&rrnode->rrascii,"%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",rrnode->rr[0],rrnode->rr[1],rrnode->rr[2],rrnode->rr[3],rrnode->rr[4],rrnode->rr[5],rrnode->rr[6],rrnode->rr[7],rrnode->rr[8],rrnode->rr[9],rrnode->rr[10],rrnode->rr[11],rrnode->rr[12],rrnode->rr[13],rrnode->rr[14],rrnode->rr[15]);
                                                break;
                                }
                        }
                        pos+=rrnode->rrlen;
                }
        }
        if(dnsnode)
        {
                *dnsnode=node;
        }
        else
        {
                lprintf(0,"dnsnode is null\n");
                dnsNodeFree(node);
                return (-1);
        }
        return(0);
}


int dnsQueryPkg(char *domain,unsigned short type,unsigned char **buff,int *bufflen)
{
        static unsigned short id=1;
        unsigned short offset;
        unsigned char *newbuf,*pos;
        int size,len;

        DNS6_DEBUG(0, "enter dnsQueryPkg");
        if(domain==NULL||buff==NULL||bufflen==NULL)
                return(-1);

        size=2+12+(5+strlen(domain))*1;
        newbuf=(unsigned char *)vmalloc(size);
        if(newbuf==NULL)
        {
                lprintf(0,"vmalloc error\n");
                return(-1);
        }
        memset(newbuf,0,size);
        pos=newbuf;
        PKG_PUT_SHORT(size-2,pos);
        PKG_PUT_SHORT(id++,pos);
        PKG_PUT_SHORT(0x0100,pos);
        PKG_PUT_SHORT(1,pos);
        PKG_PUT_SHORT(0,pos);
        PKG_PUT_SHORT(0,pos);
        PKG_PUT_SHORT(0,pos);
        len=dnsNameEncode(domain,pos);
        if(len<=0)
        {
                lprintf(0,"dnsNameEncode error\n");
                vfree(newbuf);
                return(-1);
        }
        offset=pos-newbuf-2;
        pos+=len;
        PKG_PUT_SHORT(type,pos);
        PKG_PUT_SHORT(0x0001,pos);
        len=pos-newbuf;
        *buff=newbuf;
        *bufflen=len;
        return(0);
}

unsigned int in4addrGet(struct in6_addr in6addr)
{
        int sockfd,len,ret;
  struct
  {
          unsigned int in4addr;
          struct in6_addr in6addr;
  }argu;
#define BIH_OPS_BASIC  128
#define BIH_SET        BIH_OPS_BASIC
#define BIH_GET        BIH_OPS_BASIC
#define BIH_MAX        BIH_OPS_BASIC+1

        DNS6_DEBUG(0, "enter in4addrGet");
        memset(&argu,0,sizeof(argu));
  sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
  if(sockfd < 0)
        {
                lprintf(0,"socket error\n");
                return 0;
        }
        memcpy(&argu.in6addr,&in6addr,sizeof(struct in6_addr));
        len=sizeof(argu);
        ret=getsockopt(sockfd, IPPROTO_IP, BIH_GET, &argu, &len);
        close(sockfd);
        if(ret!=0)
        {
                lprintf(0,"socket error\n");
                return 0;
        }
        return(argu.in4addr);
}

int inet_pton4(char *src,unsigned char *dst)
{
        static const char digits[] = "0123456789";
        int saw_digit, octets, ch;
        unsigned char tmp[4], *tp;

        DNS6_DEBUG(0, "enter inet_pton4");
        saw_digit = 0;
        octets = 0;
        *(tp = tmp) = 0;
        while ((ch = *src++) != '\0') {
                const char *pch;

                if ((pch = strchr(digits, ch)) != NULL) {
                        unsigned int new = *tp * 10 + (pch - digits);

                        if (new > 255)
                                return (0);
                        *tp = new;
                        if (! saw_digit) {
                                if (++octets > 4)
                                        return (0);
                                saw_digit = 1;
                        }
                } else if (ch == '.' && saw_digit) {
                        if (octets == 4)
                                return (0);
                        *++tp = 0;
                        saw_digit = 0;
                } else
                        return (0);
        }
        if (octets < 4)
                return (0);
        memcpy(dst, tmp, 4);
        return (1);
}

int inet_pton6(char *src,unsigned char *dst)
{
        static char xdigits_l[] = "0123456789abcdef",xdigits_u[] = "0123456789ABCDEF";
        unsigned char tmp[16], *tp, *endp, *colonp;
        char *xdigits, *curtok;
        int ch, saw_xdigit;
        unsigned int val;

        DNS6_DEBUG(0, "enter inet_pton6");
        memset((tp = tmp), '\0', 16);
        endp = tp + 16;
        colonp = NULL;

        if (*src == ':')
                if (*++src != ':')
                        return (0);
        curtok = src;
        saw_xdigit = 0;
        val = 0;
        while ((ch = *src++) != '\0') {
                const char *pch;

                if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
                        pch = strchr((xdigits = xdigits_u), ch);
                if (pch != NULL) {
                        val <<= 4;
                        val |= (pch - xdigits);
                        if (val > 0xffff)
                                return (0);
                        saw_xdigit = 1;
                        continue;
                }
                if (ch == ':') {
                        curtok = src;
                        if (!saw_xdigit) {
                                if (colonp)
                                        return (0);
                                colonp = tp;
                                continue;
                        }
                        if (tp + 2 > endp)
                                return (0);
                        *tp++ = (unsigned char) (val >> 8) & 0xff;
                        *tp++ = (unsigned char) val & 0xff;
                        saw_xdigit = 0;
                        val = 0;
                        continue;
                }
                if (ch == '.' && ((tp + 4) <= endp) &&
                    inet_pton4(curtok, tp) > 0) {
                        tp += 4;
                        saw_xdigit = 0;
                        break;  /* '\0' was seen by inet_pton4(). */
                }
                return (0);
        }
        if (saw_xdigit) {
                if (tp + 2 > endp)
                        return (0);
                *tp++ = (unsigned char) (val >> 8) & 0xff;
                *tp++ = (unsigned char) val & 0xff;
        }
        if (colonp != NULL) {
                const int n = tp - colonp;
                int i;

                for (i = 1; i <= n; i++) {
                        endp[- i] = colonp[n - i];
                        colonp[n - i] = 0;
                }
                tp = endp;
        }
        if (tp != endp)
                return (0);
        memcpy(dst, tmp, 16);
        return (1);
}

char *inet_ntop4(unsigned char *src, char *dst, size_t size)
{
        static const char *fmt = "%u.%u.%u.%u";
        char tmp[sizeof "255.255.255.255"];

        DNS6_DEBUG(0, "enter inet_ntop4");
        if ((size_t)sprintf(tmp, fmt, src[0], src[1], src[2], src[3]) >= size)
        {
                return (NULL);
        }
        strcpy(dst, tmp);
        return (dst);
}

char *inet_ntop6(unsigned char *src, char *dst, size_t size)
{
        char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
        struct { int base, len; } best, cur;
        unsigned int words[16 / 2];
        int i;

        DNS6_DEBUG(0, "enter inet_ntop6");
        memset(words, '\0', sizeof words);
        for (i = 0; i < 16; i++)
                words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
        best.base = -1;
        best.len=0;
        cur.base = -1;
        cur.len=0;
        for (i = 0; i < (16 / 2); i++) {
                if (words[i] == 0) {
                        if (cur.base == -1)
                                cur.base = i, cur.len = 1;
                        else
                                cur.len++;
                } else {
                        if (cur.base != -1) {
                                if (best.base == -1 || cur.len > best.len)
                                        best = cur;
                                cur.base = -1;
                        }
                }
        }
        if (cur.base != -1) {
                if (best.base == -1 || cur.len > best.len)
                        best = cur;
        }
        if (best.base != -1 && best.len < 2)
                best.base = -1;

        tp = tmp;
        for (i = 0; i < (16 / 2); i++) {
                if (best.base != -1 && i >= best.base &&
                    i < (best.base + best.len)) {
                        if (i == best.base)
                                *tp++ = ':';
                        continue;
                }
                if (i != 0)
                        *tp++ = ':';
                if (i == 6 && best.base == 0 &&
                    (best.len == 6 || (best.len == 5 && words[5] == 0xffff))) {
                        if (!inet_ntop4(src+12, tp,
                                        sizeof tmp - (tp - tmp)))
                                return (NULL);
                        tp += strlen(tp);
                        break;
                }
                tp += sprintf(tp, "%x", words[i]);
        }
        if (best.base != -1 && (best.base + best.len) ==
            (16 / 2))
                *tp++ = ':';
        *tp++ = '\0';

        if ((size_t)(tp - tmp) > size) {
                return (NULL);
        }
        strcpy(dst, tmp);
        return (dst);
}

int fileLen(char *filename)
{
        struct        stat        fstat;
        DNS6_DEBUG(0, "enter fileLen");
        if(filename==NULL)
        {
                lprintf(0,"arguments error\n");
                return(-1);
        }
        if(stat(filename, &fstat) != 0)
        {
                return(-1);
        }
        return(fstat.st_size);
}

char *buff2String1(char *buff,int bufflen)
{
        char *tmpbuf;
        int len;
        DNS6_DEBUG(0, "enter buff2String1");
        if(buff==NULL)
                return(NULL);
        if(bufflen<=0)
                len=strlen(buff);
        else
                len=bufflen;
        tmpbuf=(char *)vmalloc(len+1);
        if(tmpbuf==NULL)
                return(NULL);
        tmpbuf[len]=0;
        memcpy(tmpbuf,buff,len);
        return(tmpbuf);
}

int buffPrintf(char **fbuff,const char *format, ...)
{
        va_list args;
        char buff[4096],*buffpos = NULL;
        int ret,bufflen=0,buffsize;
        DNS6_DEBUG(0, "enter buffPrintf");
        if(fbuff==NULL||format==NULL)
        {
                lprintf(0,"arguments error\n");
                return(-1);
        }
        *fbuff=NULL;
        buffpos=buff;
        bufflen=0;
        buffsize=sizeof(buff)-1;
        va_start(args,format);
        ret=vsnprintf(buffpos+bufflen,buffsize-bufflen,format,args);
        va_end (args);
        if (ret < 0 || ret >= (buffsize-bufflen))
        {
                while (1)
                {
                        buffpos=vrealloc(buffpos,2*buffsize+1,buffsize,buffpos!=buff);
                        if(!buffpos)
                        {
                                lprintf(0,"vrealloc error\n");
                                return(-1);
                        }
                        buffsize*=2;
                        va_start(args,format);
                        ret = vsnprintf(buffpos+bufflen,buffsize-bufflen,format,args);
                        va_end (args);
                        if (ret > -1 && ret < (buffsize-bufflen))
                                break;
                }
        }
        bufflen+=ret;
        *(buffpos+bufflen)=0;
        if(buffpos==buff)
        {
                buffpos=buff2String1(buff,bufflen);
        }
        *fbuff=buffpos;
        return(bufflen);
}

void showarray(unsigned char *packp,int packlen,char hdflag,char base)
{
        int i;
        DNS6_DEBUG(0, "enter showarray");
        if(hdflag)
                lprintf(7,"==================== ARRAY[%d] ====================\n",packlen);
        for(i=0;i<packlen;i++)
        {
                switch(base)
                {
                        case 16:
                                if(i&&!(i%16))
                                        lprintf(7,"\n");
                                lprintf(7,"%02x ",packp[i]);
                                break;
                        case 10:
                                if(i&&!(i%16))
                                        lprintf(7,"\n");
                                lprintf(7,"%03d ",packp[i]);
                                break;
                        case 2:
                        {
                                unsigned char n,mask=0x80;
                                if(i&&!(i%4))
                                        lprintf(7,"\n");
                                for(n=0;n<8;n++)
                                {
                                        if(n&&!(n%4))
                                                lprintf(7,",");
                                        if(packp[i]&mask)
                                                lprintf(7,"1");
                                        else
                                                lprintf(7,"0");
                                        mask>>=1;
                                }
                                lprintf(7," ");
                                break;
                        }
                        default:
                                lprintf(7,"%c",packp[i]);
                                break;
                }
        }
        if(base==16)
                lprintf(7,"\n");
}

int getrecord(char *linebuf,int num,char *field,char gech)
{
        char *fieldstart=linebuf,*fieldend;
        int linelen=strlen(linebuf),fieldlen,i,yhnum,xyhnum,khnum;

        DNS6_DEBUG(0, "enter getrecord");
        if(gech=='\t')
                gech=' ';
        for(i=0;i<linelen;i++)
                if(*(linebuf+i)=='\t')
                        *(linebuf+i)=' ';
        while(*fieldstart==gech)
                fieldstart++;
        if(*fieldstart==0)
        {
                *field=0;
                return(-1);
        }
        while(num>0)
        {
                fieldend=strchr(fieldstart,gech);
                if(fieldend==NULL)
                {
                        *field=0;
                        return(-1);
                }
                for(i=1,yhnum=0,xyhnum=0,khnum=0;i<fieldend-fieldstart;i++)
                {
                        if(fieldstart[i]=='"')
                        {
                                if(fieldstart[i-1]!='\\')
                                        yhnum++;
                                else
                                        if(!(yhnum%2))
                                                xyhnum++;
                        }
                        else if(fieldstart[i]=='(')
                                khnum++;
                        else if(fieldstart[i]==')')
                                khnum--;
                }
                if(fieldstart[0]=='"')
                        yhnum++;
                else if(fieldstart[0]=='(')
                        khnum++;
                else if(fieldstart[0]==')')
                        khnum--;
                while((yhnum%2)||(xyhnum%2)||(khnum))
                {
                        while(*++fieldend==gech);
                        fieldend=strchr(fieldend,gech);
                        if(fieldend==NULL)
                        {
                                *field=0;
                                return(-1);
                        }
                        for(i=1,yhnum=0,xyhnum=0,khnum=0;i<fieldend-fieldstart;i++)
                        {
                                if(fieldstart[i]=='"')
                                {
                                        if(fieldstart[i-1]!='\\')
                                                yhnum++;
                                        else
                                                if(!(yhnum%2))
                                                        xyhnum++;
                                }
                                else if(fieldstart[i]=='(')
                                        khnum++;
                                else if(fieldstart[i]==')')
                                        khnum--;
                        }
                        if(fieldstart[0]=='"')
                                yhnum++;
                        else if(fieldstart[0]=='(')
                                khnum++;
                        else if(fieldstart[0]==')')
                                khnum--;
                }
                while(*++fieldend==gech);
                fieldstart=fieldend;
                num--;
        }
        fieldend=strchr(fieldstart,gech);
        if(fieldend!=NULL)
        {
                for(i=1,yhnum=0,xyhnum=0,khnum=0;i<fieldend-fieldstart;i++)
                {
                        if(fieldstart[i]=='"')
                        {
                                if(fieldstart[i-1]!='\\')
                                        yhnum++;
                                else
                                        if(!(yhnum%2))
                                                xyhnum++;
                        }
                        else if(fieldstart[i]=='(')
                                khnum++;
                        else if(fieldstart[i]==')')
                                khnum--;
                }
                if(fieldstart[0]=='"')
                        yhnum++;
                else if(fieldstart[0]=='(')
                        khnum++;
                else if(fieldstart[0]==')')
                        khnum--;
                while((yhnum%2)||(xyhnum%2)||(khnum))
                {
                        while(*++fieldend==gech);
                        fieldend=strchr(fieldend,gech);
                        if(fieldend==NULL)
                        {
                                fieldend=&fieldstart[strlen(fieldstart)];
                                break;
                        }
                        for(i=1,yhnum=0,xyhnum=0,khnum=0;i<fieldend-fieldstart;i++)
                        {
                                if(fieldstart[i]=='"')
                                {
                                        if(fieldstart[i-1]!='\\')
                                                yhnum++;
                                        else
                                                if(!(yhnum%2))
                                                        xyhnum++;
                                }
                                else if(fieldstart[i]=='(')
                                        khnum++;
                                else if(fieldstart[i]==')')
                                        khnum--;
                        }
                        if(fieldstart[0]=='"')
                                yhnum++;
                        else if(fieldstart[0]=='(')
                                khnum++;
                        else if(fieldstart[0]==')')
                                khnum--;
                }
        }
        else
                fieldend=&fieldstart[strlen(fieldstart)];
        fieldlen=fieldend-fieldstart;
        if(!fieldlen)
        {
                *field=0;
                return(-1);
        }
        memcpy(field,fieldstart,fieldlen);
        field[fieldlen]=0;
        return(fieldlen);
}

int filegetfield(FILE *fp,char *fieldname,char *fieldvalue,char septor)
{
        unsigned char fieldnamebuf[256],linebuf[256];
        int len;
        DNS6_DEBUG(0, "enter filegetfield");
        fseek(fp,0,SEEK_SET);
        while(fgets(linebuf,sizeof(linebuf),fp)!=NULL)
        {
                if(linebuf[0]=='#')
                        continue;
                len=strlen(linebuf);
                if(linebuf[len-1]=='\n')
                {
                        len--;
                        linebuf[len]=0;
                }
                if(linebuf[len-1]=='\r')
                {
                        len--;
                        linebuf[len]=0;
                }
                if(getrecord(linebuf,0,fieldnamebuf,septor)>0)
                {
                        if(!strncmp(fieldnamebuf,fieldname,strlen(fieldname)))
                        {
                                return(getrecord(linebuf,1,fieldvalue,septor));
                        }
                }
        }
        return(0);
}

int getcmdoption(int argc,char *argv[],char *optionname,char *optionvalue)
{
        int i;
        DNS6_DEBUG(0, "enter getcmdoption");
        for(i=1;i<argc;i++)
        {
                if(!strcmp(argv[i],optionname))
                {
                        if(optionvalue)
                        {
                                if(argc<=(i+1)||strlen(argv[i+1])>=256)
                                {
                                        *optionvalue=0;
                                        return(0);
                                }
                                strcpy(optionvalue,argv[i+1]);
                        }
                        return(0);
                }
        }
        return(-1);
}

int maxfd(fd_set fdset)
{
        fd_set settmpt;
        int setbuf[sizeof(fd_set)/sizeof(int)],i,j,max=0;
        DNS6_DEBUG(0, "enter maxfd");
        memcpy(setbuf,&fdset,sizeof(fdset));
        for(i=0;i<sizeof(fd_set)/sizeof(int);i++)
        {
                if(!setbuf[i])
                        continue;
                for(j=0;j<sizeof(int)*8;j++)
                {
                        if(setbuf[i]&1)
                                max=i*sizeof(int)*8+j;
                        setbuf[i]>>=1;
                }
        }
        return(max);
}

int bindsock6(int family,char *protocol,char *host,char *service)
{
        int listenfd;
        struct addrinfo hints,*res,*ressave;
        DNS6_DEBUG(0, "enter bindsock6");
        memset(&hints,0,sizeof(struct addrinfo));
        hints.ai_flags=AI_PASSIVE;
        hints.ai_family=family;
        if(strcasecmp(protocol,"UDP"))
                hints.ai_socktype=SOCK_STREAM;
        else
                hints.ai_socktype=SOCK_DGRAM;
        if(getaddrinfo(host,service,&hints,&res))
        {
                lprintf(0,"getaddrinfo error\n");
                return(-1);
        }
        ressave=res;
        do
        {
                listenfd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
                if(listenfd<0)
                        continue;
                if(!bind(listenfd,res->ai_addr,res->ai_addrlen))
                        break;
                else
                        close(listenfd);
        }
        while((res=res->ai_next)!=NULL);
        if(res==NULL)
        {
                lprintf(0,"res is NULL\n");
                freeaddrinfo(ressave);
                return(-1);
        }
        freeaddrinfo(ressave);
        if((hints.ai_socktype==SOCK_STREAM)&&(listen(listenfd,5)<0))
        {
                close(listenfd);
                lprintf(0,"listen error\n");
                return(-1);
        }
        return(listenfd);
}

sigjmp_buf env_clisock6;
void catchalrm_clisock6(int signo)
{
        DNS6_DEBUG(0, "enter catchalrm_clisock6");
        signal(SIGALRM,SIG_IGN);
        alarm(0);
        siglongjmp(env_clisock6,1);
}
int clisock6(int family,char *protocol,char *host,char *service,char *bindname,int secs)
{
        int sockfd=0,ret;
        struct addrinfo hints,*res,*ressave;

        DNS6_DEBUG(0, "enter clisock6");
        signal(SIGALRM,catchalrm_clisock6);
        memset(&hints,0,sizeof(struct addrinfo));
        hints.ai_family=family;
        if(strcasecmp(protocol,"UDP"))
                hints.ai_socktype=SOCK_STREAM;
        else
                hints.ai_socktype=SOCK_DGRAM;

        if(sigsetjmp(env_clisock6,SIGALRM)==0)
        {
                alarm(secs);
                if(getaddrinfo(host,service,&hints,&res))
                {
                        lprintf(0,"getaddrinfo error\n");
                        alarm(0);
                        return(-1);
                }
                ressave=res;
                do
                {
                        sockfd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
                        if(sockfd<0)
                                continue;
                        if(bindname&&strlen(bindname)>0)
                        {
                                if(res->ai_family==AF_INET6)
                                {
                                        struct sockaddr_in6 *sin6=(struct sockaddr_in6 *)res->ai_addr;
                                        sin6->sin6_scope_id=if_nametoindex(bindname);
                                }
                                else
                                {
                                        struct sockaddr_in *psin;
                                        struct ifreq ifr;
                                        strncpy(ifr.ifr_name,bindname,sizeof(ifr.ifr_name)-1);
                                        if(!ioctl(sockfd,SIOCGIFADDR,&ifr))
                                        {
                                                psin=(struct sockaddr_in *)&(ifr.ifr_addr);
                                                if(bind(sockfd,(struct sockaddr *)psin,sizeof(*psin)<0))
                                                {
                                                       lprintf(0,"bind error\n");
                                                       alarm(0);
                                                       return (-1);
                                                }
                                        }
                                }
                        }
                        ret=connect(sockfd,res->ai_addr,res->ai_addrlen);
                        alarm(0);
                        if(!ret)
                                break;
                        else
                        {
                                close(sockfd);
                                sockfd=0;
                        }
                }
                while((res=res->ai_next)!=NULL);
        }
        else
        {
                if(sockfd)
                        close(sockfd);
                return(-1);
        }
        if(res==NULL)
        {
                lprintf(0,"Cannot connect %s:%s\n",host,service);
                freeaddrinfo(ressave);
                return(-1);
        }
        freeaddrinfo(ressave);
        return(sockfd);
}

int sendbuff(int sockid,unsigned char *buff,int bufflen)
{
        int nleft,ret;
        DNS6_DEBUG(0, "enter sendbuff");
        nleft=bufflen;
        while(nleft>0)
        {
                ret=write(sockid,buff,nleft);
                if(ret<0)
                {
                        if(errno==EINTR)
                                ret=0;
                        else
                                break;
                }
                nleft-=ret;
                buff+=ret;
        }
        return (bufflen-nleft);
}

int recvbuff(int sockid,unsigned char *buff,int bufflen,char onlyone)
{
        int nleft,ret;
        DNS6_DEBUG(0, "enter recvbuff");
        nleft=bufflen;
        while(nleft>0)
        {
                ret=read(sockid,buff,nleft);
                lprintf(0,"ret=%d\n",ret);
                if(ret<0)
                {
                        if(errno==EINTR)
                                ret=0;
                        else
                                break;
                }
                else if(ret==0)
                        break;
                nleft-=ret;
                buff+=ret;
                if(onlyone)
                        break;
        }
        return (bufflen-nleft);
}

sigjmp_buf env_dnsRecvPkg;
void catchalrm_dnsRecvPkg(int signo)
{
        DNS6_DEBUG(0, "enter catchalrm_dnsRecvPkg");
        signal(SIGALRM,SIG_IGN);
        alarm(0);
        siglongjmp(env_dnsRecvPkg,1);
}
int dnsRecvPkg(int skid,unsigned char **buff,int *bufflen,int secs)
{
        unsigned short plen;
        unsigned char *pbuf=NULL;
        int ret,type=0,len=4;
        DNS6_DEBUG(0, "enter dnsRecvPkg");
        if(getsockopt(skid,SOL_SOCKET,SO_TYPE,&type,(socklen_t *)&len)!=0)
                return(-1);
        signal(SIGALRM,catchalrm_dnsRecvPkg);
        if(sigsetjmp(env_dnsRecvPkg,SIGALRM)==0)
        {
                alarm(secs);
                if(type==1)
                {
                        ret=recvbuff(skid,(unsigned char *)&plen,sizeof(plen),0);
                        if(ret!=sizeof(plen))
                        {
                                lprintf(0,"recvbuff error\n");
                                alarm(0);
                                return(-1);
                        }
                        plen=ntohs(plen);
                        pbuf=(unsigned char *)vmalloc(plen);
                        if(pbuf==NULL)
                        {
                                lprintf(0,"vmalloc error\n");
                                alarm(0);
                                return(-1);
                        }
                        ret=recvbuff(skid,pbuf,plen,0);
                        if(ret!=plen)
                        {
                                lprintf(0,"recvbuff error\n");
                                alarm(0);
                                vfree(pbuf);
                                return(-1);
                        }
                }
                else
                {
                        struct sockaddr_in addr;
                        int addrlen=sizeof(addr);
                        plen=1024;
                        pbuf=(unsigned char *)vmalloc(plen);
                        if(pbuf==NULL)
                        {
                                lprintf(0,"vmalloc error\n");
                                alarm(0);
                                return(-1);
                        }
                        //addr.sin6_family=AF_INET6;
                        ret=recvfrom(skid,pbuf,plen,0,(struct sockaddr *)&addr,(socklen_t *)&addrlen);
                        if(ret<=0)
                        {
                                lprintf(0,"recvbuff error\n");
                                alarm(0);
                                vfree(pbuf);
                                return(-1);
                        }
                        plen=ret;
                        //connect(skid,(struct sockaddr *)&addr,addrlen);
                }
                alarm(0);
        }
        else
        {
                DNS6_DEBUG(0,"recv timeout");
                if(pbuf)
                {
                        vfree(pbuf);
                }
                return(-1);
        }
        if(buff)
                *buff=pbuf;
        if(bufflen)
                *bufflen=plen;
        return(0);
}

int dnsSendPkg(int skid,unsigned char *buff,int bufflen)
{
        unsigned short plen;
        int ret,type=0,len=4;
        DNS6_DEBUG(0, "enter dnsSendPkg");
        if(getsockopt(skid,SOL_SOCKET,SO_TYPE,&type,(socklen_t *)&len)!=0)
                return(-1);
        if(type==1)
        {
                plen=bufflen;
                plen=htons(plen);
                ret=sendbuff(skid,(unsigned char *)&plen,sizeof(plen));
                if(ret!=sizeof(plen))
                {
                        lprintf(0,"sendbuff error\n");
                        return(-1);
                }
        }
        ret=sendbuff(skid,buff,bufflen);
        if(ret!=bufflen)
        {
                lprintf(0,"sendbuff error(%s)\n",strerror(errno));
                return(-1);
        }
        return(0);
}

int dnsQuery(int family,char *protocol,char *server,char *domain,unsigned short type,int secs,unsigned char *src,int srclen,unsigned char **dst,int *dstlen)
{
        unsigned short plen;
        int ret,skid,qbufflen,len;
        unsigned char *qbuff=NULL,*pos;
        struct dns_node *dnsnode;

        DNS6_DEBUG(0, "enter dnsQuery");
        if(dst)
                *dst=NULL;
        if(dstlen)
                *dstlen=0;
        skid=clisock6(family,protocol,server,"53",NULL,secs);
        if(skid<0)
        {
                lprintf(0,"clisock6 error\n");
                return(-1);
        }
        if(domain)
        {
                ret=dnsQueryPkg(domain,type,&qbuff,&qbufflen);
                if(ret!=0)
                {
                        lprintf(0,"dnsQueryPkg error\n");
                        close(skid);
                        return(-1);
                }
                if(strcmp(protocol,"TCP")==0)
                {
                        pos=qbuff;
                        len=qbufflen;
                }
                else
                {
                        pos=qbuff+2;
                        len=qbufflen-2;
                }
        }
        else
        {
                pos=src;
                len=srclen;
        }
        ret=dnsSendPkg(skid,pos,len);
        if(qbuff)
                vfree(qbuff);
        if(ret!=0)
        {
                lprintf(0,"sendbuff error\n");
                close(skid);
                return(-1);
        }
        ret=dnsRecvPkg(skid,&qbuff,&qbufflen,secs);
        if(ret!=0)
        {
                //lprintf(0,"dnsRecvPkg error\n");
                close(skid);
                return(-1);
        }
        if(dst)
                *dst=qbuff;
        if(dstlen)
                *dstlen=qbufflen;
/*
        if(domain)
        {
                ret=dnsParse(qbuff,qbufflen,&dnsnode);
                vfree(qbuff);
                if(ret!=0)
                {
                        lprintf(0,"dnsParse error\n");
                        close(skid);
                        return(-1);
                }
                dnsNodeShow(dnsnode);
                dnsNodeFree(dnsnode);
        }
        else
        {
                if(dst)
                        *dst=qbuff;
                if(dstlen)
                        *dstlen=qbufflen;
        }
*/
        close(skid);
        return(0);
}

int dnsQueryGet(unsigned char *pkg,int pkglen,unsigned short type,void *result)
{
        struct dns_node *dnsnode;
        struct dns_rr_node *rrnode;
        int ret;
        DNS6_DEBUG(0, "enter dnsQueryGet");
        if(pkg==NULL||pkglen<=0||result==NULL||(type!=ns_t_a&&type!=ns_t_aaaa))
        {
                lprintf(0,"arguments error\n");
                return(-1);
        }
        if(dnsParse(pkg,pkglen,&dnsnode)==0)
        {
                for(rrnode=dnsnode->an;rrnode;rrnode=rrnode->next)
                {
                        if(rrnode->type==type)
                                break;
                }
                if(rrnode)
                {
                        if(type==ns_t_aaaa)
                                ret=inet_pton6(rrnode->rrascii,result);
                        else
                                ret=inet_pton4(rrnode->rrascii,result);
                        if(ret==1)
                                ret=0;
                        else
                                ret=-1;
                }
                else
                        ret=-1;
        }
        else
                ret=-1;
        vfree(dnsnode);
        return(ret);
}

int dnsTcpHandle(int skid)
{
        unsigned char *src,*dst;
        char *proto;
        int ret,srclen,dstlen,len=4;
        struct dns_node *dnsnode;

        DNS6_DEBUG(0, "enter dnsTcpHandle");
        skid=accept(skid,NULL,NULL);
        if(skid<0)
        {
                lprintf(0,"accept error(%s)\n",strerror(errno));
                return(-1);
        }
        ret=dnsRecvPkg(skid,&src,&srclen,0);
        if(ret<0)
        {
                lprintf(0,"dnsRecvPkg error\n");
                close(skid);
                return(-1);
        }
        proto="TCP";
        ret=dnsQuery(0,proto,DNSSERVER,NULL,0,3,src,srclen,&dst,&dstlen);
        if(loglevel)
        {
                struct sockaddr_in addr;
                struct dns_node *dnsnode;
                int addrlen=sizeof(addr);
                
                if(ret==0&&dnsParse(dst,dstlen,&dnsnode)==0)
                {
                        if(0 == getpeername(skid,(struct sockaddr *)&addr,(socklen_t *)&addrlen))
                        {
                              lprintf(5,"%u.%u.%u.%u request %s %d %d to %s\n",NIPQUAD(addr.sin_addr.s_addr),dnsnode->qd->name,dnsnode->qd->type,dnsnode->qd->class,DNSSERVER);
                        }
                        dnsNodeShow(dnsnode);
                        dnsNodeFree(dnsnode);
                }
        }
        vfree(src);
        if(ret!=0)
        {
                lprintf(0,"dnsQuery error\n");
                close(skid);
                return(-1);
        }
        ret=dnsSendPkg(skid,dst,dstlen);
        vfree(dst);
        close(skid);
        if(ret!=0)
        {
                lprintf(0,"dnsSendPkg error\n");
                return(-1);
        }
        return(0);
}

int dnsUdpHandle(int skid)
{
        struct dns_node *dnsnode,*dnsnode_src;
        struct dns_quest_node *questnode;
        struct dns_rr_node *rrnode;
        struct sockaddr_in addr;
        unsigned char *src,*dst,*dst4,*dst6;
        char *proto;
        int ret,srclen,dstlen,addrlen,dst4len,dst6len,ret4,ret6;
        struct in6_addr in6addr;
        unsigned int in4addr;

        DNS6_DEBUG(0, "enter dnsUdpHandle");
        addrlen=sizeof(addr);
        srclen=1024;
        src=(unsigned char *)vmalloc(srclen);
        if(src==NULL)
        {
                lprintf(0,"vmalloc error\n");
                return(-1);
        }
        ret=recvfrom(skid,src,srclen,0,(struct sockaddr *)&addr,(socklen_t *)&addrlen);
        if(ret<=0)
        {
                lprintf(0,"recvfrom error(%s)\n",strerror(errno));
                vfree(src);
                return(-1);
        }
        srclen=ret;
        if(dnsParse(src,srclen,&dnsnode_src)!=0||dnsnode_src->qd==NULL)
        {
                lprintf(0,"dnsParse error\n");
                vfree(src);
                dnsNodeFree(dnsnode_src);
                return(-1);
        }
        questnode=dnsnode_src->qd;

        if(loglevel)
        {
                lprintf(5,"%u.%u.%u.%u request %s %d %d to %s %d\n",NIPQUAD(addr.sin_addr.s_addr),dnsnode_src->qd->name,dnsnode_src->qd->type,dnsnode_src->qd->class,DNSSERVER,srclen);
//                dnsNodeShow(dnsnode);
//                dnsNodeFree(dnsnode);
        }

        proto="UDP";
        if(BIHSTATUS==0||dnsnode_src->qd->type!=ns_t_a||(NETWORKTYPE!=2&&NETWORKTYPE!=3))
        {
                ret=dnsQuery(0,proto,DNSSERVER,NULL,0,3,src,srclen,&dst,&dstlen);
                if(loglevel)
                {
                        if(ret==0&&dnsParse(dst,dstlen,&dnsnode)==0)
                        {
                                lprintf(3,"%u.%u.%u.%u request %s %d %d to %s %d\n",NIPQUAD(addr.sin_addr.s_addr),dnsnode->qd->name,dnsnode->qd->type,dnsnode->qd->class,DNSSERVER,srclen);
                                dnsNodeShow(dnsnode);
                                dnsNodeFree(dnsnode);
                        }
                }
                dnsNodeFree(dnsnode_src);
                vfree(src);
                if(ret!=0)
                {
                        DNS6_DEBUG(0,"dnsQuery error\n");
                        return(-1);
                }
                ret=sendto(skid,dst,dstlen,0,(struct sockaddr *)&addr,addrlen);
                vfree(dst);
                if(ret!=dstlen)
                {
                        DNS6_DEBUG(0,"sendto error(%s)\n",strerror(errno));
                        return(-1);
                }
                return(0);
        }
        ret4=dnsQuery(0,proto,DNSSERVER,dnsnode_src->qd->name,ns_t_a,3,NULL,0,&dst4,&dst4len);
        if(ret4==0)
        {
                ret4=dnsQueryGet(dst4,dst4len,ns_t_a,&in4addr);
                vfree(dst4);
        }
        if(ret4!=0)
                lprintf(3,"dnsQuery A for %s fail\n",dnsnode_src->qd->name);
        else
                lprintf(3,"dnsQuery A(%u.%u.%u.%u) for %s\n",NIPQUAD(in4addr),dnsnode_src->qd->name);
        ret6=dnsQuery(0,proto,DNSSERVER,dnsnode_src->qd->name,ns_t_aaaa,3,NULL,0,&dst6,&dst6len);
        if(ret6==0)
        {
                ret6=dnsQueryGet(dst6,dst6len,ns_t_aaaa,in6addr.s6_addr);
                vfree(dst6);
        }
        if(ret6!=0)
                lprintf(0,"dnsQuery AAAA for %s fail\n",dnsnode_src->qd->name);
        else
        {
                char tmpbuf[256];
                tmpbuf[0]=0;
                inet_ntop6(in6addr.s6_addr,tmpbuf,sizeof(tmpbuf)-1);
                lprintf(3,"dnsQuery AAAA(%s) for %s\n",tmpbuf,dnsnode_src->qd->name);
        }
        if(ret4!=0&&ret6!=0)
        {
                lprintf(0,"dnsQuery(%s) error\n",dnsnode_src->qd->name);
                dnsNodeFree(dnsnode_src);
                vfree(src);
                return(-1);
        }
        lprintf(3,"%u.%u.%u.%u get ipv4(%s)/ipv6(%s) from %s for %s\n",
                NIPQUAD(addr.sin_addr.s_addr),ret4==0?"S":"F",ret6==0?"S":"F",DNSSERVER,dnsnode_src->qd->name);
        if(ret4!=0)
        {
                in4addr=in4addrGet(in6addr);
                if(in4addr==0)
                {
                        lprintf(0,"in4addrGet error\n");
                        dnsNodeFree(dnsnode_src);
                        vfree(src);
                        return(-1);
                }
        }
        else
                map_clear(in4addr);
        questnode->addr=in4addr;
        dnsnode_src->flag|=0x8080;
        ret=dnsEncode(dnsnode_src,&dst,&dstlen);
        dnsNodeFree(dnsnode_src);
        vfree(src);
        if(ret==0)
        {
                ret=sendto(skid,dst+2,dstlen-2,0,(struct sockaddr *)&addr,addrlen);
                vfree(dst);
                if(ret!=(dstlen-2))
                {
                        lprintf(0,"sendto error(%s)\n",strerror(errno));
                        return(-1);
                }
                return(0);
        }
        else
        {
                lprintf(0,"dnsEncode error\n");
                return(-1);
        }
}

int bihmodeGet(void)
{
        int ret,slen;
        char buff[128];
        FILE *fp;

        DNS6_DEBUG(0, "enter bihmodeGet");
        if(fileExist("/proc/bih/mode")!=0)
        {
                DNS6_DEBUG(0, "/proc/bih/mode is not exist");
                BIHMODE=0;
                BIHSTATUS=0;
                return(-1);
        }
        fp=popen("cat /proc/bih/mode","r");
        if(fp==NULL)
        {
                DNS6_DEBUG(0, "/proc/bih/mode open failed");
                return(-1);
        }
        if(fgets(buff,sizeof(buff)-1,fp)!=NULL)
        {
                pclose(fp);
                slen=strlen(buff);
                if(buff[slen-1]=='\n')
                {
                        slen--;
                        buff[slen]=0;
                }
                if(buff[slen-1]=='\r')
                {
                        slen--;
                        buff[slen]=0;
                }
                if(strncmp(buff,"NULL",4)==0)
                {
                        BIHMODE=0;
                        BIHSTATUS=0;
                }
                else if(strncmp(buff,"BIS",3)==0)
                {
                        BIHMODE=1;
                        BIHSTATUS=1;
                }
                else if(strncmp(buff,"BIA",3)==0)
                {
                        BIHMODE=2;
                        BIHSTATUS=1;
                }
        }
        else
        {
                pclose(fp);
                DNS6_DEBUG(0, "/proc/bih/mode fgets failed");
                return(-1);
        }
        return(0);
}

unsigned int bih_private_get(void)
{
        int ret,slen;
        char buff[128];
        FILE *fp;
        DNS6_DEBUG(0, "enter bih_private_get");
        if(fileExist("/proc/bih/private")!=0)
                return(-1);
        fp=popen("cat /proc/bih/private","r");
        if(fp==NULL)
        {
                return(-1);
        }
        if(fgets(buff,sizeof(buff)-1,fp)!=NULL)
        {
                pclose(fp);
                slen=strlen(buff);
                if(buff[slen-1]=='\n')
                {
                        slen--;
                        buff[slen]=0;
                }
                if(buff[slen-1]=='\r')
                {
                        slen--;
                        buff[slen]=0;
                }
                PRIVATE=inet_addr(buff);
        }
        else
        {
                pclose(fp);
                return(-1);
        }
        return(0);
}


#ifdef ANDROID_CHANGES
int getcfg(void)
#else
int getcfg(char *cfgfile)
#endif
{
        static unsigned long mtime=0;
        struct stat statnode;
        char fieldvalue[256];
        int oldtype=NETWORKTYPE;
        FILE *fp;

        lprintf(0, "enter getcfg");
        bih_private_get();
        NETWORKTYPE=networktype();
        bihmodeGet();
        if(NETWORKTYPE==2||NETWORKTYPE==3)
        {
                if(oldtype!=NETWORKTYPE)
                        map_clear(0);
                if(BIHSTATUS==0)
                        bih_turn(1);
        }
        else
        {
                if(BIHSTATUS==1)
                        bih_turn(0);
        }
        lprintf(3,"===========================================\n");
        lprintf(3,"NAMESERVER:  %s\n",DNSSERVER);
        lprintf(3,"LOGLEVEL:    %d\n",loglevel);
        lprintf(3,"NETWORK:     %s\n",networktype_name(NETWORKTYPE));
        lprintf(3,"BIH:        %s\n",BIHSTATUS?"OPEN":"CLOSE");
        lprintf(3,"PRIVATE:     %u.%u.%u.%u\n",NIPQUAD(PRIVATE));
#ifdef ANDROID_CHANGES
#if 0   // Android 4.4 net.dns1 could not change the system dns.
        if(snprintf(pidpropname,sizeof(pidpropname),"net.dns1") >= PROPERTY_KEY_MAX)
               exit(EXIT_FAILURE);
        property_set(pidpropname, "127.0.0.1");
        //check the DNSSERVER if is the 127.0.0.1, if it is not check the dns2
#endif
#else
        if(stat(cfgfile, &statnode)!=0)
        {
                return(-1);
        }
        if(mtime==statnode.st_mtime)
                return(0);
        if((fp=fopen(cfgfile,"r"))==NULL)
        {
                return(-1);
        }
        if(filegetfield(fp,"nameserver",fieldvalue,' '))
                strncpy(DNSSERVER,fieldvalue,sizeof(DNSSERVER)-1);
        if(filegetfield(fp,"log",fieldvalue,' '))
                loglevel=atoi(fieldvalue);
        fclose(fp);
        if(strcmp(DNSSERVER,"127.0.0.1")&&fileLen("/system/etc/resolv.conf")>0)
        {
                rename("/system/etc/resolv.conf","/system/etc/resolv.conf.dns6");
        }
        fp=fopen("/system/etc/resolv.conf","w");
        if(fp)
        {
                fprintf(fp,"nameserver 127.0.0.1\n");
                fclose(fp);
        }
        if(stat(cfgfile, &statnode)!=0)
        {
                return(-1);
        }
        mtime=statnode.st_mtime;
#endif
        return(0);
}

void catchhup(int signo)
{
        DNS6_DEBUG(0, "enter catchhup");
#ifdef ANDROID_CHANGES
        getcfg();
#else
        getcfg(CONFFILE);
#endif
        bihmodeGet();
        signal(SIGHUP,catchhup);
}

void catchexit(int signo)
{
        DNS6_DEBUG(0, "enter catchexit");
#ifdef ANDROID_CHANGES
#if 0 // Android 4.4 net.dns1 could not change the system dns.
    if(snprintf(pidpropname,sizeof(pidpropname),"net.dns1") >= PROPERTY_KEY_MAX)
        exit(EXIT_FAILURE);
        property_set(pidpropname, "");
    //reset the DNS1
#endif
#else
        rename("/system/etc/resolv.conf.dns6","/system/etc/resolv.conf");
#endif
        closelog();
        exit(0);
}

int main(int argc,char *argv[])
{
        int skid,tcpid,udpid,nfds,maxnfds;
        int maxreadfd,maxwritefd,i;
        char level[32];
        struct timeval tm;

        if(getcmdoption(argc,argv,"--help",NULL)==0)
        {
#ifdef ANDROID_CHANGES
                fprintf(stderr,"Usage: %s [--help][-s server][-d][-v level]\n",argv[0]);
#else
                fprintf(stderr,"Usage: %s [--help][-s server][-d][-v level][-f file]\n",argv[0]);
#endif
                exit(1);
        }
        lprintf(5,"\This is version 1.0.0 for Internet IPv6 domain system\n");
        lprintf(5,"\Copyrighted (C) 2010,2011 by the China Mobile Communications \n");
        lprintf(5,"\http://code.google.com/p/bump-in-the-host/source/ \n");
        memset(&sockreadset,0,sizeof(fd_set));
        memset(&sockwriteset,0,sizeof(fd_set));

        signal(SIGHUP,catchhup);
        signal(SIGINT,catchexit);
        signal(SIGQUIT,catchexit);
        signal(SIGTERM,catchexit);
#ifdef ANDROID_CHANGES
        // we can not sure this could run ok...
#else
        getcmdoption(argc,argv,"-f",CONFFILE);
        getcfg(CONFFILE);
#endif
        getcmdoption(argc,argv,"-s",DNSSERVER);
    #ifdef ANDROID_CHANGES
        // add to set the DNS
        if(property_get("net.veth_spi4.ipv6_dns1", DNSSERVER, "")==0 &&
          property_get("net.veth_spi3.ipv6_dns1", DNSSERVER, "")==0 &&
          property_get("net.veth_spi2.ipv6_dns1", DNSSERVER, "")==0 &&
          property_get("net.veth_spi1.ipv6_dns1", DNSSERVER, "")==0 &&
          property_get("net.veth_spi0.ipv6_dns1", DNSSERVER, "")==0 &&
          property_get("net.veth_sdio4.ipv6_dns1", DNSSERVER, "") == 0 &&
          property_get("net.veth_sdio3.ipv6_dns1", DNSSERVER, "") == 0 &&
          property_get("net.veth_sdio2.ipv6_dns1", DNSSERVER, "") == 0 &&
          property_get("net.veth_sdio1.ipv6_dns1", DNSSERVER, "") == 0 &&
          property_get("net.veth_sdio0.ipv6_dns1", DNSSERVER, "") == 0)
          {    char googledns6[21] = "2001:4860:4860::8888";
              lprintf(5, "\veth_spi or veth_sdio not found use the default DNSSERVER\n");
              memset(DNSSERVER, 0, sizeof(DNSSERVER) );
              memcpy(DNSSERVER, googledns6, sizeof(googledns6));
          }
#endif
        if(getcmdoption(argc,argv,"-d",NULL)==0)
                daemonflag=1;
        else
                daemonflag=0;
        if(getcmdoption(argc,argv,"-v",level)==0)
        {
                loglevel=atoi(level);
        }
        //openlog(argv[0],LOG_NDELAY,LOG_USER);
        if(daemonflag)
                daemon(1,1);
        tcpid=bindsock6(AF_INET,"TCP","0.0.0.0","53");
        if(tcpid<=0)
        {
                lprintf(0,"bindsock6 error\n");
#ifdef ANDROID_CHANGES
        //reverse the modification
#else
                rename("/system/etc/resolv.conf.dns6","/system/etc/resolv.conf");
#endif
                return(-1);
        }
        FD_SET(tcpid,&sockreadset);
        udpid=bindsock6(AF_INET,"UDP","0.0.0.0","53");
        if(udpid<=0)
        {
                lprintf(0,"bindsock6 error\n");
#ifdef ANDROID_CHANGES
        //reverse the modification
#else
                rename("/system/etc/resolv.conf.dns6","/system/etc/resolv.conf");
#endif
                return(-1);
        }
        FD_SET(udpid,&sockreadset);

        while(1)
        {
                tm.tv_sec=5;
                tm.tv_usec=0;
                memcpy(&sockreadsettmp,&sockreadset,sizeof(fd_set));
                memcpy(&sockwritesettmp,&sockwriteset,sizeof(fd_set));
                maxreadfd=maxfd(sockreadsettmp);
                maxwritefd=maxfd(sockwritesettmp);
                maxnfds=maxreadfd>maxwritefd?maxreadfd:maxwritefd;
                nfds=select(maxnfds+1,&sockreadsettmp,&sockwritesettmp,NULL,&tm);
                if(nfds<0)
                {
                        lprintf(0, "nfds is smaller than 0, loop");
                        continue;
                }
                else if(nfds==0)
                {
                        lprintf(0, "nfds is 0, loop");
#ifdef ANDROID_CHANGES
                        getcfg();
#else
                        getcfg(CONFFILE);
#endif
                        continue;
                }
                lprintf(1,"----------------------------------------------------------------------\n");
                if(FD_ISSET(tcpid,&sockreadsettmp))
                {
                        dnsTcpHandle(tcpid);
                }
                else if(FD_ISSET(udpid,&sockreadsettmp))
                {
                        dnsUdpHandle(udpid);
                }
        }
}
