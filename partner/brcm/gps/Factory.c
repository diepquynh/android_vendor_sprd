///============================================================================
/// Copyright 2009 Broadcom Corporation -- http://www.broadcom.com
/// This program is the proprietary software of Broadcom Corporation and/or
/// its licensors, and may only be used, duplicated, modified or distributed
/// pursuant to the terms and conditions of a separate, written license
/// agreement executed between you and Broadcom (an "Authorized License").
/// Except as set forth in an Authorized License, Broadcom grants no license
/// (express or implied), right to use, or waiver of any kind with respect to
/// the Software, and Broadcom expressly reserves all rights in and to the
/// Software and all intellectual property rights therein. IF YOU HAVE NO
/// AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE IN ANY
/// WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE ALL USE OF
/// THE SOFTWARE.
/// ---------------------------------------------------------------------------
// Factory.c : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/*-----------------------------------------------------------------------------
Parse a line containing numbers into array. 
If the field is empty, value of array entry will be 0.
/// fmt can be one of 
%d = integer
%f = float
%lf = double

/// data is the array
/// size is the byte count of one array element
/// n is the number of elements in the array
Example: float array[16];
data <= array
size <= sizeof(array[0])
n    <= sizeof(array) / sizeof(array[0])
-----------------------------------------------------------------------------*/
static void parseNumbers(char* line, const char* fmt, void* data, int size, int n)
{
    int i;
    for ( i =0 ; i < n; ++i)
    {
        line = strchr(line, ',');
        if (NULL == line)
        {
            break;
        }
        line++;
        sscanf(line, fmt, (char*)data + i * size);
    }
}

/*-----------------------------------------------------------------------------
Parse the FTS factory test NMEA line, see GLL documentation
Return true if test pass
-----------------------------------------------------------------------------*/
static int parseFts(char* nmea)
{
    enum { 
	    SENTENCE_VERSION        = 0, 
		MESSAGE_NAME            = 1, // Text 
        PRN                     = 2, 
	    LAST_SIGNAL_STRENGTH    = 3, 
	    AVERAGED_SIGNAL_STRENGTH = 4, 
	    UNITS_DBM               = 5, 
	    LAST_CN_ANTENNA         = 6, 
	    AVERAGED_CN_ANTENNA     = 7, 
	    LAST_CN_BASEBAND        = 8, 
	    AVERAGED_CN_BASEBAND    = 9, 
	    UNITS_DBHZ              = 10, 
        REF_CLOCK_OFFSET_PPB    = 11, 
        UNCERTAINTY_PPB         = 12, 
        RTC_OFFSET_PPM          = 13, 
        CNTIN                   = 14, // Text 
        TCXO_PPB                = 15, 
        DECODED_WORD_COUNT      = 16, 
        DECODING_ATTEMPT_COUNT  = 17, 
        WORD_ERROR_RATE         = 18, 
        RF_AGC_VALUE            = 19, 
        ENERGY_DROP_OUT_COUNTER = 20,

		// Not a field name, for accounting only
	    CHECKSUM		= 21,

        FIELD_COUNT
    };

    float value[FIELD_COUNT] = {0};

    parseNumbers(nmea, "%f", value, sizeof(value[0]), FIELD_COUNT);

    int svid	= value[PRN]; 
    float snr 	= value[LAST_SIGNAL_STRENGTH];
    float cn	= value[LAST_CN_BASEBAND];

    printf("SVID = %d , SignalStrengh = %f , C/No = %f\n", svid , snr, cn);
    return 0;
}

int main(int argc, char* argv[])
{
	int pid = 0;

	printf("GPS Test Starting...\n");

	if((pid = fork()) < 0){
		perror("create sub process error!");
	}
	else if(pid == 0){
		// excutable name and .xml path should be costomized 
		if(execlp("glgps", "glgps", "-c", "/etc/gpsconfig.xml", "Factory_High_SNR", NULL) < 0)
		{
			perror("error on execlp");
		}
	}
	else{
		sleep(1);
		printf("GPS running...\n");

   		// nmea pipe path should be costomized
   		printf("Opening pipe...\n");
   		int pipefd = open("/data/gpspipe", O_RDONLY); 
		
		if(pipefd < 0)
		{
			perror("open pipe error!");
			return 1;
		}

   		enum { MAX_RETRY_INITIAL = 6 };
   		int retry = MAX_RETRY_INITIAL;
    
   		printf("Reading from socket...\n");
    	do {
        	char nmea[1024] = {0};
			int byteCount = 0;
			
			if((byteCount = read(pipefd, (unsigned char*)nmea, sizeof(nmea)) ) < 0)
			{
				perror("read pipe error\n");
			}

        	//printf("Byte count: %d %s\n", byteCount, nmea);
        		
			if (byteCount > 0)
        	{
        		enum { MAX_RETRY = 3 };
        		retry = MAX_RETRY;

        		// process only factory NMEA sentence, 
        		// here may need to note the version, to use "$PGLOR,0,FTS" or "$PGLOR,1,FTS"
        		if (NULL != strstr(nmea, "$PGLOR,1,FTS"))
        		{
           			//if each nmea need to be showed please open here
           			parseFts(nmea) ;
       			}
       		}
        	else if (--retry == 0)
        	{
				break;	
        	}
        	else
        	{
            	//printf("No data...\n");
				continue;
        	}
    	}while (1);

	}
	
	printf("Gps Test Stop.\n");
    
    return 0;
}

