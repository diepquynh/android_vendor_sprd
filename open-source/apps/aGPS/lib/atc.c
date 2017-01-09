/*
*
* Copyright (C) 2008 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* see the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <atc.h>

struct at_channel atcs[2];
int debug_mode = 1;

static work_queue_t *work_queue_new(void)
{
	work_queue_t *work_q;

	work_q = (work_queue_t *) mmalloc(sizeof(work_queue_t));

	work_q->head = work_q->tail = NULL;
	work_q->item_count = 0;
	return work_q;
}

static work_item_t *work_queue_add(work_queue_t *work_q, void *data)
{
	work_item_t *work_item;

	if (!work_q) {
		return NULL;
	}
	work_item = (work_item_t *) mmalloc(sizeof(work_item_t));
	work_item->next = NULL;
	work_item->data = data;
	gettimeofday(&(work_item->time_queued), NULL);

	if (work_q->head == NULL) {
		work_q->head = work_q->tail = work_item;
		work_q->item_count = 1;
	} else {
		work_q->tail->next = work_item;
		work_q->tail = work_item;
		work_q->item_count++;
	}
	return work_item;
}

static void *work_queue_pop(work_queue_t *work_q)
{
	work_item_t *work_item;
	void *data;

	if (!work_q || !work_q->head) {
		return NULL;
	}
	work_item = work_q->head;
	data = work_item->data;
	work_q->head = work_item->next;
	if (work_q->head == NULL) {
		work_q->tail = NULL;
	}
	free(work_item);
	return data;
}

static void *work_queue_delete(work_queue_t *work_q, work_item_t *work_item)
{
	work_item_t *prev, *curr;
	void *data;

	if (!work_q || !work_q->head || !work_item) {
		return NULL;
	}
	data = work_item->data;
	if (work_q->head == work_item) {
		work_q->head = work_item->next;
	} else {
		prev = work_q->head;
		curr = work_q->head->next;
		while (curr) {
			if (curr == work_item) {
				break;
			}
			prev = curr;
			curr = curr->next;
		}

		if (curr == NULL) {
			dprintf("can't locate work_item %p\n", work_item);
			return NULL;
		}

		prev->next = curr->next;

		if (work_q->tail == curr)
			work_q->tail = prev;
	}

	if (work_q->head == NULL) {
		work_q->tail = NULL;
	}
	free(work_item);
	return data;
}

/**
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

/**
 * Reads a line from the AT channel, returns NULL on timeout.
 * Assumes it has exclusive read access to the FD
 *
 * This line is valid only until the next call to readline
 *
 * This function exists because as of writing, android libc does not
 * have buffered stdio.
 */
static const char *readline(struct at_channel *atc)
{
	ssize_t count;

	char *p_read = NULL;
	char *p_eol = NULL;
	char *ret;

	/* this is a little odd. I use *atc->s_ATBufferCur == 0 to
	 * mean "buffer consumed completely". If it points to a character, than
	 * the buffer continues until a \0
	 */
	if (*atc->s_ATBufferCur == '\0') {
		/* empty buffer */
		atc->s_ATBufferCur = atc->s_ATBuffer;
		*atc->s_ATBufferCur = '\0';
		p_read = atc->s_ATBuffer;
	} else {   /* *atc->s_ATBufferCur != '\0' */
		/* there's data in the buffer from the last read */

		// skip over leading newlines
		while (*atc->s_ATBufferCur == '\r' || *atc->s_ATBufferCur == '\n')
			atc->s_ATBufferCur++;

		p_eol = findNextEOL(atc->s_ATBufferCur);

		if (p_eol == NULL) {
			/* a partial line. move it up and prepare to read more */
			size_t len;

			len = strlen(atc->s_ATBufferCur);

			memmove(atc->s_ATBuffer, atc->s_ATBufferCur, len + 1);
			p_read = atc->s_ATBuffer + len;
			atc->s_ATBufferCur = atc->s_ATBuffer;
		}
		/* Otherwise, (p_eol !- NULL) there is a complete line  */
		/* that will be returned the while () loop below        */
	}

	while (p_eol == NULL) {
		if (0 == MAX_AT_RESPONSE - (p_read - atc->s_ATBuffer)) {
			dprintf("ERROR: Input line exceeded buffer\n");
			/* ditch buffer and start over again */
			atc->s_ATBufferCur = atc->s_ATBuffer;
			*atc->s_ATBufferCur = '\0';
			p_read = atc->s_ATBuffer;
		}

		do {
			/* if O_NONBLOCK is set in fd, -1 will return and errno is EAGAIN */
			count = read(atc->fd, p_read,
					MAX_AT_RESPONSE - (p_read - atc->s_ATBuffer));
		} while (count < 0 && errno == EINTR);

		if (count > 0) {
			// s_readCount += count;

			p_read[count] = '\0';

			// skip over leading newlines
			while (*atc->s_ATBufferCur == '\r' || *atc->s_ATBufferCur == '\n')
				atc->s_ATBufferCur++;

			p_eol = findNextEOL(atc->s_ATBufferCur);
			p_read += count;
		} else if (count <= 0) {
			/* read error encountered or EOF reached */
			if(count == 0) {
				dprintf("atchannel: EOF reached");
			} else {
				if (errno != EAGAIN)
					dprintf("atchannel: read error %s", strerror(errno));
			}
			return NULL;
		}
	}

	/* a full line in the buffer. Place a \0 over the \r and return */

	ret = atc->s_ATBufferCur;
	*p_eol = '\0';
	atc->s_ATBufferCur = p_eol + 1; /* this will always be <= p_read,    */
	/* and there will be a \0 at *p_read */

	dprintf("AT< %s\n", ret);
	return ret;
}

/**
 * Sends string s to the radio with a \r appended.
 * Returns AT_ERROR_* on error, 0 on success
 *
 * This function exists because as of writing, android libc does not
 * have buffered stdio.
 */
static int writeline(struct at_channel *atc, char *s, size_t len)
{
	size_t cur = 0;
	ssize_t written;

	if (atc->fd < 0) {
		return -1;
	}
/*at cmd finished with \r */
	strcat(s, "\r");
	len++;

	dprintf("AT> %s\n", s);

	/* the main string */
	while (cur < len) {
		do {
			written = write(atc->fd, s + cur, len - cur);
		} while (written < 0 && errno == EINTR);

		if (written < 0) {
			/* if O_NONBLOCK is set in fd, -1 will return and errno is EAGAIN */
			if (errno != EAGAIN) {
				return -2;
			}
			dprintf("atchannel: write not enough space, wait 100ms\n");
#ifdef DEBUG_SIPC
			dprintf("prev right data: %s\n", atc->sipc_buf);
			read(atc->dfd, atc->sipc_buf, sizeof(atc->sipc_buf));
			dprintf("curr wrong data: %s\n", atc->sipc_buf);
#endif
			usleep(100 * 1000);
			continue;
		}
#ifdef DEBUG_SIPC
		else {
			read(atc->dfd, atc->sipc_buf, sizeof(atc->sipc_buf));
		}
#endif

		cur += written;
	}

	return 0;
}

#ifndef USE_NP
static void setTimespecRelative(struct timespec *p_ts, long long msec)
{
	struct timeval tv;

	gettimeofday(&tv, (struct timezone *) NULL);

	/* what's really funny about this is that I know
	   pthread_cond_timedwait just turns around and makes this
	   a relative time again */
	p_ts->tv_sec = tv.tv_sec + (msec / 1000);
	p_ts->tv_nsec = (tv.tv_usec + (msec % 1000) * 1000L ) * 1000L;
}
#endif /*USE_NP*/

/**
 * returns 1 if line is a final response indicating error
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * s_finalResponsesError[] = {
	"ERROR",
	"+CMS ERROR:",
	"+CME ERROR:",
	"NO CARRIER", /* sometimes! */
	"NO ANSWER",
	"NO DIALTONE",
};
static int isFinalResponseError(const char *line)
{
	size_t i;

	for (i = 0 ; i < NUM_ELEMS(s_finalResponsesError) ; i++) {
		if (strStartsWith(line, s_finalResponsesError[i])) {
			return 1;
		}
	}

	return 0;
}

/**
 * returns 1 if line is a final response indicating success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * s_finalResponsesSuccess[] = {
	"OK",
	"CONNECT"       /* some stacks start up data on another channel */
};
static int isFinalResponseSuccess(const char *line)
{
	size_t i;

	for (i = 0 ; i < NUM_ELEMS(s_finalResponsesSuccess) ; i++) {
		if (strStartsWith(line, s_finalResponsesSuccess[i])) {
			return 1;
		}
	}

	return 0;
}

/**
 * returns 1 if line is a final response, either  error or success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static int isFinalResponse(const char *line)
{
	return isFinalResponseSuccess(line) || isFinalResponseError(line);
}

/** assumes s_commandmutex is held */
static void handleFinalResponse(struct at_channel *atc, const char *line)
{
	pthread_mutex_lock(&atc->response_cond_mutex);
	if (atc->atrep.finalResponse)
		free(atc->atrep.finalResponse);
	atc->atrep.finalResponse = strdup(line);
	pthread_cond_signal(&atc->response_cond);
	pthread_mutex_unlock(&atc->response_cond_mutex);
}

static struct at_channel *atc_try_init(struct at_channel *atc)
{
	static pthread_mutex_t auto_init_mutex = PTHREAD_MUTEX_INITIALIZER;
	if (atc == NULL) {
		atc = &atcs[0];
		if (atc->fd <= 0) {
			pthread_mutex_lock(&auto_init_mutex);
			if (atc->fd <= 0)
				atc = atc_init(NULL, O_RDWR/* | O_NONBLOCK*/, -1);
			pthread_mutex_unlock(&auto_init_mutex);
		}
	}
	return atc;
}

static void match_unsolicited_cb(struct at_channel *atc, const char *line)
{
	work_item_t *curr;
	struct at_unso_node *unode;
	int found = 0;
	pthread_mutex_lock(&atc->queue_mutex);
	curr = atc->queue->head;
	while (curr) {
		unode = curr->data;
		if (strStartsWith(line, unode->prefix)) {
			/* we can queue all cb in one queue in future */
			if (unode->cb) unode->cb(line, strlen(line));
			found = 1;
		}
		curr = curr->next;
	}
	pthread_mutex_unlock(&atc->queue_mutex);
	if (!found) {
		dprintf("%s not find [ %s ] callback\n", __func__, line);
	}
}

static void processLine(struct at_channel *atc, const char *line)
{
	if (isFinalResponseSuccess(line)) {
		atc->atrep.success = 1;
		handleFinalResponse(atc, line);
	} else if (isFinalResponseError(line)) {
		atc->atrep.success = 0;
		handleFinalResponse(atc, line);
	} else {
		/* unsolicited or async response AT */
		match_unsolicited_cb(atc, line);
	}
}

static void *readerLoop(void *arg)
{
	struct at_channel *atc = arg;
	for (;;) {
		const char * line;
		line = readline(atc);
		if (line == NULL) {
			dprintf("%s exit\n", __func__);
			break;
		}
		processLine(atc, line);
	}
	return NULL;
}

int atc_register_unsolicited(struct at_channel *atc, char *prefix_unso, at_response_cb cb)
{
	struct at_unso_node *unode;
	work_item_t *work_item;
	atc = atc_try_init(atc);
	if (atc == NULL)
		return -1;
	unode = (struct at_unso_node *)mmalloc(sizeof (struct at_unso_node));
	unode->prefix = strdup(prefix_unso);
	unode->cb = cb;
	pthread_mutex_lock(&atc->queue_mutex);
	work_item = work_queue_add(atc->queue, unode);
	pthread_mutex_unlock(&atc->queue_mutex);
	return 0;
}

int atc_send(struct at_channel *atc, char *at, size_t len, int sync, int timeout_ms/* millisecond */)
{
	int ret;
	atc = atc_try_init(atc);
	if (atc == NULL)
		return -1;
	/* we can queue all AT in future*/
	if (sync)
		pthread_mutex_lock(&atc->s_command_mutex);

	pthread_mutex_lock(&atc->write_mutex);
	ret = writeline(atc, at, len);
	pthread_mutex_unlock(&atc->write_mutex);

	if (ret < 0) {
		if (sync)
			pthread_mutex_unlock(&atc->s_command_mutex);
		return ret;
	}

	if (sync) {
		/**
		 * wait until this sync AT returns result
		 * 1. "+CME ERROR"
		 * 2. "OK"
		 */
		int err = 0;
		struct timespec ts;
#ifndef USE_NP
		if (timeout_ms != 0) {
			setTimespecRelative(&ts, timeout_ms);
		}
#endif /*USE_NP*/
		pthread_mutex_lock(&atc->response_cond_mutex);
		if (!atc->atrep.finalResponse) {
			if (timeout_ms != 0) {
#ifdef USE_NP
				err = pthread_cond_timeout_np(&atc->response_cond, &atc->response_cond_mutex, timeout_ms);
#else
				err = pthread_cond_timedwait(&atc->response_cond, &atc->response_cond_mutex, &ts);
#endif /*USE_NP*/
			} else {
				err = pthread_cond_wait(&atc->response_cond, &atc->response_cond_mutex);
			}
		}
		if (err == ETIMEDOUT) {
			dprintf("%s timeout\n", __func__);
			ret = -2;
		} else {
			if (atc->atrep.finalResponse) {
				dprintf("Get response [ %s ]\n", atc->atrep.finalResponse);
				free(atc->atrep.finalResponse);
				atc->atrep.finalResponse = NULL;
			}
			ret = atc->atrep.success ? 0 : -3;
		}
		pthread_mutex_unlock(&atc->response_cond_mutex);

		pthread_mutex_unlock(&atc->s_command_mutex);
	}
	return ret;
}

// mkfifo为atc[1]建立测试通道
// O_RDWR | O_NONBLOCK
struct at_channel *atc_init(const char *dev, int mode, int fd)
{
	struct at_channel *atc = NULL;
	unsigned int i;
	char *name = (char*)dev;
	pthread_t tid;

	for (i = 0; i < NUM_ELEMS(atcs); i++) {
		if (atcs[i].fd <= 0) {
			atc = &atcs[i];
			break;
		}
	}

	if (!atc) {
		dprintf("open %s: we don't have free channel to use\n", name);
		return NULL;
	}

	if (!name) name = ATC_DEVICE_DEFAULT;

	dprintf("open %s --> atc%d\n", name, i);
	if (fd < 0)
		atc->fd = open(name, mode);
	else
		atc->fd = fd;
	if (atc->fd < 0) {
		dprintf("open %s: failed [%d]\n", name, errno);
		return NULL;
	}
	atc->name = name;
	atc->s_ATBufferCur = atc->s_ATBuffer;
	pthread_mutex_init(&atc->queue_mutex, NULL);
	pthread_mutex_init(&atc->write_mutex, NULL);
	pthread_mutex_init(&atc->s_command_mutex, NULL);
	pthread_mutex_init(&atc->response_cond_mutex, NULL);
	pthread_cond_init(&atc->response_cond, NULL);
	atc->queue = work_queue_new();

	if (isatty(atc->fd)) {
		struct termios ser_settings;
		dprintf("%s is tty device, force it to be raw data mode\n", name);
		tcgetattr(atc->fd, &ser_settings);
		cfmakeraw(&ser_settings);
		tcsetattr(atc->fd, TCSANOW, &ser_settings);
	}
	if (pthread_create(&tid, NULL, readerLoop, atc) < 0) {
		dprintf("atc create [%s] readerLoop thread failed\n", atc->name);
	}
#ifdef DEBUG_SIPC
	if (atc->dfd <= 0)
		atc->dfd = open("/d/sipc/sbuf", O_RDONLY);
#endif

	return atc;
}
