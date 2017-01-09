/*
 * tfaContainer.h
 *
 *  Created on: Sep 11, 2013
 *      Author: wim
 */

#ifndef TFACONTAINER_H_
#define TFACONTAINER_H_


#include "tfa98xxParameters.h"

int tfaContParseIni(char *iniFile, char *outFileName);
int tfaContCreateContainer(nxpTfaContainer_t *contIn, char *outFileName);
int tfaContainerSave(nxpTfaContainer_t *cont,char *filename);
nxpTfaProfileList_t *tfaContFindProfile(nxpTfaContainer_t *cont,const char *name);
nxpTfaProfileList_t *tfaContGet1stProfList(nxpTfaContainer_t *cont);
nxpTfaProfileList_t *tfaContGetDevProfList(nxpTfaContainer_t *cont,int devIdx,int profIdx);
nxpTfaDescPtr_t *tfaContSetOffset(nxpTfaContainer_t *cont,nxpTfaDescPtr_t *dsc,int idx);
char *tfaContGetString(nxpTfaDescPtr_t *dsc);
nxpTfaDeviceList_t *tfaContGetDevList(nxpTfaContainer_t *cont,int idx);
nxpTfaDescriptorType_t parseKeyType(char *key);
uint32_t tfaContCRC32(uint8_t *addr,uint32_t num,uint32_t crc);


#endif /* TFACONTAINER_H_ */
