/**
 * Copyright (C) 2016 Spreadtrum Communications Inc.
  **/

#ifndef BT_LIBBQBBT_INCLUDE_BQB_H_
#define BT_LIBBQBBT_INCLUDE_BQB_H_

#define BQB_CTRL_PATH "/data/misc/.bqb_ctrl"
#define ROUTE_BQB 0
#define ROUTE_AT 1
#define ENABLE_BQB_TEST "AT+SPBQBTEST=1"
#define DISABLE_BQB_TEST "AT+SPBQBTEST=0"
#define TRIGGER_BQB_TEST "AT+SPBQBTEST=?"
#define NOTIFY_BQB_ENABLE "\r\n+SPBQBTEST OK: ENABLED\r\n"
#define NOTIFY_BQB_DISABLE "\r\n+SPBQBTEST OK: DISABLE\r\n"
#define TRIGGER_BQB_ENABLE "\r\n+SPBQBTEST OK: BQB\r\n"
#define TRIGGER_BQB_DISABLE "\r\n+SPBQBTEST OK: AT\r\n"
#define UNKNOW_COMMAND "\r\n+SPBQBTEST ERROR: UNKNOW COMMAND\r\n"

#define ENABLE_WCNIT_TEST "AT+SPBQBTEST=2"
#define NOTIFY_WCNIT_ENABLE "\r\n+SPBQBTEST WCNIT OK: ENABLED\r\n"

typedef enum {
    BQB_NORMAL,
    BQB_NOPSKEY,
} bt_bqb_mode_t;

typedef enum {
    BQB_CLOSED,
    BQB_OPENED,
} bt_bqb_state_t;


/*
 * Bluetooth BQB Channel Control Interface
 */
typedef struct {
    /** Set to sizeof(bt_vndor_interface_t) */
    size_t          size;

    /*
     * Functions need to be implemented in Vendor libray (libbt-vendor.so).
     */

    /**
     * Caller will open the interface and pass in the callback routines
     * to the implemenation of this interface.
     */
    void (*init)(void);

    /**  Vendor specific operations */
    void (*exit)(void);
    void (*set_fd)(int fd);
    int (*get_bqb_state)(void);
    int (*check_received_str)(int fd, char* engbuf, int len);
    void (*eng_send_data)(char *engbuf, int len);
} bt_bqb_interface_t;

int current_bqb_state;

int bt_on(bt_bqb_mode_t mode);
void bt_off(void);

typedef int (*controller2uplayer_t)(char * buf, unsigned int len);

void lmp_assert(void);
void lmp_deassert(void);

int get_bqb_state(void);
void set_bqb_state(int state);

#endif  // BT_LIBBQBBT_INCLUDE_BQB_H_
