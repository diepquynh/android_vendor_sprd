
#include "common.h"
#include "ModemThread.h"
#include "AprData.h"

#define MODEM_SOCKET_NAME         "modemd"
#define WCND_SOCKET_NAME          "wcnd"

ModemThread::ModemThread(AprData* aprData)
{
	m_aprData = aprData;
}

ModemThread::~ModemThread()
{
	m_aprData = NULL;
}

void ModemThread::Setup()
{
	APR_LOGD("ModemThread::Setup()\n");
}

void ModemThread::Execute(void* arg)
{
	APR_LOGD("ModemThread::Execute()\n");
	int modemfd = -1;
	int wcnfd = -1;
	char buf[256];
	int numRead;

reconnect:
	if (modemfd < 0)
		modemfd = ConnectService(MODEM_SOCKET_NAME);
	if (wcnfd < 0)
		wcnfd = ConnectService(WCND_SOCKET_NAME);

	if (modemfd < 0 && wcnfd < 0) {
		APR_LOGE("%s: cannot create local socket server", __FUNCTION__);
		exit(-1);
	}

	for(;;) {
		fd_set read_fds;
		int rc = 0;
		int max = -1;

		FD_ZERO(&read_fds);

		max = CtrlPipe[0];
		FD_SET(CtrlPipe[0], &read_fds);
		if (modemfd >= 0) {
			FD_SET(modemfd, &read_fds);
			if (modemfd > max)
				max = modemfd;
		}
		if (wcnfd >= 0) {
			FD_SET(wcnfd, &read_fds);
			if (wcnfd > max)
				max = wcnfd;
		}

		if ((rc = select(max + 1, &read_fds, NULL, NULL, NULL)) < 0) {
			if (errno == EINTR)
				continue;
			APR_LOGE("select failed (%s) modemfd=%d, wcnfd=%d, max=%d\n",
				strerror(errno), modemfd, wcnfd, max);
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
		}

		if (FD_ISSET(modemfd, &read_fds)) {
			memset(buf, 0, sizeof(buf));
			do {
				numRead = read(modemfd, buf, sizeof(buf));
			} while(numRead < 0 && errno == EINTR);

			if (numRead <= 0) {
				close(modemfd);
				modemfd = -1;
				goto reconnect;
			}
			buf[255] = '\0';

			APR_LOGD("receive \"%s\"\n", buf);
			m_ei.private_data = (void*)buf;
			if (strstr(buf, "Modem Blocked") != NULL) {
				m_ei.et = E_MODEM_BLOCKED;
				m_aprData->setChanged();
				m_aprData->notifyObservers((void*)&m_ei);
			} else if (strstr(buf, "Assert") != NULL) {
				m_ei.et = E_MODEM_ASSERT;
				m_aprData->setChanged();
				m_aprData->notifyObservers((void*)&m_ei);
			}
		}

		if (FD_ISSET(wcnfd, &read_fds)) {
			memset(buf, 0, sizeof(buf));
			do {
				numRead = read(wcnfd, buf, sizeof(buf));
			} while(numRead < 0 && errno == EINTR);

			if (numRead <= 0) {
				close(wcnfd);
				wcnfd = -1;
				goto reconnect;
			}
			buf[255] = '\0';

			APR_LOGD("receive \"%s\"\n", buf);
			m_ei.private_data = (void*)buf;
			if (strstr(buf, "WCN-CP2-EXCEPTION") != NULL) {
				m_ei.et = E_WCN_ASSERT;
				m_aprData->setChanged();
				m_aprData->notifyObservers((void*)&m_ei);
			}
		}
	}

	close(modemfd);
	close(wcnfd);
}

int ModemThread::ConnectService(const char *socket_name)
{
	int cfd, iter;
	for (iter = 0; iter < 30; iter++) {
		cfd = socket_local_client(socket_name,
				ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
		APR_LOGD("socket_name=%s, cfd=%d\n", socket_name, cfd);
		if (cfd >= 0)
			break;
		sleep(1);
	}

	return cfd;
}
