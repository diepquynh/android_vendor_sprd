/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
  **/

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "libbt-bqb"
#else
#define LOG_TAG "libbt-bqb"
#endif

#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <errno.h>
#include "buffer_allocator.h"
#include "bt_vendor_lib.h"
#include "bt_hci_bdroid.h"
#include "vendor.h"
#include "bqb.h"

#define HCI_PSKEY   0xFCA0
#define HCI_VSC_ENABLE_COMMMAND 0xFCA1
#define UNUSED(expr) do { (void)(expr); } while (0)

#define UNUSED(expr) do { (void)(expr); } while (0)

#define RDWR_FD_FAIL (-2)
#define GENERIC_FAIL (-1)

#define PSKEY_CMD 0xFCA0
#define VENDOR_CMD_NONSIGNAL_START 0xFCD1
#define VENDOR_CMD_NONSIGNAL_STOP 0xFCD2

#define RESPONSE_LENGTH 100


static const char *VENDOR_LIBRARY_NAME = "libbt-vendor.so";
static const char *VENDOR_LIBRARY_SYMBOL_NAME = "BLUETOOTH_VENDOR_LIB_INTERFACE";

static void *lib_handle;
static bt_vendor_interface_t *lib_interface;

static int s_bt_fd = -1;

static uint8_t local_bdaddr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint32_t idle_timeout_ms;

static bt_bqb_mode_t bt_bqb_mode;

/* dump received event or sent command */
static void dump_data(uint8_t *p_buf, int data_len) {
    int i = 0;
    int buf_printf_len = data_len * 3 + 1;
    char *p_buf_pskey_printf = malloc(buf_printf_len);
    memset(p_buf_pskey_printf, 0, buf_printf_len);
    char *p = p_buf_pskey_printf;
    ALOGI("start to dump_data");
    for (i = 0; i < data_len; i++)
        p += snprintf(p, buf_printf_len, "%02x ", p_buf[i]);
    ALOGI("%s", p_buf_pskey_printf);
    free(p_buf_pskey_printf);
    p_buf_pskey_printf = NULL;
}

/* send hci command */
static int send_cmd(uint8_t *p_buf, int data_len) {
    int i;
    ssize_t ret = 0;

    dump_data(p_buf, data_len);

    /* Send command via HC's xmit_cb API */
    ALOGD("wirte command size=%d", data_len);
    uint16_t total = 0;
    while (data_len) {
        ret = write(s_bt_fd, p_buf + total, data_len);
        ALOGD("wrote data_len= %d, ret size=%d", data_len, ret);
        switch (ret) {
            case -1:
                ALOGE("%s error writing to serial port: %s", __func__, strerror(errno));
                return total;
            case 0:  // don't loop forever in case write returns 0.
                return total;
            default:
                total += ret;
                data_len -= ret;
                break;
        }
    }
    return 0;
}

/* receive event in while loop */
static int recv_event(tINT_CMD_CBACK callback) {
    int buf_printf_len = 0;
    int i = 0;
    int read_data_len = 0;
    uint8_t new_state = BT_VND_LPM_WAKE_ASSERT;
    char *event_resp = malloc(RESPONSE_LENGTH + sizeof(HC_BT_HDR) - 1);
    memset(event_resp, 0, RESPONSE_LENGTH);
    char *p_recv = event_resp + sizeof(HC_BT_HDR) - 1;

    while (1) {
        usleep(100 * 1000);
        lib_interface->op(BT_VND_OP_LPM_WAKE_SET_STATE, &new_state);
        read_data_len += read(s_bt_fd, p_recv + read_data_len, RESPONSE_LENGTH);
        if (read_data_len < 0) {
            ALOGI("Failed to read ack, read_data_len=%d(%s)", read_data_len , strerror(errno));
            free(event_resp);
            return -1;
        } else if (read_data_len < 3) {
            continue;
        }

        dump_data((uint8_t*)p_recv, read_data_len);

        if (p_recv[0] == 0x04 && read_data_len >= (p_recv[2] + 2)) {
            ALOGI("read  ACK(0x04) ok \n");
            break;
        } else if (p_recv[0] == 0x04) {
            continue;
        } else {
            ALOGE("read ACK(0x%x)is not expect,retry\n,", p_recv[0]);
        }
    }
    callback(event_resp);
    return read_data_len;
}

static void firmware_config_cb(bt_vendor_op_result_t result) {
    ALOGD("%s, result=%d", __func__, result);
}

static void sco_config_cb(bt_vendor_op_result_t result) {
  UNUSED(result);
}

static void low_power_mode_cb(bt_vendor_op_result_t result) {
  UNUSED(result);
}

static void sco_audiostate_cb(bt_vendor_op_result_t result) {
  UNUSED(result);
}

static void *buffer_alloc_cb(int size) {
  char *puf = malloc(size);
  memset(puf, 0, size);
  return puf;
}

static void buffer_free_cb(void *buffer) {
  free(buffer);
}

static void transmit_completed_callback(BT_HDR *response, void *context) {
  UNUSED(response);
  UNUSED(context);
}

static uint8_t transmit_cb(UNUSED_ATTR uint16_t opcode, void *buffer, tINT_CMD_CBACK callback) {
  UNUSED(callback);
  HC_BT_HDR *p_buf = (HC_BT_HDR *)buffer;
  int len = p_buf->len+1;
  uint8_t *p = (uint8_t *)buffer;
  if (opcode == HCI_PSKEY || opcode == HCI_VSC_ENABLE_COMMMAND) {
      p  = p  + sizeof(HC_BT_HDR) - 1;
    *p  = 0x01;    // hci command
    send_cmd(p, len);
    free(buffer);
    recv_event(callback);
  }
  return true;
}

static void epilog_cb(bt_vendor_op_result_t result) {
    ALOGD("%s, result=%d", __func__, result);
}

static void a2dp_offload_cb(bt_vendor_op_result_t result, bt_vendor_opcode_t op, uint8_t bta_av_handle) {
  ALOGD("result=%d, op=%d, bta_av_handle=%d", result, op, bta_av_handle);
}

static bt_vendor_callbacks_t lib_callbacks = {
  sizeof(lib_callbacks),
  firmware_config_cb,
  sco_config_cb,
  low_power_mode_cb,
  sco_audiostate_cb,
  buffer_alloc_cb,
  buffer_free_cb,
  transmit_cb,
  epilog_cb,
  a2dp_offload_cb
};



// Interface functions

static bool vendor_open(const uint8_t *local_bdaddr) {
  assert(lib_handle == NULL);
  bt_vendor_callbacks_t * lib_cb = &lib_callbacks;
  if (lib_cb == NULL) {
    ALOGD("lib_cb == NULL");
  }

  lib_handle = dlopen(VENDOR_LIBRARY_NAME, RTLD_NOW);
  if (!lib_handle) {
    ALOGD("%s unable to open %s: %s", __func__, VENDOR_LIBRARY_NAME, dlerror());
    goto error;
  }

  lib_interface = (bt_vendor_interface_t *)dlsym(lib_handle, VENDOR_LIBRARY_SYMBOL_NAME);
  if (!lib_interface) {
    ALOGD("%s unable to find symbol %s in %s: %s", __func__, VENDOR_LIBRARY_SYMBOL_NAME, VENDOR_LIBRARY_NAME, dlerror());
    goto error;
  }

  ALOGD("alloc value %p", lib_callbacks.alloc);

  int status = lib_interface->init(lib_cb, (unsigned char *)local_bdaddr);
  if (status) {
    ALOGD("%s unable to initialize vendor library: %d", __func__, status);
    goto error;
  }

  return true;

error:
  lib_interface = NULL;
  if (lib_handle)
    dlclose(lib_handle);
  lib_handle = NULL;
  return false;
}

static void vendor_close(void) {
  if (lib_interface)
    lib_interface->cleanup();

  if (lib_handle)
    dlclose(lib_handle);

  lib_interface = NULL;
  lib_handle = NULL;
}


static void lmp_enable(void) {
    uint8_t command = BT_VND_LPM_ENABLE;
    lib_interface->op(BT_VND_OP_GET_LPM_IDLE_TIMEOUT, &command);
    lib_interface->op(BT_VND_OP_GET_LPM_IDLE_TIMEOUT, &idle_timeout_ms);
}

static void lmp_disable(void) {
    uint8_t command = BT_VND_LPM_DISABLE;
    lib_interface->op(BT_VND_OP_GET_LPM_IDLE_TIMEOUT, &command);
}

void lmp_assert(void) {
    uint8_t command = BT_VND_LPM_WAKE_ASSERT;
    lib_interface->op(BT_VND_OP_LPM_WAKE_SET_STATE, &command);
}

void lmp_deassert(void) {
    uint8_t command = BT_VND_LPM_WAKE_DEASSERT;
    lib_interface->op(BT_VND_OP_LPM_WAKE_SET_STATE, &command);
}


/* bringup bt up */
int bt_on(bt_bqb_mode_t mode) {
    int fd_array[CH_MAX];
    ALOGD("%s", __func__);
    int power_state = BT_VND_PWR_OFF;
    vendor_open(local_bdaddr);
    lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
    power_state = BT_VND_PWR_ON;
    lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
    lib_interface->op(BT_VND_OP_USERIAL_OPEN, &fd_array);
    s_bt_fd = fd_array[0];
    ALOGD("%s completed fd_array[0]=%d", __func__, fd_array[0]);

    lmp_enable();

    bt_bqb_mode = mode;
    if (mode == BQB_NOPSKEY) {
        goto out;
    }

    lib_interface->op(BT_VND_OP_FW_CFG, NULL);

out:
    set_bqb_state(BQB_OPENED);
    ALOGD("%s completed", __func__);
    return s_bt_fd;
}

/* close bt */
void bt_off(void) {
    ALOGD("%s", __func__);
    int power_state = BT_VND_PWR_OFF;

    if (bt_bqb_mode == BQB_NOPSKEY) {
        goto out;
    }
    lmp_assert();
    lib_interface->op(BT_VND_OP_EPILOG, NULL);

out:
    lib_interface->op(BT_VND_OP_USERIAL_CLOSE, NULL);

    lmp_disable();

    lib_interface->op(BT_VND_OP_POWER_CTRL, &power_state);
    lib_interface->cleanup();
    s_bt_fd = -1;
    vendor_close();
    set_bqb_state(BQB_CLOSED);
    ALOGD("%s completed", __func__);
}
