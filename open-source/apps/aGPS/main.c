/*
 *  Test atd module
 *
 *  Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <unistd.h>
#include <sys/stat.h>
#include <atd.h>
#define atd_test_path "/dev/atd"
int my_at_response_cb(const char *at, int len)
{
	dprintf("Unsolicited callback get AT [ %s ]\n", at);
	return 0;
}

/**
 * you can use following method to have a interactive test
 * 1. logcat -s ATDistributer
 * 2. gcom.arm -t /dev/atd
 */
int main(int argc, char *argv[])
{
	int fdm = -1;
#if 1
	char *slave_name;
	fdm = open("/dev/ptmx", O_RDWR);
	tcflush(fdm, TCIFLUSH);
	grantpt(fdm);
	unlockpt(fdm);
	fcntl(fdm, F_SETFD, FD_CLOEXEC);
	slave_name = ptsname(fdm);
	unlink(atd_test_path);
	symlink(slave_name, atd_test_path);
#else
	unlink(atd_test_path);
	mkfifo(atd_test_path, 0666);
#endif
	dprintf("create %s\n", atd_test_path);
	atc_init(atd_test_path, O_RDWR, fdm);
	register_unsolicited("+SPAPGSDL:", my_at_response_cb);
	register_unsolicited("+SPAGPSEND", my_at_response_cb);
	register_unsolicited("+SPAGPSSTATE:", my_at_response_cb);
	register_unsolicited("+SPAGPSNILR", my_at_response_cb);
	register_unsolicited("+SPAGPSMTLR:", my_at_response_cb);
	register_unsolicited("+SPAGPSRESET:", my_at_response_cb);
	at_send_async("AT+LUTH=req,3,0", 15);
	while (1) {
		at_send("AT+SPLN=1", 9, 10*1000);
		at_send("AT+SPAGPSUL=<protocol_type>,<rrc_type>,<pdu_len>,<pdu_data>", 59, 0);
	}
	unlink(atd_test_path);
	return 0;
}
