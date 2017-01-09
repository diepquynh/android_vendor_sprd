/**
  test/components/audio_effects/omxaudiomixertest.c

  This simple test application take one or more input stream/s. passes
  these streams to an audio mixer component and stores the mixed output in another
  output file.

  Copyright (C) 2008-2009 STMicroelectronics
  Copyright (C) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA
  02110-1301  USA

*/

#include "omxaudiomixertest.h"

#define BUFFER_COUNT_ACTUAL 2
#define FRAME_SIZE 1152*2*2 // 1152 samples* 2 channels * 2byte/16bits per channel

OMX_CALLBACKTYPE callbacks = { .EventHandler = audiomixerEventHandler,
                               .EmptyBufferDone = audiomixerEmptyBufferDone,
                               .FillBufferDone = audiomixerFillBufferDone,
};

static void setHeader(OMX_PTR header, OMX_U32 size) {
  OMX_VERSIONTYPE* ver = (OMX_VERSIONTYPE*)(header + sizeof(OMX_U32));
  *((OMX_U32*)header) = size;

  ver->s.nVersionMajor = VERSIONMAJOR;
  ver->s.nVersionMinor = VERSIONMINOR;
  ver->s.nRevision = VERSIONREVISION;
  ver->s.nStep = VERSIONSTEP;
}

void display_help() {
  printf("\n");
  printf("Usage: omxaudiomixertest [-o outfile] [-gi gain] -t -r 44100 -n 2 filename0 [filename1 filename2 filename3]\n");
  printf("\n");
  printf("       -o outfile: If this option is specified, the output stream is written to outfile\n");
  printf("                   otherwise redirected to std output\n");
  printf("       -gi       : Gain of stream i[0..3] data [0...100]\n");
  printf("       -r 44100  : Sample Rate [Default 44100]\n");
  printf("       -n 2      : Number of channel [Default 2]\n\n");
  printf("       -h        : Displays this help\n");
  printf("\n");
  exit(1);
}

/* Application private date: should go in the component field (segs...) */
appPrivateType* appPriv;
int fd[4];
unsigned int filesize[4];
int flagIsOutputExpected;
int flagOutputReceived;
int flagInputReceived;
int flagIsGain[4];
int flagSampleRate;
int flagChannel;
char *input_file[4], *output_file;
OMX_BOOL bEOS[4];
FILE *outfile;

OMX_BUFFERHEADERTYPE *inBuffer[8], *outBuffer[2],*inBufferSink[2];
static OMX_BOOL isPortDisabled[4];
static int iBufferDropped[4];

int main(int argc, char** argv) {

  OMX_PORT_PARAM_TYPE sParam;
  OMX_U32 data_read;
  int j;
  int i=0;
  OMX_PARAM_PORTDEFINITIONTYPE sPortDef;
  OMX_AUDIO_CONFIG_VOLUMETYPE sVolume;
  int gain[4];
  int argn_dec;
  int index_files = 0, index_gain = 0;
  OMX_U32 srate=0,nchannel=0;
  OMX_ERRORTYPE err;
  char c;

  gain[0]=gain[1]=gain[2]=gain[3]=100;
  fd[0] = fd[1] = fd[2] = fd[3] = 0;
  bEOS[0] = bEOS[1] = bEOS[2] = bEOS[3] = OMX_FALSE;
  /* Obtain file descriptor */
  if(argc < 2){
    display_help();
  } else {
    flagIsOutputExpected = 0;
    flagOutputReceived = 0;
    flagInputReceived = 0;
    flagIsGain[0] = 0;
    flagIsGain[1] = 0;
    flagIsGain[2] = 0;
    flagIsGain[3] = 0;
    flagSampleRate = 0;
    flagChannel = 0;

    argn_dec = 1;
    while (argn_dec<argc) {
      if (*(argv[argn_dec]) =='-') {
        if (flagIsOutputExpected) {
          display_help();
        }
        switch (*(argv[argn_dec]+1)) {
        case 'h':
          display_help();
          break;
        case 'o':
          flagIsOutputExpected = 1;
          break;
        case 'g':
          index_gain = atoi(argv[argn_dec]+2);
          if(index_gain > 3) {
          DEBUG(DEFAULT_MESSAGES, "-g%i is not valid\n", index_gain);
          index_gain = 0;
          }
        flagIsGain[index_gain] = 1;
        break;
        case 'r':
          flagSampleRate = 1;
          break;
        case 'n':
          flagChannel = 1;
          break;
        default:
          display_help();
        }
      } else {
        if (flagIsGain[index_gain]) {
          gain[index_gain] = (int)atoi(argv[argn_dec]);
          DEBUG(DEFAULT_MESSAGES, "gain[%d]=%d\n", index_gain, gain[index_gain]);
          flagIsGain[index_gain] = 0;
          if(gain[index_gain] > 100) {
            DEBUG(DEFAULT_MESSAGES, "Gain of stream %i should be between [0..100]\n", index_gain);
            gain[index_gain] = 100;
          }
          index_gain = 0;
        } else if (flagIsOutputExpected) {
          output_file = malloc(strlen(argv[argn_dec]) * sizeof(char) + 1);
          strcpy(output_file,argv[argn_dec]);
          flagIsOutputExpected = 0;
          flagOutputReceived = 1;
        } else if (flagSampleRate) {
          srate = (int)atoi(argv[argn_dec]);
          flagSampleRate = 0;
        } else if (flagChannel) {
          nchannel = (int)atoi(argv[argn_dec]);
          flagChannel = 0;
        } else {
            if (index_files>3) {
                DEBUG(DEB_LEV_ERR, "Too many input files. Only first four are accepted\n");
            } else {
              input_file[index_files] = malloc(strlen(argv[argn_dec]) * sizeof(char) + 1);
              strcpy(input_file[index_files],argv[argn_dec]);
              flagInputReceived = 1;
              index_files++;
            }
        }
      }
      argn_dec++;
    }
    if (!flagInputReceived) {
      display_help();
    }
    DEBUG(DEFAULT_MESSAGES, "Input files %s %s %s %s \n", input_file[0], input_file[1], input_file[2], input_file[3]);
    DEBUG(DEFAULT_MESSAGES, " to ");
    if (flagOutputReceived) {
      DEBUG(DEFAULT_MESSAGES, " %s\n", output_file);
    } else {
      DEBUG(DEFAULT_MESSAGES, " Audio Sink\n");
    }
  }

  if(input_file[0]== NULL)  {
    DEBUG(DEB_LEV_ERR, "Provide at least an input file\n");
    exit(1);
  }

  for (i = 0; i<index_files; i++) {
	  fd[i] = open(input_file[i], O_RDONLY);
	  if(fd[i] < 0){
		  DEBUG(DEB_LEV_ERR, "Error opening input file %i\n", i);
		  exit(1);
	  }
  }

  if (flagOutputReceived) {
    outfile = fopen(output_file,"wb");
    if(outfile == NULL) {
      DEBUG(DEB_LEV_ERR, "Error at opening the output file");
      exit(1);
    }
  }


  for (i = 0; i<index_files; i++) {
	  filesize[i] = getFileSize(fd[i]);
  }

  /* Initialize application private data */
  appPriv = malloc(sizeof(appPrivateType));
  pthread_cond_init(&appPriv->condition, NULL);
  pthread_mutex_init(&appPriv->mutex, NULL);
  appPriv->eventSem = malloc(sizeof(tsem_t));
  tsem_init(appPriv->eventSem, 0);
  appPriv->eofSem = malloc(sizeof(tsem_t));
  tsem_init(appPriv->eofSem, 0);
  iBufferDropped[0] = 0;
  iBufferDropped[1] = 0;
  iBufferDropped[2] = 0;
  iBufferDropped[3] = 0;

  err = OMX_Init();
  if(err != OMX_ErrorNone) {
    DEBUG(DEB_LEV_ERR, "OMX_Init() failed\n");
    exit(1);
  }
  /** Ask the core for a handle to the audio mixer component */
  err = OMX_GetHandle(&appPriv->handle, "OMX.st.audio.mixer", NULL , &callbacks);
  if(err != OMX_ErrorNone) {
    DEBUG(DEB_LEV_ERR, "Audio Mixer OMX_GetHandle failed\n");
    exit(1);
  }

  /*Max 4 input stream*/
  for(j=0;j<4;j++) {
    isPortDisabled[j] = OMX_FALSE;
    if((gain[j] >= 0) && (gain[j] <100)) {
      sVolume.nPortIndex = j;
      err = OMX_GetConfig(appPriv->handle, OMX_IndexConfigAudioVolume, &sVolume);
      if(err!=OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR,"Error %08x In OMX_GetConfig %i \n",err, j);
      }
      sVolume.sVolume.nValue = gain[j];
      DEBUG(DEFAULT_MESSAGES, "Setting Gain[%i] %d \n",(int)j, gain[j]);
      err = OMX_SetConfig(appPriv->handle, OMX_IndexConfigAudioVolume, &sVolume);
      if(err!=OMX_ErrorNone) {
        DEBUG(DEB_LEV_ERR,"Error %08x In OMX_SetConfig %i \n",err, j);
      }
    }
  }

  /** Get the number of ports */
  setHeader(&sParam, sizeof(OMX_PORT_PARAM_TYPE));
  err = OMX_GetParameter(appPriv->handle, OMX_IndexParamAudioInit, &sParam);
  if(err != OMX_ErrorNone){
    DEBUG(DEB_LEV_ERR, "Error in getting OMX_PORT_PARAM_TYPE parameter\n");
    exit(1);
  }
  DEBUG(DEFAULT_MESSAGES, "Audio Mixer has %d ports\n",(int)sParam.nPorts);

// disable unused ports
  for (j = index_files; j<4; j++) {
	  isPortDisabled[j] = OMX_TRUE;
	  err = OMX_SendCommand(appPriv->handle, OMX_CommandPortDisable, j, NULL);
	  tsem_down(appPriv->eventSem);
	  DEBUG(DEFAULT_MESSAGES, "Port %i disabled\n", j);
  }
  for (j = 0; j < index_files; j++) {
	  setHeader(&sPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	  sPortDef.nPortIndex = j;
	  err = OMX_GetParameter(appPriv->handle, OMX_IndexParamPortDefinition, &sPortDef);

	  sPortDef.nBufferCountActual = 2;
	  err = OMX_SetParameter(appPriv->handle, OMX_IndexParamPortDefinition, &sPortDef);
	  if(err != OMX_ErrorNone){
		  DEBUG(DEB_LEV_ERR, "Error in getting OMX_PORT_PARAM_TYPE parameter\n");
		  exit(1);
	  }
  }

  err = OMX_SendCommand(appPriv->handle, OMX_CommandStateSet, OMX_StateIdle, NULL);

  for (j=0; j<8; j++) {
	  inBuffer[j] = 0;
  }
  outBuffer[0] = outBuffer[1] = NULL;


  for(j=0; j<index_files; j++) {
    err = OMX_AllocateBuffer(appPriv->handle, &inBuffer[j*2], j, NULL, BUFFER_IN_SIZE);
    if (err != OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "Error on AllocateBuffer %i %p on port %i\n", j*2, inBuffer[j*2], j);
      exit(1);
    }
    err = OMX_AllocateBuffer(appPriv->handle, &inBuffer[j * 2 + 1], j, NULL, BUFFER_IN_SIZE);
    if (err != OMX_ErrorNone) {
      DEBUG(DEB_LEV_ERR, "Error on AllocateBuffer %i %p on port %i\n", j*2+1, inBuffer[j*2+1], j);
      exit(1);
    }
  }

   err = OMX_AllocateBuffer(appPriv->handle, &outBuffer[0], 4, NULL, BUFFER_IN_SIZE);
   if (err != OMX_ErrorNone) {
     DEBUG(DEB_LEV_ERR, "Error on AllocateBuffer 0 %p on port 4\n", outBuffer[0]);
     exit(1);
   }
   err = OMX_AllocateBuffer(appPriv->handle, &outBuffer[1], 4, NULL, BUFFER_IN_SIZE);
   if (err != OMX_ErrorNone) {
     DEBUG(DEB_LEV_ERR, "Error on AllocateBuffer 1 %p on port 4\n", outBuffer[1]);
     exit(1);
   }

  tsem_down(appPriv->eventSem);

  err = OMX_SendCommand(appPriv->handle, OMX_CommandStateSet, OMX_StateExecuting, NULL);

  /* Wait for commands to complete */
  tsem_down(appPriv->eventSem);

  for (i = 0; i<index_files; i++) {
	    data_read = read(fd[i], inBuffer[i*2]->pBuffer, FRAME_SIZE);
	    inBuffer[i*2]->nFilledLen = data_read;
	    filesize[i] -= data_read;
	    data_read = read(fd[i], inBuffer[i*2+1]->pBuffer, FRAME_SIZE);
	    inBuffer[i*2+1]->nFilledLen = data_read;
	    filesize[i] -= data_read;
  }


  for (i = 0; i<index_files; i++) {
	  err = OMX_EmptyThisBuffer(appPriv->handle, inBuffer[i*2]);
	  err = OMX_EmptyThisBuffer(appPriv->handle, inBuffer[i*2+1]);
  }

  /** Schedule a couple of buffers to be filled on the output port
    * The callback itself will re-schedule them.
    */
  err = OMX_FillThisBuffer(appPriv->handle, outBuffer[0]);
  err = OMX_FillThisBuffer(appPriv->handle, outBuffer[1]);

  /*Port Disable option available in case of direct play out only*/
  if(!flagOutputReceived) {
	  DEBUG(DEFAULT_MESSAGES, "\nIf you want to disabled port enter port number[0..3]: else Enter 'q' \n\n");
	  while(!bEOS[0] || !bEOS[1] || !bEOS[2] || !bEOS[3]) {
		  DEBUG(DEFAULT_MESSAGES, "Port status 0=%i, 1=%i, 2=%i, 3=%i\n",isPortDisabled[0], isPortDisabled[1], isPortDisabled[2], isPortDisabled[3]);
		  DEBUG(DEFAULT_MESSAGES, "Port play   0=%i, 1=%i, 2=%i, 3=%i\n",bEOS[0], bEOS[1], bEOS[2], bEOS[3]);
		  DEBUG(DEFAULT_MESSAGES, "Entry : ");
		  c = getchar();
		  if(c=='\n') {
			  continue;
		  } else if(c == 'q') {
			  DEBUG(DEFAULT_MESSAGES,"No port to disable\n");
			  break;
		  } else {
			  i= (int)atoi(&c);
			  if(i>=0 && i<4) {
				  DEBUG(DEFAULT_MESSAGES,"Disabling/Enabling Port %i\n", i);
				  if (isPortDisabled[i] == OMX_TRUE) {
				        err = OMX_SendCommand(appPriv->handle, OMX_CommandPortEnable, i, NULL);
				        err = OMX_AllocateBuffer(appPriv->handle, &inBuffer[i*2], i, NULL, BUFFER_IN_SIZE);
				        err = OMX_AllocateBuffer(appPriv->handle, &inBuffer[i*2+1], i, NULL, BUFFER_IN_SIZE);
				        tsem_down(appPriv->eventSem);
				        isPortDisabled[i] = OMX_FALSE;
				        data_read = read(fd[i], inBuffer[i*2]->pBuffer, FRAME_SIZE);
				        inBuffer[i*2]->nFilledLen = data_read;
				        data_read = read(fd[i], inBuffer[i*2+1]->pBuffer, FRAME_SIZE);
				        inBuffer[i*2+1]->nFilledLen = data_read;
				        //Sending Empty buffer
				        err = OMX_EmptyThisBuffer(appPriv->handle, inBuffer[i*2]);
				        err = OMX_EmptyThisBuffer(appPriv->handle, inBuffer[i*2+1]);
				  } else {
					  isPortDisabled[i] = OMX_TRUE;
					  err = OMX_SendCommand(appPriv->handle, OMX_CommandPortDisable, i, NULL);
					  while(iBufferDropped[i]!=2) {
						  usleep(10000);
					  }
					  for(j=0;j<BUFFER_COUNT_ACTUAL;j++) {
						  err = OMX_FreeBuffer(appPriv->handle, i, inBuffer[j+i]);
					  }
					  tsem_down(appPriv->eventSem);
					  iBufferDropped[i] = 0;
				  }
			  } else {
				  DEBUG(DEFAULT_MESSAGES,"Either Port %i is already disabled or not valid\n",i);
			  }
		  }
	  }
  }

  DEBUG(DEFAULT_MESSAGES, "Waiting for EOS\n");
  if(isPortDisabled[0] == OMX_FALSE) {
    tsem_down(appPriv->eofSem);
    DEBUG(DEFAULT_MESSAGES, "Received EOS 1\n");
  }
  if(isPortDisabled[1] == OMX_FALSE) {
    tsem_down(appPriv->eofSem);
    DEBUG(DEFAULT_MESSAGES, "Received EOS 2\n");
  }
  if(isPortDisabled[2] == OMX_FALSE) {
    tsem_down(appPriv->eofSem);
    DEBUG(DEFAULT_MESSAGES, "Received EOS 3\n");
  }
  if(isPortDisabled[3] == OMX_FALSE) {
    tsem_down(appPriv->eofSem);
    DEBUG(DEFAULT_MESSAGES, "Received EOS 4\n");
  }

  err = OMX_SendCommand(appPriv->handle, OMX_CommandStateSet, OMX_StateIdle, NULL);
  /* Wait for commands to complete */
  tsem_down(appPriv->eventSem);

  err = OMX_SendCommand(appPriv->handle, OMX_CommandStateSet, OMX_StateLoaded, NULL);
  for(j=0; j<index_files; j++) {
    if(isPortDisabled[j] == OMX_FALSE) {
      err = OMX_FreeBuffer(appPriv->handle, j, inBuffer[j*2]);
      err = OMX_FreeBuffer(appPriv->handle, j, inBuffer[j*2+1]);
    }
  }

   for(j=0;j<BUFFER_COUNT_ACTUAL;j++) {
     err = OMX_FreeBuffer(appPriv->handle, 4, outBuffer[j]);
   }

  /* Wait for commands to complete */
  tsem_down(appPriv->eventSem);

  OMX_FreeHandle(appPriv->handle);

  free(appPriv->eventSem);
  free(appPriv);

  if (flagOutputReceived) {
    if(fclose(outfile) != 0) {
      DEBUG(DEB_LEV_ERR,"Error in closing output file\n");
      exit(1);
    }
    free(output_file);
  }
  for (i = 0; i<index_files; i++) {
	  close(fd[i]);
	  free(input_file[i]);
  }

  return 0;
}

/* Callbacks implementation */
OMX_ERRORTYPE audiomixerEventHandler(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_EVENTTYPE eEvent,
  OMX_U32 Data1,
  OMX_U32 Data2,
  OMX_PTR pEventData) {

  DEBUG(DEB_LEV_SIMPLE_SEQ, "Hi there, I am in the %s callback\n", __func__);
  if(eEvent == OMX_EventCmdComplete) {
    if (Data1 == OMX_CommandStateSet) {
      DEBUG(DEB_LEV_SIMPLE_SEQ, "Volume Component State changed in ");
      switch ((int)Data2) {
      case OMX_StateInvalid:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateInvalid\n");
        break;
      case OMX_StateLoaded:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateLoaded\n");
        break;
      case OMX_StateIdle:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateIdle\n");
        break;
      case OMX_StateExecuting:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateExecuting\n");
        break;
      case OMX_StatePause:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StatePause\n");
        break;
      case OMX_StateWaitForResources:
        DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_StateWaitForResources\n");
        break;
      }
      tsem_up(appPriv->eventSem);
    } else  if (Data1 == OMX_CommandPortEnable){
      tsem_up(appPriv->eventSem);
    } else if (Data1 == OMX_CommandPortDisable){
      tsem_up(appPriv->eventSem);
    }
  } else if(eEvent == OMX_EventBufferFlag) {
      DEBUG(DEB_LEV_SIMPLE_SEQ, "OMX_EventBufferFlag\n");
    if((int)Data2 == OMX_BUFFERFLAG_EOS) {
      tsem_up(appPriv->eofSem);
    }
  } else {
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Param1 is %i\n", (int)Data1);
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Param2 is %i\n", (int)Data2);
  }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE audiomixerEmptyBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

  OMX_ERRORTYPE err;
  int data_read;


  DEBUG(DEB_LEV_FULL_SEQ, "Hi there, I am in the %s callback from the port %i\n", __func__, (int)pBuffer->nInputPortIndex);

  if(isPortDisabled[pBuffer->nInputPortIndex] == OMX_FALSE) {
    data_read = read(fd[pBuffer->nInputPortIndex], pBuffer->pBuffer, FRAME_SIZE);
    pBuffer->nFilledLen = data_read;
    pBuffer->nOffset = 0;
    filesize[pBuffer->nInputPortIndex] -= data_read;
    DEBUG(DEB_LEV_SIMPLE_SEQ, "Sending from file %i data read=%d\n", (int)pBuffer->nInputPortIndex, data_read);
    if (data_read <= 0) {
      DEBUG(DEB_LEV_SIMPLE_SEQ, "In the %s no more input data available\n", __func__);
      ++iBufferDropped[pBuffer->nInputPortIndex];
      if(iBufferDropped[pBuffer->nInputPortIndex]==2) {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Dropping Empty This buffer to Audio Mixer Stream %i\n", __func__, (int)pBuffer->nInputPortIndex);
        return OMX_ErrorNone;
      } else if(iBufferDropped[pBuffer->nInputPortIndex]>2) {
        DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Dropping Empty This buffer to Audio Mixer Stream %i\n", __func__, (int)pBuffer->nInputPortIndex);
        return OMX_ErrorNone;
      }
      pBuffer->nFilledLen=0;
      pBuffer->nFlags = OMX_BUFFERFLAG_EOS;
      bEOS[pBuffer->nInputPortIndex]=OMX_TRUE;
      DEBUG(DEB_LEV_SIMPLE_SEQ, "In %s Sending EOS for Stream %i\n", __func__, (int)pBuffer->nInputPortIndex);
      err = OMX_EmptyThisBuffer(hComponent, pBuffer);
      return OMX_ErrorNone;
    }
  } else {
    ++iBufferDropped[pBuffer->nInputPortIndex];
    return OMX_ErrorNone;
  }
  if(!bEOS[pBuffer->nInputPortIndex]) {
    DEBUG(DEB_LEV_FULL_SEQ, "Empty buffer %p\n", pBuffer);
    err = OMX_EmptyThisBuffer(hComponent, pBuffer);
  }else {
    DEBUG(DEB_LEV_FULL_SEQ, "In %s Dropping Empty This buffer to Audio Mixer\n", __func__);
  }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE audiomixerFillBufferDone(
  OMX_HANDLETYPE hComponent,
  OMX_PTR pAppData,
  OMX_BUFFERHEADERTYPE* pBuffer) {

  OMX_ERRORTYPE err;
  int i;

  DEBUG(DEB_LEV_FULL_SEQ, "Hi there, I am in the %s callback. Got buflen %i for buffer at 0x%p\n",
                          __func__, (int)pBuffer->nFilledLen, pBuffer);

  /* Output data to standard output */
  if(pBuffer != NULL) {
    if (pBuffer->nFilledLen == 0) {
      DEBUG(DEB_LEV_ERR, "Ouch! In %s: no data in the output buffer!\n", __func__);
      return OMX_ErrorNone;
    }
    if (flagOutputReceived) {
      if(pBuffer->nFilledLen > 0) {
        fwrite(pBuffer->pBuffer, 1, pBuffer->nFilledLen, outfile);
      }
    } else {
      for(i=0;i<pBuffer->nFilledLen;i++) {
        putchar(*(char*)(pBuffer->pBuffer + i));
      }
    }
    pBuffer->nFilledLen = 0;
    /* Reschedule the fill buffer request */
    if(!bEOS[0] || !bEOS[1] || !bEOS[2] || !bEOS[3]) {
       err = OMX_FillThisBuffer(hComponent, pBuffer);
    } else {
       DEBUG(DEB_LEV_FULL_SEQ, "In %s Dropping Fill This buffer to Audio Mixer\n", __func__);
    }
  } else {
    DEBUG(DEB_LEV_ERR, "Ouch! In %s: had NULL buffer to output...\n", __func__);
  }
  return OMX_ErrorNone;
}

/** Gets the file descriptor's size
  * @return the size of the file. If size cannot be computed
  * (i.e. stdin, zero is returned)
  */
static int getFileSize(int fd) {

  struct stat input_file_stat;
  int err;

  /* Obtain input file length */
  err = fstat(fd, &input_file_stat);
  if(err){
    DEBUG(DEB_LEV_ERR, "fstat failed");
    exit(-1);
  }
  return input_file_stat.st_size;
}
