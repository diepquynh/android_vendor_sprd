/******************************************************************************
 *
 *  Copyright (C) 2016 Spreadtrum Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#define LOG_TAG "bt_sitm"

#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/prctl.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_sprd.h"
#include "sitm.h"
#ifdef USERIAL_SCO_PORT
#include <termios.h>
#endif

#ifndef SITM_DBG
#define SITM_DBG TRUE
#endif

#if (SITM_DBG == TRUE)
#define SITMD(param, ...) ALOGD("%s "param, __FUNCTION__, ## __VA_ARGS__)
#else
#define SITMD(param, ...) {}
#endif
#define SITME(param, ...) ALOGE("%s "param, __FUNCTION__, ## __VA_ARGS__)

static pthread_t read_th;
static pthread_t write_th;


static int userial_fd = INVALID_FD;
static int stack_fd = INVALID_FD;
static int vendor_fd = INVALID_FD;
#ifdef USERIAL_SCO_PORT
static int sco_fd = INVALID_FD;
#endif

static int tx_fds[2] = { INVALID_FD, INVALID_FD };
static int rx_fds[2] = { INVALID_FD, INVALID_FD };

static packet_receive_data_t *recv_stack_buf;
static packet_receive_data_t *recv_serial_buf;
static const uint8_t preamble_sizes[] = {
  HCI_COMMAND_PREAMBLE_SIZE,
  HCI_ACL_PREAMBLE_SIZE,
  HCI_SCO_PREAMBLE_SIZE,
  HCI_EVENT_PREAMBLE_SIZE
};

static void dump_hex(uint8_t *src, int len);
static void parse_frame(packet_receive_data_t *receive_data, data_ready_cb data_ready, frame_complete_cb frame_complete);
static int stack_data_ready(uint8_t *data, uint32_t len);
static int serial_data_transmit(uint8_t *data, uint32_t len);
static int serial_data_ready(uint8_t *data, uint32_t len);
static int stack_data_transmit(uint8_t *data, uint32_t len);

#ifdef USERIAL_SCO_PORT
static int open_sco_userial(char *name) {
    struct termios termios;
    int fd = open(name, O_RDWR);
    if (fd < 0) {
        ALOGE("open %s failed: %s", name, strerror(errno));
        return fd;
    }
    tcflush(sco_fd, TCIOFLUSH);

    tcgetattr(sco_fd, &termios);
    cfmakeraw(&termios);
    termios.c_cflag |= (CRTSCTS);
    tcsetattr(sco_fd, TCSANOW, &termios);
    tcflush(sco_fd, TCIOFLUSH);

    tcsetattr(sco_fd, TCSANOW, &termios);
    tcflush(sco_fd, TCIOFLUSH);
    tcflush(sco_fd, TCIOFLUSH);

    /* set input/output baudrate */
    cfsetospeed(&termios, B3000000);
    cfsetispeed(&termios, B3000000);
    tcsetattr(sco_fd, TCSANOW, &termios);
    return fd;
}
#endif

static void sitm_read_th(void *arg)
{
    int ret, max_fd;
    fd_set fds;
    struct sigaction actions;
    SITMD("+++");
    prctl(PR_SET_NAME, (unsigned long)"sitm_read_th", 0, 0, 0);
    UNUSED(arg);
    do {
        FD_ZERO(&fds);
        FD_SET(userial_fd, &fds);
        FD_SET(rx_fds[0], &fds);
        max_fd = userial_fd > rx_fds[0] ? userial_fd : rx_fds[0];
        ret = select(max_fd + 1, &fds, NULL, NULL, NULL);
        if(ret <= 0) {
            SITME("select userial faield");
            break;
        }
        if (FD_ISSET(rx_fds[0], &fds)) {
            if (rx_fds[0] != INVALID_FD) {
                SITMD("close rx_fds 0");
                close(rx_fds[0]);
                rx_fds[0] = INVALID_FD;
            }
            break;
        }
        //ALOGD("userial frame");
        parse_frame(recv_serial_buf, serial_data_ready, stack_data_transmit);
    } while (1);
    SITMD("---");
}

static void sitm_write_th(void *arg)
{
    int ret, max_fd;
    fd_set fds;
    struct sigaction actions;
    UNUSED(arg);
    SITMD("+++");
    prctl(PR_SET_NAME, (unsigned long)"sitm_write_th", 0, 0, 0);
    do {
        FD_ZERO(&fds);
        FD_SET(vendor_fd, &fds);
        FD_SET(tx_fds[0], &fds);
        max_fd = vendor_fd > tx_fds[0] ? vendor_fd : tx_fds[0];
        ret = select(max_fd + 1, &fds, NULL, NULL, NULL);
        if(ret <= 0) {
            SITME("select userial faield");
            break;
        }
        if (FD_ISSET(tx_fds[0], &fds)) {
            if (tx_fds[0] != INVALID_FD) {
                SITMD("close tx_fds 0");
                close(tx_fds[0]);
                tx_fds[0] = INVALID_FD;
            }
            break;
        }
        //ALOGD("stack frame");
        parse_frame(recv_stack_buf, stack_data_ready, serial_data_transmit);
    } while (1);
    SITMD("---");
}




int sitm_server_start_up(int fd)
{
    int fds[2] = { INVALID_FD, INVALID_FD };
    SITMD("+++");
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, fds) == -1) {
        SITME("%s error creating socketpair: %s", __func__, strerror(errno));
        goto error;
    }

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, tx_fds) == -1) {
        SITME("%s error creating tx_fds: %s", __func__, strerror(errno));
        goto error;
    }

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, rx_fds) == -1) {
        SITME("%s error creating rx_fds: %s", __func__, strerror(errno));
        goto error;
    }


    userial_fd = fd;
    stack_fd = fds[0];
    vendor_fd = fds[1];
    if (pthread_create(&read_th, NULL, (void*)sitm_read_th, NULL) < 0) {
        SITME("create sthread_react failed");
        goto error;
    }
    if (pthread_create(&write_th, NULL, (void*)sitm_write_th, NULL) < 0) {
        SITME("create sthread_react failed");
        goto error;
    }

    recv_serial_buf = (packet_receive_data_t*)malloc(sizeof(packet_receive_data_t));
    memset(recv_serial_buf, 0, sizeof(packet_receive_data_t));
    if (recv_serial_buf == NULL) {
        SITME("malloc recv_serial_buf failed");
        goto error;
    }
    recv_stack_buf = (packet_receive_data_t*)malloc(sizeof(packet_receive_data_t));
    memset(recv_stack_buf, 0, sizeof(packet_receive_data_t));
    if (recv_stack_buf == NULL) {
        SITME("malloc recv_stack_buf failed");
        goto error;
    }

    SITMD("---");

    return stack_fd;
error:
    if (stack_fd != INVALID_FD) {
        close(stack_fd);
        stack_fd = INVALID_FD;
    }
    if (vendor_fd != INVALID_FD) {
        close(vendor_fd);
        vendor_fd = INVALID_FD;
    }
    if (recv_serial_buf != NULL) {
        free(recv_serial_buf);
        recv_serial_buf = NULL;
    }
    if (recv_stack_buf != NULL) {
        free(recv_stack_buf);
        recv_stack_buf = NULL;
    }
    return -1;
}

int sitm_server_shut_down()
{
    SITMD("+++");

    write(rx_fds[1], "0", 1);
    write(tx_fds[1], "0", 1);
    SITMD("pthread_join read_th");
    pthread_join(read_th, NULL);
    SITMD("pthread_join write_th");
    pthread_join(write_th, NULL);
    SITMD("pthread_join done");

    if (rx_fds[1] != INVALID_FD) {
        SITMD("close rx_fds 1");
        close(rx_fds[1]);
        rx_fds[1] = INVALID_FD;
    }

    if (tx_fds[1] != INVALID_FD) {
        SITMD("close tx_fds 1");
        close(tx_fds[1]);
        tx_fds[1] = INVALID_FD;
    }

    if (stack_fd != INVALID_FD) {
        SITMD("close stack_fd");
        close(stack_fd);
        stack_fd = INVALID_FD;
    }
    if (vendor_fd != INVALID_FD) {
        SITMD("close vendor_fd");
        close(vendor_fd);
        vendor_fd = INVALID_FD;
    }
    if (recv_serial_buf != NULL) {
        SITMD("free recv_serial_buf");
        free(recv_serial_buf);
        recv_serial_buf = NULL;
    }
    if (recv_stack_buf != NULL) {
        SITMD("free recv_stack_buf");
        free(recv_stack_buf);
        recv_stack_buf = NULL;
    }

#ifdef USERIAL_SCO_PORT
    if (sco_fd != INVALID_FD) {
        SITMD("close sco_fd");
        close(sco_fd);
        sco_fd = INVALID_FD;
    }
#endif

    SITMD("---");
    return 0;
}

static int stack_data_ready(uint8_t *data, uint32_t len) {
    int ret = read(vendor_fd, data, len);
    return ret;
}

static int serial_data_transmit(uint8_t *data, uint32_t len) {
#ifdef USERIAL_SCO_PORT
    if (*data == DATA_TYPE_SCO) {
        if (sco_fd < 0) {
            sco_fd = open_sco_userial(USERIAL_SCO_PORT);
        }
        if (sco_fd > 0) {
            return write(sco_fd, data, len);
        } else {
            return -1;
        }
    }
#endif

    uint32_t alignment = BYTE_ALIGNMENT;
    if (alignment > 1) {
        uint32_t tail = alignment - ((len + alignment) % alignment);
        if (tail) {
            uint8_t *p = data + len;
            len += tail;
            while (tail-- && p != NULL) 
                *p++ = 0;
        }
    }
    return write(userial_fd, data, len);
}

static int serial_data_ready(uint8_t *data, uint32_t len) {
    int ret = read(userial_fd, data, len);
    if (len > 7)  {
        ALOGD("R[%u]: %02X%02X%02X%02x %02X%02X%02X%02X", len,
            data[0], data[1], data[2], data[3], data[len -4], data[len -3], data[len -2], data[len -1]);
    } else {
        static uint32_t i;
        static char buf[20];
        for (i = 0; i < len; i++) {
                snprintf(buf + 2 * i, sizeof(buf), "%02X", data[i]);
        }
        ALOGD("R[%u]: %s", len, buf);
    }
    return ret;
}

static int stack_data_transmit(uint8_t *data, uint32_t len) {
    return write(vendor_fd, data, len);
}


static void parse_frame(packet_receive_data_t *receive_data, data_ready_cb data_ready, frame_complete_cb frame_complete) {
    uint8_t byte;
    while (data_ready(&byte, 1) == 1) {
        switch (receive_data->state) {
            case BRAND_NEW:
                if (byte > DATA_TYPE_EVENT || byte < DATA_TYPE_COMMAND) {
                    SITME("unknow head: 0x%02x", byte);
                    break;
                }
                receive_data->type = byte;
                // Initialize and prepare to jump to the preamble reading state
                receive_data->bytes_remaining = preamble_sizes[PACKET_TYPE_TO_INDEX(receive_data->type)] + 1;
                memset(receive_data->preamble, 0, PREAMBLE_BUFFER_SIZE);
                receive_data->index = 0;
                receive_data->state = PREAMBLE;
            case PREAMBLE:
                receive_data->preamble[receive_data->index] = byte;
                receive_data->index++;
                receive_data->bytes_remaining--;

                if (receive_data->bytes_remaining == 0) {
                    // For event and sco preambles, the last byte we read is the length
                    receive_data->bytes_remaining = (receive_data->type == DATA_TYPE_ACL) ? RETRIEVE_ACL_LENGTH(receive_data->preamble) : byte;
                    size_t buffer_size = receive_data->index + receive_data->bytes_remaining;

                    if (!receive_data->buffer) {
                        SITME("%s error getting buffer for incoming packet of type %d and size %zd", __func__, receive_data->type, buffer_size);
                        // Can't read any more of this current packet, so jump out
                        receive_data->state = receive_data->bytes_remaining == 0 ? BRAND_NEW : IGNORE;
                        break;
                    }

                    // Initialize the buffer
                    memcpy(receive_data->buffer, receive_data->preamble, receive_data->index);

                    receive_data->state = receive_data->bytes_remaining > 0 ? BODY : FINISHED;
                }

                break;
            case BODY:
                receive_data->buffer[receive_data->index] = byte;
                receive_data->index++;
                receive_data->bytes_remaining--;
                size_t bytes_read = data_ready((receive_data->buffer + receive_data->index), receive_data->bytes_remaining);
                receive_data->index += bytes_read;
                receive_data->bytes_remaining -= bytes_read;

                receive_data->state = receive_data->bytes_remaining == 0 ? FINISHED : receive_data->state;
                break;
            case IGNORE:
                ALOGD("PARSE IGNORE");
                receive_data->bytes_remaining--;
                if (receive_data->bytes_remaining == 0) {
                receive_data->state = BRAND_NEW;
                // Don't forget to let the hal know we finished the packet we were ignoring.
                // Otherwise we'll get out of sync with hals that embed extra information
                // in the uart stream (like H4). #badnewsbears
                //hal->packet_finished(type);
                return;
                }

                break;
            case FINISHED:
                SITME("%s the state machine should not have been left in the finished state.", __func__);
            break;
            default:
                ALOGD("PARSE DEFAULT");
            break;
        }

    if (receive_data->state == FINISHED) {
        receive_data->index;
        frame_complete(receive_data->buffer, receive_data->index);

        // We don't control the buffer anymore
        receive_data->state = BRAND_NEW;
        return;
    }
  }
}
