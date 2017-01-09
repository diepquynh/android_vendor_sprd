/******************************************************************************
 *
 *  Load libbt-vendor.so for spreadtrum marlin bt aging test
 *
 ******************************************************************************/

#define LOG_TAG "bt_vendor"

#include <string.h>
#include <assert.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include "buffer_allocator.h"
#include "bt_vendor_lib.h"
#include "bt_hci_bdroid.h"
#include "vendor.h"

#define HCI_PSKEY   0xFCA0
#define HCI_VSC_ENABLE_COMMMAND 0xFCA1
#define UNUSED(expr) do { (void)(expr); } while (0)

extern int send_cmd(uint8_t *p_buf, int data_len);
extern int recv_event(tINT_CMD_CBACK callback);

static const char *VENDOR_LIBRARY_NAME = "libbt-vendor.so";
static const char *VENDOR_LIBRARY_SYMBOL_NAME = "BLUETOOTH_VENDOR_LIB_INTERFACE";

void *lib_handle;
bt_vendor_interface_t *lib_interface;


void firmware_config_cb(bt_vendor_op_result_t result) {
    ALOGD("sensen, %s, result=%d", __func__, result);
}

void sco_config_cb(bt_vendor_op_result_t result) {
  UNUSED(result);
}

void low_power_mode_cb(bt_vendor_op_result_t result) {
  UNUSED(result);
}

void sco_audiostate_cb(bt_vendor_op_result_t result) {
  UNUSED(result);
}

void *buffer_alloc_cb(int size) {
  char *puf = malloc(size);
  memset(puf, 0, size);
  return puf;
}

void buffer_free_cb(void *buffer) {
  free(buffer);
}

void transmit_completed_callback(BT_HDR *response, void *context) {
  UNUSED(response);
  UNUSED(context);
}

uint8_t transmit_cb(UNUSED_ATTR uint16_t opcode, void *buffer, tINT_CMD_CBACK callback) {
  UNUSED(callback);
  HC_BT_HDR *p_buf = (HC_BT_HDR *)buffer;
  int len = p_buf->len+1;
  uint8_t *p = (uint8_t *)buffer;
  if(opcode == HCI_PSKEY || opcode == HCI_VSC_ENABLE_COMMMAND) {
      p  = p  + sizeof(HC_BT_HDR) - 1;
    *p  = 0x01;    //hci command
    send_cmd(p, len);
    free(buffer);
    recv_event(callback);
  }
  return true;
}

void epilog_cb(bt_vendor_op_result_t result) {
    ALOGD("sensen, %s, result=%d", __func__, result);
}

bt_vendor_callbacks_t lib_callbacks = {
  sizeof(lib_callbacks),
  firmware_config_cb,
  sco_config_cb,
  low_power_mode_cb,
  sco_audiostate_cb,
  buffer_alloc_cb,
  buffer_free_cb,
  transmit_cb,
  epilog_cb
};



// Interface functions

bool vendor_open(const uint8_t *local_bdaddr) {
  assert(lib_handle == NULL);
  bt_vendor_callbacks_t * lib_cb = &lib_callbacks;
  if(lib_cb == NULL) {
    ALOGD("sensen lib_cb == NULL");
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

error:;
  lib_interface = NULL;
  if (lib_handle)
    dlclose(lib_handle);
  lib_handle = NULL;
  return false;
}

void vendor_close(void) {
  if (lib_interface)
    lib_interface->cleanup();

  if (lib_handle)
    dlclose(lib_handle);

  lib_interface = NULL;
  lib_handle = NULL;
}
