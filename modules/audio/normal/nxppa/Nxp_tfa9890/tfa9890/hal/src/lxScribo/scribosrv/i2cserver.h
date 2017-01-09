/*
 * i2cserver.c
 *
 *  Created on: Apr 21, 2012
 *      Author: wim
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lxScribo.h"
#include "NXP_I2C.h"

int i2c_GetSpeed(int bus);
void i2c_SetSpeed(int bus, int bitrate);

int i2c_WriteRead(int bus, int addrWr, void* dataWr, int sizeWr, int* nWr, void* dataRd, int sizeRd, int* nRd);
_Bool i2c_Write(int bus, int addrWr, void* dataWr, int sizeWr);
_Bool i2c_Read(int bus, int addr, void* data, int size, int* nr);
_Bool gui_GetValue(menuElement_t item, int* val);
_Bool gui_SetValue(menuElement_t item, int* val);


