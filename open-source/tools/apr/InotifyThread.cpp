
#include "common.h"
#include "InotifyThread.h"
#include "AprData.h"

#define EVENT_NUM 16
#define MAX_BUF_SIZE 1024

static char* events[] =
{
	(char*)"File was accessed",
	(char*)"File was modified",
	(char*)"File attributes were changed",
	(char*)"writeable file closed",
	(char*)"Unwriteable file closed",
	(char*)"File was opened",
	(char*)"File was moved from X",
	(char*)"File was moved to Y",
	(char*)"Subfile was created",
	(char*)"Subfile was deleted",
	(char*)"Self was deleted",
	(char*)"Self was moved",
	(char*)"",
	(char*)"Backing fs was unmounted",
	(char*)"Event queued overflowed",
	(char*)"File was ignored"
};

InotifyThread::InotifyThread(CrashBehavior* cb)
{
	m_ifd = -1;
	m_cb = cb;
}

InotifyThread::~InotifyThread()
{
	m_cb = NULL;
}

void InotifyThread::Setup()
{
	m_cb->init_wd_context(&m_wds);
}

void InotifyThread::Execute(void* arg)
{
	APR_LOGD("InotifyThread::Execute()\n");
	char buffer[MAX_BUF_SIZE];
	struct inotify_event* event;
	int len;
	int index;

	_inotify_init();

	while (1)
	{
		fd_set read_fds;
		int rc = 0;
		int max = -1;

		FD_ZERO(&read_fds);

		if (m_ifd) {
			max = m_ifd;
			FD_SET(m_ifd, &read_fds);
		}

		FD_SET(CtrlPipe[0], &read_fds);
		if (CtrlPipe[0] > max)
			max = CtrlPipe[0];

		if ((rc = select(max + 1, &read_fds, NULL, NULL, NULL)) < 0) {
			if (errno == EINTR)
				continue;
			APR_LOGE("select failed (%s) m_ifd=%d, max=%d\n", strerror(errno), m_ifd, max);
			sleep(1);
			continue;
		} else if (!rc)
			continue;

		if (FD_ISSET(CtrlPipe[0], &read_fds)) {
			char c = CtrlPipe_Shutdown;
			TEMP_FAILURE_RETRY(read(CtrlPipe[0], &c, 1));
			if (c == CtrlPipe_Shutdown) {
				break;
			}
			continue;
		}

		if (FD_ISSET(m_ifd, &read_fds)) {
			memset(buffer, 0, sizeof(buffer));
			len = read(m_ifd, buffer, MAX_BUF_SIZE);
			if (len < 0) {
				APR_LOGE("read failed (%s)\n", strerror(errno));
				continue;
			}
			APR_LOGD("Some event happens, len = %d.\n", len);
			index = 0;
			while (index < len)
			{
				event = (struct inotify_event *)(buffer+index);

				/*
				 *
				 *
				 */
				m_cb->handle_event(m_ifd, &m_wds, event);

				index += sizeof(struct inotify_event) + event->len;
			}
		}
	}

	_inotify_dele();
}

int InotifyThread::_inotify_init()
{
	uint32_t i;
	int wd;
	m_ifd = inotify_init();
	if (m_ifd < 0) {
		APR_LOGD("anr_inotify_init(): Fail to initialize inotify.\n");
		exit(-1);
	}

	for (i=0; i<m_wds.size(); i++) {
		wd = inotify_add_watch(m_ifd, m_wds[i].pathname.c_str(), m_wds[i].mask);
		if (wd < 0) {
			APR_LOGE("inotify_add_watch(): Can't add watch for %s; errno=%d, ENOENT=%d\n",
				m_wds[i].pathname.c_str(), errno, ENOENT);
			if (errno == ENOENT)
				continue;
			exit(-1);
		}
		m_wds[i].wd = wd;
	}

	return 0;
}

int InotifyThread::_inotify_dele()
{
	/* All watches are destroyed and cleaned up on close. */
	close(m_ifd);
	m_ifd = -1;
	/* clear vector for m_cb->init_wd_context(&m_wds) */
	m_wds.clear();

	return 0;
}
