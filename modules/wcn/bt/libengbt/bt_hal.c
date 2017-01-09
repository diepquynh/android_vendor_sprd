/**
 * bt_cmd_excuter.c --- libengbt implementation
 *
 * Copyright (C) 2015 Spreadtrum Communications Inc.
 */

#undef LOG_TAG
#define LOG_TAG "bte"


#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <private/android_filesystem_config.h>

#include <hardware/hardware.h>
#include <hardware/bluetooth.h>
#include <dlfcn.h>
#include <utils/Log.h>
#include <semaphore.h>


#include "bt_hal.h"
#if ((defined SPRD_FEATURE_TEST) && (SPRD_FEATURE_TEST == TRUE))

#define BTD(param, ...) ALOGD("HAL %s "param, __FUNCTION__, ## __VA_ARGS__)
#define BTE(param, ...) ALOGE("HAL %s "param, __FUNCTION__, ## __VA_ARGS__)
#define UNUSED(x) (void)(x)


static const char kBluetoothLibraryName[] = "/system/lib/hw/bluetooth.default.so";

const bt_interface_t* bt_iface;
const hw_module_t* module;
const bluetooth_device_t* bt_device;
static hw_device_t* device;
static void *bt_handle;

static bt_status_t status;

static uint8_t bt_enabled = 0;

static sem_t semt_wait;
static pthread_mutex_t mutex_lock;


static const btest_interface_t *sBluetoothTest = NULL;

 static btest_callbacks_t bt_test_callbacks = {
    sizeof(bt_callbacks_t),
    NULL,
    NULL
};


#ifndef CASE_RETURN_STR
#define CASE_RETURN_STR(const) case const: return #const;
#endif


static const char* dump_bt_status(bt_status_t status)
{
    switch(status)
    {
        CASE_RETURN_STR(BT_STATUS_SUCCESS)
        CASE_RETURN_STR(BT_STATUS_FAIL)
        CASE_RETURN_STR(BT_STATUS_NOT_READY)
        CASE_RETURN_STR(BT_STATUS_NOMEM)
        CASE_RETURN_STR(BT_STATUS_BUSY)
        CASE_RETURN_STR(BT_STATUS_UNSUPPORTED)

        default:
            return "unknown status code";
    }
}

static void adapter_state_changed(bt_state_t state)
{
    BTD("%s", (state == BT_STATE_OFF)?"OFF":"ON");
    if (state == BT_STATE_ON) {
        bt_enabled = 1;
    } else {
        bt_enabled = 0;
    }
    sem_post(&semt_wait);
}

bt_callbacks_t bt_callbacks = {
    sizeof(bt_callbacks_t),
    adapter_state_changed,
    NULL,
    NULL,
    NULL, /* device_found_cb */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL, /* dut_mode_recv_cb */
    NULL, /* le_test_mode_cb */
    NULL  /* energy_info_cb */
};

static bool set_wake_alarm(uint64_t delay_millis, bool should_wake, alarm_cb cb, void *data)
{
    UNUSED(delay_millis);
    UNUSED(should_wake);
    UNUSED(cb);
    UNUSED(data);
    return false;
}

static int acquire_wake_lock(const char *lock_name)
{
    UNUSED(lock_name);
    return BT_STATUS_SUCCESS;
}

static int release_wake_lock(const char *lock_name)
{
    UNUSED(lock_name);
    return BT_STATUS_SUCCESS;
}


bt_os_callouts_t bt_os_callouts = {
    sizeof(bt_os_callouts_t),
    set_wake_alarm,
    acquire_wake_lock,
    release_wake_lock,
};


static int bt_lib_load(void)
{
    const char *id = BT_STACK_MODULE_ID;

    bt_handle = dlopen(kBluetoothLibraryName, RTLD_NOW);
    if (!bt_handle) {
        char const *err_str = dlerror();
        BTE("%s", err_str ? err_str : "error unknown");
        goto error;
    }

    const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
    module = (struct hw_module_t *)dlsym(bt_handle, sym);
    if (!module) {
        BTE("%s", sym);
        goto error;
    }

    if (strcmp(id, module->id) != 0) {
        BTE("id=%s does not match HAL module ID: %s", id, module->id);
        goto error;
    }

    BTD("loaded HAL id=%s path=%s hmi=%p handle=%p",
        id, kBluetoothLibraryName, module, bt_handle);

    return 0;

error:
    if (bt_handle)
        dlclose(bt_handle);

    return -EINVAL;
}

static int bt_lib_unload(void)
{
    int err = 0;
    if (bt_handle)
        dlclose(bt_handle);
    return err;
}

static int btif_test_init(void)
{
    sBluetoothTest = (btest_interface_t *) bt_iface->get_profile_interface(BT_PROFILE_TEST_ID);
    if (!sBluetoothTest) {
        BTE("Failed to get Bluetooth Test Interface");
        return -1;
    }
    sBluetoothTest->init(&bt_test_callbacks);
    return 0;
}

static int bt_init(void)
{

    BTD("%s", __func__);
    if (bt_lib_load()) {
        BTE("faild to load Bluetooth library");
        return -1;
    }

    if (module->methods->open(module, BT_HARDWARE_MODULE_ID, &device)) {
        BTE("faild to open the Bluetooth module");
        return -1;
    }

    bt_device = (bluetooth_device_t*)device;
    bt_iface = bt_device->get_bluetooth_interface();

    status = bt_iface->init(&bt_callbacks);
    if (status != BT_STATUS_SUCCESS) {
        BTE("faild to initialize Bluetooth stack");
        return -1;
    }

    status = bt_iface->set_os_callouts(&bt_os_callouts);
    if (status != BT_STATUS_SUCCESS) {
        BTE("faild to set up Bluetooth OS callouts");
        return -1;
    }

    btif_test_init();

    sem_init(&semt_wait, 0, 0);
    pthread_mutex_init(&mutex_lock, NULL);
    return 0;
}

static void bt_cleanup(void)
{
    BTD();
    bt_iface->cleanup();
    sleep(1);
    BTE("sleep 1 second");
    bt_lib_unload();
}

static void bt_callbacks_init(bthal_callbacks_t* cb)
{
    bt_test_callbacks.nonsig_test_rx_recv_cb = cb->nonsig_test_rx_recv_cb;
    bt_test_callbacks.dut_mode_recv_cb = cb->dut_mode_recv_cb;
}

static int bt_enable(bthal_callbacks_t* callbacks)
{
    struct timeval time_now;
    struct timespec act_timeout;
    BTD();
    bt_callbacks_init(callbacks);
    if (bt_enabled) {
        BTE("bluetooth is already enabled");
        return 0;
    }

    pthread_mutex_lock(&mutex_lock);

    if (bt_init() < 0) {
        pthread_mutex_unlock(&mutex_lock);
        BTE("bt_init failed");
        return -1;
    }

    status = bt_iface->enable(false);
    if (status != BT_STATUS_SUCCESS)
        BTE("HAL REQUEST FAILED status : %d (%s)", status, dump_bt_status(status));

    gettimeofday(&time_now, NULL);
    act_timeout.tv_sec = time_now.tv_sec + 8;
    act_timeout.tv_nsec = time_now.tv_usec * 1000;

    if (sem_timedwait(&semt_wait, &act_timeout) <0 ) {
        BTE("%s timeout", __func__);
    }
    pthread_mutex_unlock(&mutex_lock);

    if (!bt_enabled)
        return -1;

    return 0;
}

static int bt_disable(void)
{
    struct timeval time_now;
    struct timespec act_timeout;

    BTD();
    if (!bt_enabled) {
        BTE("Bluetooth is already disabled");
        return 0;
    }

    pthread_mutex_lock(&mutex_lock);
    status = bt_iface->disable();
    if (status != BT_STATUS_SUCCESS)
        BTE("HAL REQUEST FAILED status : %d (%s)", status, dump_bt_status(status));

    gettimeofday(&time_now, NULL);
    act_timeout.tv_sec = time_now.tv_sec + 8;
    act_timeout.tv_nsec = time_now.tv_usec * 1000;

    if (sem_timedwait(&semt_wait, &act_timeout) <0 ) {
        BTE("%s timeout", __func__);
    }
    pthread_mutex_unlock(&mutex_lock);

    if (bt_enabled) {
        bt_cleanup();
        return -1;
    }

    bt_cleanup();

    return 0;
}

static int bt_dut_mode_configure(uint8_t mode)
{
    BTD("%s", __func__);
    if (!bt_enabled) {
        BTE("bluetooth must be enabled for test_mode to work.");
        return -1;
    }

    status = bt_iface->dut_mode_configure(mode);
    if (status != BT_STATUS_SUCCESS)
        BTE("HAL REQUEST FAILED status : %d (%s)", status, dump_bt_status(status));
    return status;
}

static int bt_dut_mode_send(uint16_t opcode, uint8_t* buf, uint8_t len)
{
    if (bt_iface)
        return bt_iface->dut_mode_send(opcode, buf, len);
    return -1;
}

static int bt_le_test_mode(uint16_t opcode, uint8_t* buf, uint8_t len)
{
    if (bt_iface)
        return bt_iface->le_test_mode(opcode, buf, len);
    return -1;
}


static int bt_set_nonsig_tx_testmode(uint16_t enable,
    uint16_t le, uint16_t pattern, uint16_t channel,
    uint16_t pac_type, uint16_t pac_len, uint16_t power_type,
    uint16_t power_value, uint16_t pac_cnt)
{
    if (sBluetoothTest)
        return sBluetoothTest->set_nonsig_tx_testmode(enable, le, pattern, channel,
            pac_type, pac_len, power_type, power_value, pac_cnt);
    return -1;
}

static int bt_set_nonsig_rx_testmode(uint16_t enable,
    uint16_t le, uint16_t pattern, uint16_t channel,
    uint16_t pac_type,uint16_t rx_gain, bt_bdaddr_t addr)
{
    if (sBluetoothTest)
        return sBluetoothTest->set_nonsig_rx_testmode(enable, le, pattern, channel,
        pac_type, rx_gain, addr);
    return -1;
}

static int bt_get_nonsig_rx_data(uint16_t le)
{
    if (sBluetoothTest)
        return sBluetoothTest->get_nonsig_rx_data(le);
    return -1;
}

static uint8_t is_enable(void)
{
    return bt_enabled;
}


static bt_test_kit_t bt_test_kit = {
    .size = sizeof(bt_test_kit_t),
    .enable = bt_enable,
    .disable = bt_disable,
    .dut_mode_configure = bt_dut_mode_configure,
    .dut_mode_send = bt_dut_mode_send,
    .le_test_mode = bt_le_test_mode,
    .is_enable = is_enable,
    .set_nonsig_tx_testmode = bt_set_nonsig_tx_testmode,
    .set_nonsig_rx_testmode = bt_set_nonsig_rx_testmode,
    .get_nonsig_rx_data = bt_get_nonsig_rx_data
};

const bt_test_kit_t *bt_test_kit_get_interface(void) {
    return &bt_test_kit;
}
#else
const bt_test_kit_t *bt_test_kit_get_interface(void) {
    return NULL;
}
#endif

