
#define LOG_TAG "audio_mux_pcm"

#include <stdlib.h>
#include <sys/select.h>
#include <fcntl.h>
#include <tinyalsa/asoundlib.h>
#include <cutils/log.h>
#include <pthread.h>
#include  "audio_mux_pcm.h"



#define MUX_PCM_IN  0x1
#define MUX_PCM_OUT 0x2


#define SAUDIO_CMD_NONE          0x00000000
#define SAUDIO_CMD_OPEN           0x00000001
#define SAUDIO_CMD_CLOSE         0x00000002
#define SAUDIO_CMD_START         0x00000004
#define SAUDIO_CMD_STOP             0x00000008
#define SAUDIO_CMD_PREPARE  0x00000010
#define SAUDIO_CMD_TRIGGER              0x00000020
#define SAUDIO_CMD_RECEIVE         0x00000040

#define SAUDIO_CMD_OPEN_RET            0x00010000
#define SAUDIO_CMD_CLOSE_RET         0x00020000
#define SAUDIO_CMD_START_RET          0x00040000
#define SAUDIO_CMD_STOP_RET             0x00080000
#define SAUDIO_CMD_PREPARE_RET  0x00100000
#define SAUDIO_CMD_TRIGGER_RET  0x00200000
#define SAUDIO_CMD_RECEIVE_RET         0x00400000
#define AUDIO_PLAYBACK  0
#define AUDIO_CAPTURE     1

#define PCM_ERROR_MAX 128
#define AUDIO_PLAYBACK_BUFFER (1024*3)



#ifdef VB_CONTROL_PARAMETER_V2
#define AUDIO_MUX_CTRL_FILE     "/dev/spimux27"
#define AUDIO_MUX_PLAYBACK_FILE "/dev/spimux28"
#define AUDIO_MUX_CAPTURE_FILE "/dev/spimux29"
#else
#define AUDIO_MUX_CTRL_FILE     "/dev/ts0710mux24"
#define AUDIO_MUX_PLAYBACK_FILE "/dev/ts0710mux25"
#define AUDIO_MUX_CAPTURE_FILE "/dev/ts0710mux26"
#endif
#define  MAX_SND_CARD_FILE_NAME    30

struct mux_snd_card
{
	int ctl_fd;
	unsigned int state;
	pthread_mutex_t lock;
	const char *ctl_file;
	const char *playback_file;
	const char *capture_file;
	int card_id;
};

struct pcm {
    int fd;
    unsigned int flags;
    int running:1;
    int underruns;
    unsigned int buffer_size;
    unsigned int boundary;
    char error[PCM_ERROR_MAX];
    struct pcm_config config;
    struct snd_pcm_mmap_status *mmap_status;
    struct snd_pcm_mmap_control *mmap_control;
    struct snd_pcm_sync_ptr *sync_ptr;
    void *mmap_buffer;
    unsigned int noirq_frames_per_msec;
    int wait_for_avail_min;
};




struct mux_pcm
{
    struct pcm dummy_pcm;
    pthread_mutex_t lock;
    int  mux_fd;
    int stream_type;
    int state;
    int bytes_written;
    struct  mux_snd_card * card;
};


struct cmd_common {
	unsigned int command;
	unsigned int sub_cmd;
	unsigned int reserved1;
	unsigned int reserved2;
};

struct cmd_prepare {
	struct cmd_common common;
	unsigned int rate;	/* rate in Hz */
	unsigned char channels;	/* channels */
	unsigned char format;
	unsigned char reserved1;
	unsigned char reserved2;
	unsigned int period;	/* period size */
	unsigned int periods;	/* periods */
};

struct cmd_open {
	struct cmd_common common;
	uint32_t stream_type;
};

static  struct mux_snd_card  g_snd_card[SND_CARD_MUX_MAX] ={
{
.card_id = SND_CARD_VOICE_TG,
.ctl_file = "/dev/spimux24""",
.playback_file = "/dev/spimux25",
.capture_file = "/dev/spimux26",
},
{
.card_id = SND_CARD_VOIP_TG,
.ctl_file = "/dev/spimux27""",
.playback_file = "/dev/spimux28",
.capture_file = "/dev/spimux29",
},


};

static int audio_ctrl_fd=0;
static pthread_mutex_t  audio_mux_ctrl_mutex = PTHREAD_MUTEX_INITIALIZER;

static int  audio_mux_card_lock()
{
     pthread_mutex_lock(&audio_mux_ctrl_mutex);
     return 0;
}

static int audio_mux_card_unlock()
{
     pthread_mutex_unlock(&audio_mux_ctrl_mutex);
     return 0;
}

static int  audio_mux_ctrl_lock(pthread_mutex_t *lock)
{
     pthread_mutex_lock(lock);
     return 0;
}

static int audio_mux_ctrl_unlock(pthread_mutex_t *lock)
{
     pthread_mutex_unlock(lock);
     return 0;
}

 int  mux_read( int fd,void *buffer, unsigned int bytes,  int timeout_s )
{
     char *read_buffer = buffer;
	int result = 0;
    fd_set fds_read;
    struct timeval timeout = {3,0};
    int maxfd = 0;

    unsigned int  bytes_read = 0;
	unsigned int  bytes_to_read = bytes;
    ALOGD(": mux_read, fd %d,bytes %d,timeout %d",fd,bytes,timeout_s);

    if(fd <0){
        return 0;
    }
    maxfd = fd  + 1;
    timeout.tv_sec = timeout_s;
    timeout.tv_usec =0;

    while(bytes){
        FD_ZERO(&fds_read);
        FD_SET(fd  , &fds_read);
        result = select(maxfd,&fds_read,NULL,NULL,&timeout);
	//result = select(maxfd,&fds_read,NULL,NULL,NULL);
        if(result < 0) {
            ALOGE(" :saudio_wait_common_cmd :select error %d",errno);
            break;
        }
        else if(!result) {
            ALOGE(" :saudio_wait_common_cmd select timeout");
            return -1;
        }
        if(FD_ISSET(fd  ,&fds_read) <= 0) {
            ALOGE(" :saudio_wait_common_cmd select ok but no fd is set");
            continue;
        }
        bytes_read = read(fd , (void*)read_buffer , bytes);
        if(bytes_read > 0) {
		read_buffer +=  bytes_read;
        	bytes -= bytes_read;
        }
         else if((!((errno == EAGAIN) || (errno == EINTR))) || (0 == bytes_read)) {
                ALOGE("pipe read error %d,bytes read is %d",errno,bytes_to_read - bytes);
                return -1;
            } else {
		ALOGW("pipe_read_warning: %d,ret is %d",errno,bytes_read);
	    }

    }

	return bytes_to_read - bytes;

}




static void mux_pipe_clear(int fd)
{
	ALOGW("mux_pipe_clear in!!!");
	uint8_t  tmp_buffer[1024] = {0};
	int ret = 0;
	do{
		ALOGW("mux_pipe_clear in!");
		ret = mux_read(  fd,tmp_buffer, 1024, 0);
		ALOGW("mux_pipe_clear loop.");
	}while(!ret );
}


 int32_t saudio_wait_common_cmd( struct mux_pcm *pcm, uint32_t cmd, uint32_t subcmd)
{
	int32_t result = 0;
	struct cmd_common cmd_common_buffer={0};
    struct cmd_common *common = &cmd_common_buffer;

    int bytes = 0;
    int bytes_read = 0;
   // ALOGE(": function is saudio_wait_common_cmd in");

    bytes = sizeof(struct cmd_common);


	bytes_read = mux_read(pcm->mux_fd,common,bytes,5);
	if(bytes_read != bytes) {
		ALOGE("common->command  read error\n");
		return -1;
	}
    ALOGE("common->command is %x ,sub cmd %x,\n", common->command, common->sub_cmd);
    if (subcmd) {
        if ((common->command == cmd) && (common->sub_cmd == subcmd)) {
            result = 0;
        } else {
            result = -1;
        }
	} else {
		if (common->command == cmd) {
			result = common->reserved1;
		} else {
			result = -1;
		}
	}
muxerror:

	ALOGE(": function is saudio_wait_common_cmdout,result is %d",result);
	return result;
}





 int32_t saudio_send_common_cmd(struct mux_pcm * pcm,uint32_t cmd, uint32_t subcmd)
{
	int32_t result = -1;
	struct cmd_common common={0};
	struct cmd_common cmd_common_buffer={0};
	struct cmd_common *common_ret = &cmd_common_buffer;
	uint32_t cmd_ret=cmd<<16;

	ALOGE(":  saudio_send_common_cmd cmd %x, subcmd %x\n  E",cmd, subcmd);
	
	common.command=cmd;
	common.sub_cmd=subcmd;
	if(pcm->card->ctl_fd <0){
		return -1;
	}
	audio_mux_ctrl_lock(&pcm->card->lock);
	//ALOGE(":  saudio_send_common_cmd  Wirte In");
	result=write(pcm->card->ctl_fd,&common,sizeof(struct cmd_common));
	if(result <0)
	{
		audio_mux_ctrl_unlock(&pcm->card->lock);
		ALOGE(":  saudio_send_common_cmd  Wirte error");
		return result;
	}
	//ALOGE(":  saudio_send_common_cmd  Wirte out");


	result=mux_read(pcm->card->ctl_fd,common_ret,sizeof(struct cmd_common),5);
	if(result != sizeof(struct cmd_common)) {
		ALOGE(":  mux_read  error %d ",result);
		return -1;
	}

	//ALOGE(":common->command is %x ,sub cmd %x,\n", common_ret->command, common_ret->sub_cmd);

	if (common_ret->command == cmd_ret)
	{
		result = 0;
	} else
	{
		ALOGE(":  saudio_send_common_cmd  read command error %x, should be %x", common_ret->command,cmd_ret);
		result = -1;
	}

	audio_mux_ctrl_unlock(&pcm->card->lock);
	ALOGE(":  saudio_send_common_cmd  X result %d",result);
	return result;
}



static struct   mux_snd_card *   mux_snd_card_get(unsigned int card_id,unsigned int flags)
{
	int i= 0;
	struct mux_snd_card * card = NULL;
	flags = (flags&PCM_IN)?MUX_PCM_IN:MUX_PCM_OUT;
	audio_mux_card_lock();
	for(i=0; i< SND_CARD_MUX_MAX;i++) {
		if(g_snd_card[i].card_id == card_id) {
			card = &g_snd_card[i];
		}
	}
	if(!card) {
		return NULL;
	}
	if(card->ctl_fd <=  0){
		card->ctl_fd = open(card->ctl_file, O_RDWR);
		if(card->ctl_fd <= 0 ){
			ALOGE(": mux_snd_card_get ctrl open failed,%s %d %d %s",card->ctl_file,card->ctl_fd ,errno,strerror(errno));
			audio_mux_card_unlock();
			goto error;
		}
		pthread_mutex_init(&card->lock, NULL);
	}

	if(card->state & flags) {
		ALOGE(": mux_snd_card_get ctrl open failed state error,%s %d ",AUDIO_MUX_CTRL_FILE,card->state );
		audio_mux_card_unlock();
		goto error;
	}
	else
	{
		card->state  |=  flags;
	}
	audio_mux_card_unlock();

	return  card;
error:
	ALOGE("mux_snd_card_get: get card error");
	return NULL;

}


static  int   mux_snd_card_release(struct mux_snd_card * card, unsigned int flags)
{
	flags = (flags&PCM_IN)?MUX_PCM_IN:MUX_PCM_OUT;

	if(!card) {
		return -1;
	}
	audio_mux_card_lock();
	card->state  &=  (~flags);
	audio_mux_card_unlock();

	return  0;

}


struct pcm * mux_pcm_open(unsigned int card, unsigned int device,
                     unsigned int flags, struct pcm_config *config)
{
    struct mux_pcm  *pcm=NULL;
    int sub_cmd=0;
    int ret=0;
    ALOGE(": function is mux_pcm_open in flags is %x",flags);
    pcm = calloc(1, sizeof(struct mux_pcm));
    if (!pcm)
        return NULL;
    memset(pcm, 0, sizeof(struct mux_pcm));

    pcm->card = mux_snd_card_get(card,flags);
    if(!pcm->card) {
	goto error;
    }
    sub_cmd = (flags&PCM_IN)?AUDIO_CAPTURE:AUDIO_PLAYBACK;
    pcm->stream_type = sub_cmd;
    ALOGE(":pcm->stream_type  is %d",pcm->stream_type );
    if(pcm->stream_type == AUDIO_PLAYBACK){
        pcm->mux_fd=open(pcm->card->playback_file, O_RDWR);
        ALOGE(": mux_pcm_open playback pipel open ,%s %d %d %s",pcm->card->playback_file,pcm->mux_fd,errno,strerror(errno));
    }
     else{
        pcm->mux_fd=open(pcm->card->capture_file, O_RDWR);
        ALOGE(": mux_pcm_open capture data pipe open ,%s %d %d %s",pcm->card->capture_file,pcm->mux_fd,errno,strerror(errno));
   }

     if(pcm->mux_fd<= 0){
       goto error;
    }
    else {
	mux_pipe_clear(pcm->mux_fd);
    }

    ret =saudio_send_common_cmd(pcm,SAUDIO_CMD_OPEN,sub_cmd);
    if(ret){
        goto error;
    }

    pcm->dummy_pcm.fd=pcm->mux_fd;
    pcm->dummy_pcm.config=*config;
    pcm->dummy_pcm.flags=flags;

     pthread_mutex_init(&pcm->lock, NULL);
    memcpy(pcm->dummy_pcm.error,"unknow error",sizeof("unknow error"));

    ALOGE(": function is mux_pcm_open out ret %d",ret);
    return &(pcm->dummy_pcm);

error:
    pcm->dummy_pcm.fd=-1;
    return pcm;

}

static  int mux_write_wait_response(struct mux_pcm *pcm, unsigned int  bytes_to_wait, unsigned int *bytes_received)
{
	int ret = 0;
	unsigned int  bytes_total = bytes_to_wait;
	ALOGE("peter:mux_write_wait_response  start bytes_to_wait is %d,bytes_total %d",bytes_to_wait,bytes_total);
	while(bytes_to_wait){
	     //ALOGE("peter: 	mux_write_wait_response in bytes_to_wait is %d,bytes_total %d",bytes_to_wait,bytes_total);
            ret = saudio_wait_common_cmd(pcm,SAUDIO_CMD_RECEIVE<<16 ,pcm->stream_type);
	     //ALOGE("peter: 	mux_write_wait_response out %d",ret);
	    if(ret < 0)
            {
                ALOGE(": function is mux_pcm_write retrun:%d",ret);
                break;
            }
	    if(bytes_to_wait >= ret) {
            	bytes_to_wait -= ret;
	    	//ALOGE("peter: mux_write_wait_response bytes_to_wait is %d,ret %d",bytes_to_wait,ret);
	    }
	    else {
	    	ALOGE("peter: mux_write_wait_response error ret is %d,bytes_to_wait %d",ret,bytes_to_wait);
	    	ret = -1;
	    }
        }
	
	*bytes_received = bytes_total - bytes_to_wait;
	return (ret<0 )? ret: 0;
}

int mux_pcm_write(struct pcm *pcm_in, void *data, unsigned int count)
{
    struct mux_pcm  *pcm=(struct mux_pcm  *)pcm_in;
    int ret=0;
    int bytes=0;
	int left = count;
	int offset=0;
	int sendlen=0;
	unsigned int bytes_received = 0;

    ALOGE(": function is mux_pcm_write,count:%d",count);
    if( !pcm){
        return 0;
    }

    if(pcm->mux_fd >0){
        pthread_mutex_lock(&pcm->lock);
        if(!pcm->state) {
            pcm->state = 1;
            ret = saudio_send_common_cmd(pcm,SAUDIO_CMD_START,pcm->stream_type);
            if(ret){
                pthread_mutex_unlock(&pcm->lock);
                return 0;
            }
            mux_pipe_clear(pcm->mux_fd);
        }
        pthread_mutex_unlock(&pcm->lock);

        while(left) {
		bytes_received = 0;
	        sendlen = left < (AUDIO_PLAYBACK_BUFFER -pcm->bytes_written) ? left : (AUDIO_PLAYBACK_BUFFER -pcm->bytes_written);

		ALOGE("mux_pcm_write in %d",sendlen);
		bytes= write(pcm->mux_fd,data,sendlen);
		if(bytes > 0)
		{
			pcm->bytes_written += bytes;
			left -= bytes;
			data = (char * ) data + bytes;
			if(pcm->bytes_written >= AUDIO_PLAYBACK_BUFFER) {
				ret = mux_write_wait_response(pcm, pcm->bytes_written,&bytes_received);
				pcm->bytes_written -= bytes_received;
				if(ret ) {
					ALOGE("mux_pcm_write:mux_write_wait_response error bytes %d, bytes_received %d",bytes,bytes_received);
					break;
				}
			}

		}
		  else if((!((errno == EAGAIN) || (errno == EINTR))) || (0 == ret)) {
                	ALOGE("pipe write error %d",errno);
                	break;
	            }
		    else {
			ALOGE("mux_pcm_write write error:sendlen %d bytes %d,left :%d",sendlen,bytes,left);
		    }
	    }
    }



  ALOGE(": function is mux_pcm_write out count %d,left %d",count,left);
 return ret<0?-1:(count -left);
}



int mux_pcm_read(struct pcm *pcm_in, void *data, unsigned int count)
{
     struct mux_pcm  *pcm=(struct mux_pcm  *)pcm_in;
     int ret=0;
    int bytes=count;
    int bytes_read=0;
    ALOGE(": function is mux_pcm_read in");
    if( !pcm){
        return 0;
    }
    if(pcm->mux_fd > 0){
        pthread_mutex_lock(&pcm->lock);
        if(!pcm->state) {
            pcm->state = 1;
            ret =saudio_send_common_cmd(pcm,SAUDIO_CMD_START,pcm->stream_type);
            if(ret){
                pthread_mutex_unlock(&pcm->lock);
                return -1;
            }
        }
         pthread_mutex_unlock(&pcm->lock);
        ALOGE("mux_pcm_read in %d",count);
        while(bytes){
           bytes_read= mux_read(pcm->mux_fd,data,bytes,10);
           if(bytes_read < 0) {
		break;
           }
           data = (char *) data + bytes_read;
           bytes -= bytes_read;
       }
       ALOGE("mux_pcm_read out %d",bytes);
    }
    ALOGE(": function is mux_pcm_read out");
    return bytes==0?0:-1;
}


int mux_pcm_close(struct pcm *pcm_in)
{
     struct mux_pcm  *pcm=(struct mux_pcm  *)pcm_in;
    int ret=0;
    ALOGE(": function is mux_pcm_close in pcm->state is%d",pcm->state);
    if( !pcm){
        return -1;
    }
    if(pcm->mux_fd>0){
        pthread_mutex_lock(&pcm->lock);
        if(pcm->state == 1){
            ret = saudio_send_common_cmd(pcm,SAUDIO_CMD_STOP,pcm->stream_type);
            if(ret){
                pthread_mutex_unlock(&pcm->lock);
                ret= close(pcm->mux_fd);
                pcm->mux_fd=-1;
                free(pcm);
                return ret;
            }
            pcm->state = 0;
        }
        pthread_mutex_unlock(&pcm->lock);
        ret = saudio_send_common_cmd(pcm,SAUDIO_CMD_CLOSE,pcm->stream_type);
        if(ret){
            ret= close(pcm->mux_fd);
            pcm->mux_fd=-1;
            free(pcm);
            return ret;
        }
        ret= close(pcm->mux_fd);
        pcm->mux_fd=-1;
        pcm->bytes_written = 0;


    }

     if(pcm->card) {
     	mux_snd_card_release(pcm->card, pcm->dummy_pcm.flags);
     	ALOGE(": function is mux_pcm_close outstate is %x", pcm->card->state );
     	pcm->card = NULL;
    }

    free(pcm);

    return ret;
}
