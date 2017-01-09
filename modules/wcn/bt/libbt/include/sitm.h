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

#ifndef BT_LIBBT_INCLUDE_SITM_H_
#define BT_LIBBT_INCLUDE_SITM_H_

#include "vnd_buildcfg.h"

#define INVALID_FD (-1)

#ifndef BYTE_ALIGNMENT
#define BYTE_ALIGNMENT 0
#endif

#define PREAMBLE_BUFFER_SIZE 5  // max preamble size, ACL
#define PACKET_TYPE_TO_INDEX(type) ((type) - 1)
#define HCI_COMMAND_PREAMBLE_SIZE 3
#define HCI_ACL_PREAMBLE_SIZE 4
#define HCI_SCO_PREAMBLE_SIZE 3
#define HCI_EVENT_PREAMBLE_SIZE 2
#define RETRIEVE_ACL_LENGTH(preamble) ((((preamble)[4] & 0xFFFF) << 8) | (preamble)[3])
#define HCI_HAL_SERIAL_BUFFER_SIZE 1026

typedef enum {
  DATA_TYPE_COMMAND = 1,
  DATA_TYPE_ACL     = 2,
  DATA_TYPE_SCO     = 3,
  DATA_TYPE_EVENT   = 4
} serial_data_type_t;

typedef enum {
  BRAND_NEW,
  PREAMBLE,
  BODY,
  IGNORE,
  FINISHED
} receive_state_t;

typedef struct {
  receive_state_t state;
  uint16_t bytes_remaining;
  uint8_t type;
  uint8_t preamble[PREAMBLE_BUFFER_SIZE];
  uint16_t index;
  uint8_t buffer[HCI_HAL_SERIAL_BUFFER_SIZE + BYTE_ALIGNMENT];
} packet_receive_data_t;


typedef int (*frame_complete_cb)(uint8_t *data, uint32_t len);
typedef int (*data_ready_cb)(uint8_t *data, uint32_t len);


#define CASE_RETURN_STR(const) case const: return #const;


int sitm_server_start_up(int fd);
int sitm_server_shut_down();
#endif  // BT_LIBBT_INCLUDE_SITM_H_
