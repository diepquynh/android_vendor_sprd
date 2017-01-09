#include <sys/select.h>
#include <fcntl.h>

pthread_t mmi_audio_loop;

void *mmi_audio_loop_thread(void *args);

int mmi_audio_loop_open()
{
	
	if(pthread_create(&mmi_audio_loop, NULL, mmi_audio_loop_thread, NULL)) {
        ALOGE("mmi_audio_loop_thread creating failed !!!!");
		return -1;
	}
	return 0;
}

inline int mmi_pop(char **start, char **ptr, char c, int *len)
{
    char *tmp;
    *ptr = 0;
    if (*len == 0) return 0;
    tmp = strchr(*start, c);
    if (tmp) {
        if ((tmp - *start + 1) > *len) return 0;
        *tmp++ = 0;
        *len -= tmp - *start;
        *ptr = *start;
        *start = tmp;
        return 1;
    }
    return 0;
}

inline int mmi_pop_digit(char **start, char **ptr, int *len)
{
    char *p = *start;
    *ptr = 0;
    if (*len == 0) return 0;
    for (; ((p - *start) < *len) && isdigit(*p); p++);
    if (p != *start) {
        if (isdigit(*p)) return 0;
        if ((p - *start) >= *len) return 0;
        *ptr = *start;
        *p++ = 0;
        *len -= p - *start;
        *start = p;
        return 1;
    }
    return 0;
}

int headset_no_mic()
{
    char buf[12] = {'\0'};
    const char* headsetStatePath = "/sys/class/switch/h2w/state";

    int fd = open(headsetStatePath, O_RDONLY);
    if(fd < 0) {
        ALOGE("open failed %s ", strerror(errno));
    } else {
        ssize_t mBytesRead = read(fd, (char*)buf, 12);
        close(fd);

        if(mBytesRead > 0) {
            int value = atoi((char*)buf);
            // headset has mic
            if(value == 2) {
                ALOGW("Headset has no mic");
                return 1;
            } else if(value == 1) {
                ALOGW("Headset has mic");
            } else {
                ALOGW("No Headset detect");
                return 1;
            }
        }
    }
    return 0;
}

int mmi_audio_mode_map(int ap_mode)
{
    int cp_mode = 0;
    if(ap_mode == 1) {
        cp_mode = 0;
    } else if(ap_mode == 2){
        cp_mode = 1;
    } else if(ap_mode == 4) {
        if(headset_no_mic()) {
            cp_mode = 4;
        } else {
            cp_mode = 2;
        }
    }
    return cp_mode;
}

inline int mmi_pop_digit2no(char **start, char **ptr, int *len)
{
    char *p = *start;
    *ptr = 0;
    if (*len == 0) return 0;
    for (; ((p - *start) < *len) && !isdigit(*p); *p++ = 0);
    if (p != *start) {
        /* if (!isdigit(*p)) return 0; */
        if ((p - *start) >= *len) return 0;
        *ptr = *start;
        *p++ = 0;
        *len -= p - *start;
        *start = p;
        return 1;
    } else *ptr = 0;
    return 0;
}

void *mmi_audio_loop_thread(void *args)
{
    int retval, cmd, i;
    fd_set rfds;
    int loop_back = 0;
    int fd_save_loop_back_data = 0;
    uint32_t loop_back_buffer_size = 0;
    char *loop_back_buffer = NULL;
    char r_buf[128], *p;
    int mmi_audio_ctrl, max_fd;

	ALOGE("peter: open /dev/pipe/mmi.audio.ctrl in\n");
    mmi_audio_ctrl = open("/dev/pipe/mmi.audio.ctrl", O_RDWR);
    if(mmi_audio_ctrl < 0){
        ALOGE("%s, open pipe error!! ",__func__);
        return NULL;
    }
    max_fd = mmi_audio_ctrl;
    memset(r_buf, 0, sizeof r_buf);
    
    
   
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(mmi_audio_ctrl, &rfds);
        ALOGE("mmi_audio_loop_thread waiting for new command");
        ALOGE("peter: slect in");
     
        retval = select(max_fd + 1, &rfds, NULL, NULL,  NULL);
        if (retval == -1) {
           ALOGW("================== [ BUG mmi audio loop pipe error ] ==================");
           usleep(200*1000);
           continue;
        }
        ALOGE("peter: slect out");
              
        if (FD_ISSET(mmi_audio_ctrl, &rfds)) {
            char *ptr;
            int exit = 0;
            int len = read(mmi_audio_ctrl, r_buf, sizeof(r_buf) - 1);
            // the format is like this : cmd,arg0,arg1@@  @@means save record data to /data/mmi_audio_loop_record_xx_xx_xx.raw
            p = r_buf;
            while ((exit == 0) && mmi_pop(&p, &ptr, ',', &len)) {
                cmd = strtol(ptr, NULL, 0);
                ALOGW("mmi_audio_loop: cmd is %d",cmd);
                switch (cmd) {
                    case 0: {
                            at_cmd_audio_loop(0,0,0,0,0,0);
                            property_set("media.mmi.audio.loop", "0");
					}		
	                break;
                    case 1: {
                            int mode = 0, vol = 100;
                            int loopbacktype=1;
                            int voiceformat=0;
                            int delaytime=0;
                            exit=0;
                            if (mmi_pop(&p, &ptr, ',', &len)) mode = strtol(ptr, NULL, 0);
                            else exit = 1;
                            if (mmi_pop_digit(&p, &ptr, &len)) vol = strtol(ptr, NULL, 0);
                            else exit = 1;
                            
                            if(mmi_pop(&p,&ptr,',',&len)) {
                                loopbacktype=strtol(ptr,NULL,0);
                                 if(mmi_pop(&p,&ptr,',',&len)) {
                                    voiceformat=strtol(ptr,NULL,0);
                                    if(mmi_pop_digit(&p,&ptr,&len))  {
                                        delaytime=strtol(ptr,NULL,0);
                                    }
                                }
                            }

                           ALOGW("mmi_audio_loop:cmd:%d,mode %d,volume:%d,loopbacktype:%d,voiceformat:%d,delaytime:%d\n",
                                                cmd,mode,vol,loopbacktype,voiceformat,delaytime);
                            if (exit == 0) {
                                    property_set("media.mmi.audio.loop", "1");
                                    mode=mmi_audio_mode_map(mode);
                                    at_cmd_audio_loop(cmd,mode,vol,loopbacktype,voiceformat,delaytime);
                            }
                        } 
                        break;
                   case 2: {
                            int mode = 0;
                            if (mmi_pop_digit(&p, &ptr,  &len)) {
                                mode = strtol(ptr, NULL, 0);
                                ALOGW("mmi_audio_loop:cmd:%d,mode %d\n",cmd,mode);
                                mode=mmi_audio_mode_map(mode);
                                at_cmd_audio_loop(cmd,mode,0,0,0,0);
                            }
                     }
                    break;
                   case 3: {
                            int vol = 100;
                            if (mmi_pop_digit(&p, &ptr, &len)) {
                                vol = strtol(ptr, NULL, 0);
                                ALOGW("mmi_audio_loop:cmd:%d,vol %d\n",cmd,vol);
                                at_cmd_audio_loop(cmd,0,vol,0,0,0);
                            }
                     }
                    break;
                    default: ALOGW("mmi_audio_loop_thread not support this cmd : %s", r_buf); break;
				}
                mmi_pop_digit2no(&p,&ptr,&len);
                exit=1;
            }
        }
       
    }
    return NULL;
}

