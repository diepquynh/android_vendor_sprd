#ifndef _AUDIOTESTER_SERVER_H_
#define _AUDIOTESTER_SERVER_H_
typedef enum {
    GET_INFO,
    GET_PARAM_FROM_RAM,
    SET_PARAM_FROM_RAM,
    GET_PARAM_FROM_FLASH,
    SET_PARAM_FROM_FLASH,
    GET_REGISTER,
    SET_REGISTER,
    SET_PARAM_VOLUME=14,
    SET_VOLUME_APP_TYPE=15,
    CONNECT_AUDIOTESTER,
    DIS_CONNECT_AUDIOTESTER,
    HARDWARE_TEST_CMD=0xf0,
    AUDIO_EXT_TEST_CMD=0xf1,
    SOCKET_TEST_CMD=0xff,
} AUDIO_CMD;

typedef enum {
    DATA_SINGLE = 1,
    DATA_START,
    DATA_END,
    DATA_MIDDLE,
    DATA_STATUS_OK,
    DATA_STATUS_ERROR,
} AUDIO_DATA_STATE;

// This is the communication frame head
typedef struct msg_head_tag {
    unsigned int  seq_num;      // Message sequence number, used for flow control
    unsigned short
    len;          // The totoal size of the packet "sizeof(MSG_HEAD_T)+ packet size"
    unsigned char   type;         // Main command type
    unsigned char   subtype;      // Sub command type
} __attribute__((packed)) MSG_HEAD_T;

//byte 1 is data state, and other 19 bytes is reserve
typedef struct data_command {
    unsigned char data_state;
    unsigned char reserve[19];
} __attribute__((packed)) DATA_HEAD_T;


#endif
