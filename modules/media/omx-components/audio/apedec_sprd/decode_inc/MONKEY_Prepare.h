#ifndef APE_PREPARE_H
#define APE_PREPARE_H

#define SPECIAL_FRAME_MONO_SILENCE              1
#define SPECIAL_FRAME_LEFT_SILENCE              1
#define SPECIAL_FRAME_RIGHT_SILENCE             2
#define SPECIAL_FRAME_PSEUDO_STEREO             4

/*****************************************************************************
Manage the preparation stage of compression and decompression

Tasks:

1) convert data to 32-bit
2) convert L,R to X,Y
3) calculate the CRC
4) do simple analysis
5) check for the peak value
*****************************************************************************/

void MONKEY_Unprepare(int X, int Y, void * m_APEFileInfo, unsigned char * pOutputX, 
			unsigned char * pOutputY, unsigned int * pCRC);


#endif // #ifndef APE_PREPARE_H
