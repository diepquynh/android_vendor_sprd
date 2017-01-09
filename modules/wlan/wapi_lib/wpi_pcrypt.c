#include "wpi_pcrypt.h"
#include "sms4c.h"
/*ofb encrypt*/
int wpi_encrypt(unsigned char * pofbiv_in,unsigned char * pbw_in,unsigned int plbw_in,unsigned char * pkey,unsigned char * pcw_out)
{
	unsigned int ofbtmp[4];
	unsigned int * pint0, * pint1;
	unsigned char * pchar0, * pchar1,* pchar2;
	unsigned int counter,comp,i;
	unsigned int prkey_in[32];


	if(plbw_in<1)	return 1;
	//if(plbw_in>65536) return 1;

	SMS4KeyExt(pkey,  prkey_in, 0);

	counter=plbw_in / 16;
	comp=plbw_in % 16;

//get the iv
	SMS4Crypt(pofbiv_in,(unsigned char *)ofbtmp, prkey_in);
	pint0=(unsigned int *)pbw_in;
	pint1=(unsigned int *)pcw_out;
	for(i=0;i<counter;i++) {
		pint1[0]=pint0[0]^ofbtmp[0];
		pint1[1]=pint0[1]^ofbtmp[1];
		pint1[2]=pint0[2]^ofbtmp[2];
		pint1[3]=pint0[3]^ofbtmp[3];
		SMS4Crypt((unsigned char *)ofbtmp,(unsigned char *)ofbtmp, prkey_in);
		pint0+=4;
		pint1+=4;
	}
	pchar0=(unsigned char *)pint0;
	pchar1=(unsigned char *)pint1;
	pchar2=(unsigned char *)ofbtmp;
	for(i=0;i<comp;i++) {
		pchar1[i]=pchar0[i]^pchar2[i];
	}
	
	return 0;	
}

/*ofb decrypt*/
int wpi_decrypt(unsigned char * pofbiv_in,unsigned char * pcw_in,unsigned int plcw_in,unsigned char * prkey_in,unsigned char * pbw_out)
{
	return wpi_encrypt(pofbiv_in,pcw_in,plcw_in,prkey_in,pbw_out);	
}
#if 0
{
	return wpi_encrypt(pofbiv_in,pcw_in,plcw_in,prkey_in,pbw_out);	
}
#endif
/*cbc_mac*/
int wpi_pmac(unsigned char * pmaciv_in,unsigned char * pmac_in,unsigned int pmacpc_in,unsigned char * pkey,unsigned char * pmac_out)
{
	unsigned int  mactmp[4];
	unsigned int i;
	unsigned int * pint0;
	unsigned int prmackey_in[32];

	if(pmacpc_in<1) return 1;
	if(pmacpc_in>4096) return 1;

	SMS4KeyExt(pkey,  prmackey_in, 0);
	
	pint0=(unsigned int *)pmac_in;
	SMS4Crypt(pmaciv_in, (unsigned char *)mactmp, prmackey_in);	
	for(i=0;i<pmacpc_in;i++) {
		mactmp[0]^=pint0[0];
		mactmp[1]^=pint0[1];
		mactmp[2]^=pint0[2];
		mactmp[3]^=pint0[3];
		pint0 += 4;
		SMS4Crypt((unsigned char *)mactmp, (unsigned char *)mactmp, prmackey_in);
	}
	pint0 = (unsigned int *)pmac_out;
	pint0[0] = mactmp[0];
	pint0[1] = mactmp[1];
	pint0[2] = mactmp[2];
	pint0[3] = mactmp[3];

	return 0;
}

