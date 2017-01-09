#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "header.h"

using namespace std;
#define INSERTHEADER 0x200

static void usage(void)
{
    printf("================================================================================== \n");
    printf("Usage: \n");
    printf("$./signImage <cert_path> <image> <scheme_level> <debug_cert> \n");
    printf("---------------------------------------------------------------------------------- \n");
    printf("-cert_path          --The path of certificate \n");
    printf("---------------------------------------------------------------------------------- \n");
    printf("-image              --The image to be signed  \n");
    printf("---------------------------------------------------------------------------------- \n");
    printf("-scheme_level  = 3  --Three-Level SB Certficate Scheme \n");
    printf("               = 2  --Two-Level SB Certficate Scheme \n");
    printf("               = 1  --Only Content Certficate \n");
    printf("---------------------------------------------------------------------------------- \n");
    printf("-debug_cert    = 1  --enable debug cert \n");
    printf("               = 0  --disable debug cert \n");
    printf("================================================================================== \n");
}


int main(int argc, char* argv[])
{
    std::string strkeyCertAFile;
    std::string strkeyCertBFile;
    std::string strkeyCertCFile;
    std::string strContentFile;
    std::string strOutFile;
    std::string strImgFile;
    std::string strImgFile_enc;
    std::string strdebugCertFile;

    fmpreambleheader fmheader;
    memset(&fmheader,0,sizeof(fmpreambleheader));

    FILE *pFilekeyA = NULL;
    FILE *pFilekeyB = NULL;
    FILE *pFilekeyC = NULL;
    FILE *pFileContent = NULL;
    FILE *pFileimg = NULL;
    FILE *pFileimg_enc = NULL;
    FILE *pFiledebugCert = NULL;
    DWORD dwSizekeyA = 0;
    DWORD dwSizekeyB = 0;
    DWORD dwSizekeyC = 0;
    DWORD dwSizeContent = 0;
    DWORD dwSizeimg = 0;
    DWORD dwSizedebugCert = 0;
    DWORD dwTotalSize = 0;
    DWORD dwPaddingLen = 0;
    LPBYTE pBuf = NULL;
    int len;
    int level = 0;
    int deCert = 0;
    char certpath[FILE_NAME_MAX_SIZE] = "0";
    char imagename[FILE_NAME_MAX_SIZE] = "0";
    char imagename_enc[FILE_NAME_MAX_SIZE] = "0";
    char temp[FILE_NAME_MAX_SIZE] = "0";
    char certa[FILE_NAME_MAX_SIZE] = "0";
    char certb[FILE_NAME_MAX_SIZE] = "0";
    char certc[FILE_NAME_MAX_SIZE] = "0";
    char certcnt[FILE_NAME_MAX_SIZE] = "0";
    char certs[FILE_NAME_MAX_SIZE] = "0";
    char *token;
    int enc_enable = 0;
    if (argc != 5) {
        usage();
        return 1;
    }

    memset(certpath,0,FILE_NAME_MAX_SIZE);
    memset(imagename,0,FILE_NAME_MAX_SIZE);
    memset(certa,0,FILE_NAME_MAX_SIZE);
    memset(certb,0,FILE_NAME_MAX_SIZE);
    memset(certc,0,FILE_NAME_MAX_SIZE);
    memset(certcnt,0,FILE_NAME_MAX_SIZE);
    memset(certs,0,FILE_NAME_MAX_SIZE);
    strcpy(certpath,argv[1]);
    strcpy(imagename,argv[2]);
    //find the enc image name .
    strcpy(temp,argv[2]);
    token  = strstr(temp,"-sign");
    strncpy(imagename_enc,temp,token-temp);
    strcat(imagename_enc,"_enc.bin");

    level = atoi(argv[3]);
    deCert = atoi(argv[4]);

    strcpy(certa,certpath);
    strcat(certa,"/certa.bin");

    strcpy(certb,certpath);
    strcat(certb,"/certb.bin");

    strcpy(certc,certpath);
    strcat(certc,"/certc.bin");

    strcpy(certcnt,certpath);
    strcat(certcnt,"/certcnt.bin");

    strcpy(certs,certpath);
    strcat(certs,"/certs.bin");

    if (level == 3) {//Three-Level SB Certficate Scheme
        strkeyCertAFile = certa;
        strkeyCertBFile = certb;
        strContentFile = certcnt;
        strOutFile = imagename;
        strImgFile = imagename;
        strImgFile_enc = imagename_enc;
        strdebugCertFile = certs;

        pFilekeyA = _tfopen(strkeyCertAFile.c_str(),_T("rb"));
        if(pFilekeyA == NULL)
        {
            _tprintf(_T("open keycertA file failed!"));
            return 1;
        }

        fseek(pFilekeyA,0,SEEK_END);
        dwSizekeyA = ftell(pFilekeyA);
        fseek(pFilekeyA,0,SEEK_SET);
        pFilekeyB = _tfopen(strkeyCertBFile.c_str(),_T("rb"));
        if(pFilekeyB == NULL)
        {
            _tprintf(_T("open keycertB file failed!"));
            return 1;
        }

        fseek(pFilekeyB,0,SEEK_END);
        dwSizekeyB = ftell(pFilekeyB);
        fseek(pFilekeyB,0,SEEK_SET);

        pFileContent = _tfopen(strContentFile.c_str(),_T("rb"));
        if(pFileContent == NULL)
        {
            _tprintf(_T("open content cert file failed!"));
            return 1;
        }

        fseek(pFileContent,0,SEEK_END);
        dwSizeContent = ftell(pFileContent);
        fseek(pFileContent,0,SEEK_SET);
        pFileimg = _tfopen(strImgFile.c_str(),_T("rb"));
        if(pFileimg == NULL)
        {
            _tprintf(_T("open image file failed!"));
            return 1;
        }

        fseek(pFileimg,0,SEEK_END);
        dwSizeimg = ftell(pFileimg);
        fseek(pFileimg,0,SEEK_SET);

        pFileimg_enc = _tfopen(strImgFile_enc.c_str(),_T("rb"));
        if(pFileimg_enc == NULL)
        {
            _tprintf(_T("NO ENC feature"));
        } else {
           enc_enable = 1;
        }

        pFiledebugCert = _tfopen(strdebugCertFile.c_str(),_T("rb"));
        if(pFiledebugCert == NULL)
        {
            _tprintf(_T("open debug cert file failed!"));
            return 1;
        }

        if (deCert == 1) {
            fseek(pFiledebugCert,0,SEEK_END);
            dwSizedebugCert=ftell(pFiledebugCert);
            fseek(pFiledebugCert,0,SEEK_SET);
        }

        fmheader.certa_size=dwSizekeyA;
        fmheader.certa_offset=dwSizeimg+sizeof(fmpreambleheader);
        fmheader.certb_size=dwSizekeyB;
        fmheader.certb_offset=fmheader.certa_offset+fmheader.certa_size;
        fmheader.certcnt_size=dwSizeContent;
        fmheader.certcnt_offset=fmheader.certb_offset+fmheader.certb_size;
        fmheader.cert_dbg_second_size=dwSizedebugCert;
        fmheader.cert_dbg_second_offset=fmheader.certcnt_offset+fmheader.certcnt_size;

        dwTotalSize = dwSizeimg+sizeof(fmpreambleheader)+dwSizekeyA+dwSizekeyB+dwSizeContent+dwSizedebugCert;
        dwPaddingLen=dwTotalSize;

        pBuf = new BYTE[dwPaddingLen];
        memset(pBuf, 0xFF,dwPaddingLen);

        if(!enc_enable)
           len = fread(pBuf,1,dwSizeimg,pFileimg);
        else
        {
            /*read from 0 to sizeof(sys_img_header) read _enc file if enable the CE.*/
           len = fread(pBuf,1,INSERTHEADER,pFileimg);
           len = fread(pBuf+INSERTHEADER,1,dwSizeimg-INSERTHEADER,pFileimg_enc);
        }

        memcpy(pBuf+dwSizeimg,&fmheader,sizeof(fmpreambleheader));

        len = fread(pBuf+fmheader.certa_offset,1,dwSizekeyA,pFilekeyA);
        len = fread(pBuf+fmheader.certb_offset,1,dwSizekeyB,pFilekeyB);
        len = fread(pBuf+fmheader.certcnt_offset,1,dwSizeContent,pFileContent);

        if (deCert == 1) {
            len = fread(pBuf+fmheader.cert_dbg_second_offset,1,dwSizedebugCert,pFiledebugCert);
        }

        fclose(pFilekeyA);
        pFilekeyA = NULL;

        fclose(pFilekeyB);
        pFilekeyB = NULL;

        fclose(pFileContent);
        pFileContent = NULL;

        fclose(pFileimg);
        pFileimg = NULL;

        fclose(pFiledebugCert);
        pFiledebugCert = NULL;

    } else if (level == 2) {//Two-Level SB Certficate Scheme
        strkeyCertCFile = certc;
        strContentFile = certcnt;
        strOutFile = imagename;
        strImgFile = imagename;

        pFilekeyC = _tfopen(strkeyCertCFile.c_str(),_T("rb"));
        if(pFilekeyC == NULL)
        {
            _tprintf(_T("open keycertC file failed!"));
            return 1;
        }

        fseek(pFilekeyC,0,SEEK_END);
        dwSizekeyC = ftell(pFilekeyC);
        fseek(pFilekeyC,0,SEEK_SET);

        pFileContent = _tfopen(strContentFile.c_str(),_T("rb"));
        if(pFileContent == NULL)
        {
            _tprintf(_T("open keycertBfile failed!"));
            return 1;
        }

        fseek(pFileContent,0,SEEK_END);
        dwSizeContent = ftell(pFileContent);
        fseek(pFileContent,0,SEEK_SET);
        pFileimg = _tfopen(strImgFile.c_str(),_T("rb"));
        if(pFileimg == NULL)
        {
            _tprintf(_T("open keycertBfile failed!"));
            return 1;
        }

        fseek(pFileimg,0,SEEK_END);
        dwSizeimg = ftell(pFileimg);
        fseek(pFileimg,0,SEEK_SET);

        fmheader.certa_size=dwSizekeyC;
        fmheader.certa_offset=dwSizeimg+sizeof(fmpreambleheader);
        fmheader.certb_size=dwSizekeyB;
        fmheader.certb_offset=fmheader.certa_offset+fmheader.certa_size;
        fmheader.certcnt_size=dwSizeContent;
        fmheader.certcnt_offset=fmheader.certb_offset+fmheader.certb_size;


        dwTotalSize = dwSizeimg+sizeof(fmpreambleheader)+dwSizekeyC+dwSizeContent;
        dwPaddingLen=dwTotalSize;

        pBuf = new BYTE[dwPaddingLen];
        memset(pBuf, 0xFF,dwPaddingLen);

        len = fread(pBuf,1,dwSizeimg,pFileimg);
        memcpy(pBuf+dwSizeimg,&fmheader,sizeof(fmpreambleheader));

        len = fread(pBuf+fmheader.certa_offset,1,dwSizekeyC,pFilekeyC);
        len = fread(pBuf+fmheader.certcnt_offset,1,dwSizeContent,pFileContent);

        fclose(pFilekeyC);
        pFilekeyC = NULL;

        fclose(pFileContent);
        pFileContent = NULL;

        fclose(pFileimg);
        pFileimg = NULL;
    } else {//Only Content Certficate
        strContentFile = certcnt;
        strOutFile = imagename;
        strImgFile = imagename;

        pFileContent = _tfopen(strContentFile.c_str(),_T("rb"));
        if(pFileContent == NULL)
        {
            _tprintf(_T("open keycertBfile failed!"));
            return 1;
        }

        fseek(pFileContent,0,SEEK_END);
        dwSizeContent = ftell(pFileContent);
        fseek(pFileContent,0,SEEK_SET);
        pFileimg = _tfopen(strImgFile.c_str(),_T("rb"));
        if(pFileimg == NULL)
        {
            _tprintf(_T("open keycertBfile failed!"));
            return 1;
        }

        fseek(pFileimg,0,SEEK_END);
        dwSizeimg = ftell(pFileimg);
        fseek(pFileimg,0,SEEK_SET);

        fmheader.certa_size=dwSizekeyA;
        fmheader.certa_offset=dwSizeimg+sizeof(fmpreambleheader);
        fmheader.certb_size=dwSizekeyB;
        fmheader.certb_offset=fmheader.certa_offset+fmheader.certa_size;
        fmheader.certcnt_size=dwSizeContent;
        fmheader.certcnt_offset=fmheader.certb_offset+fmheader.certb_size;

        dwTotalSize = sizeof(fmpreambleheader)+dwSizeimg+dwSizeContent;
        dwPaddingLen=dwTotalSize;

        pBuf = new BYTE[dwPaddingLen];
        memset(pBuf, 0xFF,dwPaddingLen);

        len = fread(pBuf,1,dwSizeimg,pFileimg);
        memcpy(pBuf+dwSizeimg,&fmheader,sizeof(fmpreambleheader));
        len = fread(pBuf+fmheader.certcnt_offset,1,dwSizeContent,pFileContent);

        fclose(pFileContent);
        pFileContent = NULL;

        fclose(pFileimg);
        pFileimg = NULL;
    }
    FILE *pOutFile = _tfopen(strOutFile.c_str(),_T("wb"));
    if(pOutFile == NULL)
    {
        SAFE_DEL_BUF(pBuf);
        _tprintf(_T("create output file failed!\n"));
        return 1;
    }
    fwrite(pBuf,1,dwPaddingLen,pOutFile);
    fclose(pOutFile);
    return 0;
}

