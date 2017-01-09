#include "testitem.h"

#define NUM_ELEMS(x) (sizeof(x)/sizeof(x[0]))

char s_modem_ver[1024];
char s_cali_info[1024];
char s_cali_info1[1024];
char s_cali_info2[1024];
int s_sim_state = 2;
char imei_buf1[128];
char imei_buf2[128];

static char s_ATBuffer[AT_BUFFER_SIZE];
static char s_ATDictBuf[AT_BUFFER_SIZE];
static char *s_ATBufferCur = s_ATBuffer;
static int thread_run = 0;
static int modem_fd[MAX_MODEM_COUNT];
static char modem_port[MAX_MODEM_COUNT][BUF_LEN];
static char  modem_count_prop[BUF_LEN];

static int s_modem_count = 0;
static int init_mfalg= 0;
pthread_mutex_t tel_mutex = PTHREAD_MUTEX_INITIALIZER;

char* sim_name[] = {
	TEXT_SIM1,
	TEXT_SIM2
};

static const char * s_finalResponse[] = {
	"OK",
	"ERROR",
};

/** returns 1 if line starts with prefix, 0 if it does not */
static int strStartsWith(const char *line, const char *prefix)
{
	for ( ; *line != '\0' && *prefix != '\0' ; line++, prefix++) {
		if (*line != *prefix) {
			return 0;
		}
	}

	return *prefix == '\0';
}
/**
 * returns 1 if line is a final response, either  error or success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */

static int isFinalResponse(const char *line)
{
	size_t i;

	for (i = 0 ; i < NUM_ELEMS(s_finalResponse) ; i++) {
		if (strStartsWith(line, s_finalResponse[i])) {
			return 1;
		}
	}

	return 0;
}

/*
 * Returns a pointer to the end of the next line
 * special-cases the "> " SMS prompt
 *
 * returns NULL if there is no complete line
 */
static char * findNextEOL(char *cur)
{
	if (cur[0] == '>' && cur[1] == ' ' && cur[2] == '\0') {
		/* SMS prompt character...not \r terminated */
		return cur+2;
	}

	// Find next newline
	while (*cur != '\0' && *cur != '\r' && *cur != '\n') cur++;

	return *cur == '\0' ? NULL : cur;
}

static char *eng_readline(int modemfd)
{
	ssize_t count;

	char *p_read = NULL;
	char *p_eol = NULL;
	char *ret;

	/* this is a little odd. I use *s_ATBufferCur == 0 to
	 * mean "buffer consumed completely". If it points to a character, than
	 * the buffer continues until a \0
	 */

	if (*s_ATBufferCur == '\0') {
		/* empty buffer */
		s_ATBufferCur = s_ATBuffer;
		*s_ATBufferCur = '\0';
		p_read = s_ATBuffer;
	} else {   /* *s_ATBufferCur != '\0' */
		/* there's data in the buffer from the last read */

		// skip over leading newlines
		while (*s_ATBufferCur == '\r' || *s_ATBufferCur == '\n')
			s_ATBufferCur++;

		p_eol = findNextEOL(s_ATBufferCur);

		if (p_eol == NULL) {
			/* a partial line. move it up and prepare to read more */
			size_t len;

			len = strlen(s_ATBufferCur);

			memmove(s_ATBuffer, s_ATBufferCur, len + 1);
			p_read = s_ATBuffer + len;
			s_ATBufferCur = s_ATBuffer;
		}
		/* Otherwise, (p_eol !- NULL) there is a complete line  */
		/* that will be returned the while () loop below        */
	}


	while (p_eol == NULL) {
		if (0 == AT_BUFFER_SIZE - (p_read - s_ATBuffer)) {
			/* ditch buffer and start over again */
			s_ATBufferCur = s_ATBuffer;
			*s_ATBufferCur = '\0';
			p_read = s_ATBuffer;
		}
		do {
			count = read(modemfd, p_read,
					AT_BUFFER_SIZE - (p_read - s_ATBuffer));
		} while (count < 0 && errno == EINTR);

		if (count > 0) {
			p_read[count] = '\0';
			// skip over leading newlines
			while (*s_ATBufferCur == '\r' || *s_ATBufferCur == '\n')
				s_ATBufferCur++;

			p_eol = findNextEOL(s_ATBufferCur);
			p_read += count;
		} else if (count <= 0) {
			return NULL;
		}
	}

	/* a full line in the buffer. Place a \0 over the \r and return */
	ret = s_ATBufferCur;
	*p_eol = '\0';
	s_ATBufferCur = p_eol + 1; /* this will always be <= p_read,    */
	/* and there will be a \0 at *p_read */
	return ret;
}

/**************************************************************************************/
int modem_send_at(int fd, char* cmd, char* buf, unsigned int buf_len, int wait)
{
	struct timeval timeout;
	int ret = -1;
	int cmd_len = 0;
	int rsp_len = 0;
	char* line = NULL;
	fd_set readfs;

	if(fd < 0 && modem_fd[0] > 0) {
		fd = modem_fd[0];
	}
	if(NULL == cmd) {
		LOGE("error param");
		return -1;
	}

	LOGD("[fd:%d] >>>> at_cmd: %s", fd, cmd);
	cmd_len = strlen(cmd);
	ret = write(fd, cmd, cmd_len);
	if(ret != cmd_len) {
		LOGE("mmitest write err, ret=%d, cmd_len=%d\n",  ret, cmd_len);
		return -1;
	}
	write(fd, "\r", 1);

	if(wait <= 0) wait = 5;

	for(;;) {
		timeout.tv_sec = wait;
		timeout.tv_usec = 0;
		FD_ZERO(&readfs);
		FD_SET(fd, &readfs);
		ret = select(fd+1, &readfs, NULL, NULL, &timeout);
		if (ret < 0) {
			LOGE("mmitest select err ");
			if(errno == EINTR || errno == EAGAIN) {
				continue;
			} else {
				return -1;
			}
		} else if(ret == 0) {
			LOGD("mmitest select time out");
			return -1;
		} else {
			/* go to recv response*/
			break;
		}
	}

	for(;;) {
		line = eng_readline(fd);
		LOGD("mmitest %s [fd:%d] <<<< at_rsp: %s",cmd, fd, line);
		if(strstr(line, "OK"))
		{
			ret = rsp_len;
			break;
		} else if(strstr(line, "ERROR"))
		{
			ret = -1;
			break;
		}
		else {
			if(buf_len == 0 || buf == NULL) {
				continue;
			}
			if(rsp_len + strlen(line) > buf_len) {
				LOGD("mmitest  recv too many word, (%d) > (%d)\n",
						 (rsp_len + strlen(line)), buf_len);
				ret = -1;
				break;
			}
			memcpy(buf+rsp_len, line, strlen(line));
			rsp_len += strlen(line);
		}
	}
	return ret;
}


int tel_send_at(int fd, char* cmd, char* buf, unsigned int buf_len, int wait)
{
	struct timeval timeout;
	int ret = -1;
	int cmd_len = 0;
	int rsp_len = 0;
	char* line = NULL;
	fd_set readfs;

	if(NULL == cmd) {
		LOGD("error param");
		return -1;
	}

	LOGD("[fd:%d] >>>> at_cmd: %s", fd, cmd);
	cmd_len = strlen(cmd);
	ret = write(fd, cmd, cmd_len);
	if(ret != cmd_len) {
		LOGE("mmitest write err, ret=%d, cmd_len=%d", ret, cmd_len);
		return -1;
	}
	write(fd, "\r", 1);

	if(wait <= 0) wait = 5;

	for(;;) {
		timeout.tv_sec = wait;
		timeout.tv_usec = 0;
		FD_ZERO(&readfs);
		FD_SET(fd, &readfs);
		ret = select(fd+1, &readfs, NULL, NULL, &timeout);
		if (ret < 0) {
			LOGE("mmitest select err");
			if(errno == EINTR || errno == EAGAIN) {
				continue;
			} else {
				return -1;
			}
		} else if(ret == 0) {
			LOGD("mmitest select time out");
			return -1;
		} else {
			/* go to recv response*/
			break;
		}
	}

	for(;;) {
		line = eng_readline(fd);
		LOGD("mmitest %s [fd:%d] <<<< at_rsp: %s",cmd, fd, line);
		if(strstr(line, "OK"))
		{
			ret = rsp_len;
			break;
		} else if(strstr(line, "ERROR")){
			ret = -1;
			break;
		}else {
			if(buf_len == 0 || buf == NULL) {
				continue;
			}
			if(rsp_len + strlen(line) > buf_len) {
				LOGD("mmitest recv too many word, (%d) > (%d)",
						 (rsp_len + strlen(line)), buf_len);
				ret = -1;
				break;
			}
			memcpy(buf+rsp_len, line, strlen(line));
			rsp_len += strlen(line);
		}
	}
	return ret;
}

void* modem_init_func()
{
	char* at_rsp = NULL;
	int pos = 0;
	int i = 0;
	char modem_prop[512];
	char property[PROPERTY_VALUE_MAX];
	char tmp[512];
	char* ptmp = NULL;
	int sim_state;
	pthread_detach(pthread_self()); 	//free by itself

	property_get(PROP_MODEM_LTE_ENABLE, property, "not_find");
	if(!strcmp(property, "1")){
	    sprintf(modem_count_prop,PROP_MODEM_LTE_COUNT);
	    property_get(modem_count_prop, modem_prop, "");
	    sprintf(modem_port[0],"/dev/stty_lte1");
	    sprintf(modem_port[1],"/dev/stty_lte4");
	}else{
	    sprintf(modem_count_prop,PROP_MODEM_W_COUNT);
	    property_get(modem_count_prop, modem_prop, "");
	    sprintf(modem_port[0],"/dev/stty_w1");
	    sprintf(modem_port[1],"/dev/stty_w4");
	}
	s_modem_count = atoi(modem_prop);
	LOGD("mmitest get %s = %d", modem_count_prop, s_modem_count);
	if(s_modem_count > 0 && s_modem_count <= MAX_MODEM_COUNT) {
		for(i = 0; i < s_modem_count; i++) {
			while(1) {
				modem_fd[i] = open(modem_port[i], O_RDWR);
				LOGD("open %s = %d",modem_port[i],modem_fd[i]);
				if(modem_fd[i] > 0) {
					break;
				}
				usleep(1000);
			}
		}
	} else {
		return NULL;
	}
	pthread_mutex_lock(&tel_mutex);
	if(modem_send_at(modem_fd[0], "AT", NULL, 0, 0) < 0) return NULL;
	switch(s_modem_count) {
		case 2:
			if(modem_send_at(modem_fd[0], "AT+SMMSWAP=0", NULL, 0, 0) < 0) return NULL;
			break;
		case 3:
			if(modem_send_at(modem_fd[0], "AT+SMMSWAP=1", NULL, 0, 0) < 0) return NULL;
			break;
	}


	if((pos = modem_send_at(modem_fd[0], "AT+CGMR", s_modem_ver, sizeof(s_modem_ver), 0)) < 0) return NULL;
	s_modem_ver[pos] = '\0';
	LOGD("get modem version[%d]: %s",strlen(s_modem_ver), s_modem_ver);

	if((pos = modem_send_at(modem_fd[0], "AT+SGMR=1,0,3,0", s_cali_info, sizeof(s_cali_info), 0)) < 0) return NULL;
	strcat(s_cali_info,"BIT");
	s_cali_info[pos+3] = '\0';
	LOGD("get cali info[%d]: %s",strlen(s_cali_info), s_cali_info);


	if((pos = modem_send_at(modem_fd[0], "AT+SGMR=1,0,3,1", s_cali_info1, sizeof(s_cali_info1), 0)) < 0) return NULL;
	strcat(s_cali_info1,"BIT");
	s_cali_info1[pos+3] = '\0';
	LOGD("get cali info1[%d]: %s",strlen(s_cali_info1), s_cali_info1);

	if(!strcmp(property, "1")){
	    if((pos = modem_send_at(modem_fd[0], "AT+SGMR=1,0,3,3", s_cali_info2, sizeof(s_cali_info2), 0)) < 0) return NULL;
	    strcat(s_cali_info2,"BIT");
	    s_cali_info2[pos+3] = '\0';
	    LOGD("get cali info2[%d]: %s", strlen(s_cali_info2), s_cali_info2);
      }
	if((pos = modem_send_at(modem_fd[0], "AT+CGSN", imei_buf1, sizeof(imei_buf1), 0)) < 0) return NULL;

	imei_buf1[pos] = '\0';
	LOGD("get imei[%d]: %s", strlen(imei_buf1), imei_buf1);

	if(s_modem_count == 2){
		if((pos = modem_send_at(modem_fd[1], "AT+CGSN", imei_buf2, sizeof(imei_buf2), 0)) < 0) return NULL;

		imei_buf2[pos] = '\0';
		LOGD("get imei[%d]: %s", strlen(imei_buf2), imei_buf2);
	}
	if(strcmp(property, "1")){
	    for(i = 0; i < s_modem_count; i++) {
			if(modem_send_at(modem_fd[i], "AT+SFUN=2", NULL, 0, 10)<0){
				LOGD("AT+SFUN=2 open sim card %d failed",i);
			}
	    }
	}
	init_mfalg = 1;
	pthread_mutex_unlock(&tel_mutex);

	return NULL;
}

char* test_modem_get_ver(void)
{
	return s_modem_ver;
}

char* test_modem_get_caliinfo(void)
{
	return s_cali_info;
}

void* sim_check_thread()
{
	int i;
	char* at_rsp = NULL;
	char tmp[512];
	char* ptmp = NULL;
	int sim_state;
	int test_result = 0;
	int cur_row = 5;
	int ret = RL_FAIL;

	for(i = 0; i < s_modem_count; i++) {
		if(thread_run != 1) return NULL;
		if(modem_send_at(modem_fd[i], "AT+EUICC?", tmp, sizeof(tmp), 0) < 0) {
			sim_state = -1;
		} else {
			ptmp = tmp;
			ptmp = strstr(ptmp, "EUICC");
			LOGD("sim_check_thread line =%s", ptmp);
			if(ptmp){
				eng_tok_start(&ptmp);
				eng_tok_nextint(&ptmp, &sim_state);
			}else{
				sim_state = -1;
			}
		}
		LOGD("get sim%d, state=%d", i, sim_state);
		ui_set_color(CL_WHITE);
		cur_row = ui_show_text(cur_row+1, 0, sim_name[i]);
		if(sim_state == 0) {
			ui_set_color(CL_GREEN);
			cur_row = ui_show_text(cur_row, 0, TEXT_PASS);
			test_result++;
		} else {
			ui_set_color(CL_RED);
			cur_row = ui_show_text(cur_row, 0, TEXT_FAIL);
		}
	}
	s_sim_state = test_result;
	//update
	if(test_result != s_modem_count) {
		ret = RL_FAIL; //fail
		ui_set_color(CL_RED);
		cur_row = ui_show_text(cur_row+1, 0, TEXT_TEST_FAIL);
	} else {
		ret = RL_PASS; //pass
		ui_set_color(CL_GREEN);
		cur_row = ui_show_text(cur_row+1, 0, TEXT_TEST_PASS);
	}
	ui_push_result(ret);
	ui_clear_rows(4, 1);
	ui_set_color(CL_WHITE);
	ui_show_text(4, 0, TEXT_SIM_RESULT);
	gr_flip();
	sleep(1);
	return NULL;
}

int test_sim_pretest(void)
{
	int i;
	char tmp[512];
	char* ptmp = NULL;
	int sim_state;
	int test_result = 0;
	int ret = RL_FAIL;

	for(i = 0; i < s_modem_count; i++) {
        if(modem_send_at(modem_fd[i], "AT+EUICC?", tmp, sizeof(tmp), 0) < 0) {
            sim_state = -1;
        } else {
			ptmp = tmp;
			ptmp = strstr(ptmp, "EUICC");
			LOGD("test_sim_pretest   line =%s", ptmp);
			if(ptmp){
				eng_tok_start(&ptmp);
				eng_tok_nextint(&ptmp, &sim_state);
			}else{
				sim_state = -1;
			}
        }
        LOGD("mmitest get sim%d, state=%d", i, sim_state);
        if(sim_state == 0) {
            test_result++;
        }
	}
    s_sim_state = test_result;

    if(s_sim_state != s_modem_count) {
        ret= RL_FAIL;
    } else {
        ret= RL_PASS;
    }
    save_result(CASE_TEST_SIMCARD,ret);
    return ret;
}

int test_sim_start(void)
{
	int ret = 0;
	pthread_t t;
	char property[PROPERTY_VALUE_MAX];
	property_get(PROP_MODEM_LTE_ENABLE, property, "not_find");

	ui_fill_locked();
	ui_show_title(MENU_TEST_SIMCARD);
	if((!strcmp(property, "1")) && (0 == init_mfalg) ){   //7731 should send init sim card AT first
		ui_set_color(CL_WHITE);
		ui_show_text(2,0,TEXT_MODEM_INITING);
		gr_flip();
		ret=RL_NA;
		sleep(1);
	}else{
		ui_set_color(CL_WHITE);
		ui_show_text(2, 0, TEXT_SIM_SCANING);
		gr_flip();
		thread_run = 1;
		pthread_create(&t, NULL, (void*)sim_check_thread, NULL);
		ret = ui_handle_button(NULL,NULL,NULL);
		thread_run = 0;
		pthread_join(t, NULL);
	}

	save_result(CASE_TEST_SIMCARD,ret);
	return ret;
}
