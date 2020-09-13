#ifndef __TASK_FW_H__
#define __TASK_FW_H__

#include <stdint.h>
#include <string>

#include "message.h"

#define DEV_BIN_PATH_MAX_LEN			1024

#define TYPE_UPDATE_TARTGET_BOOT		0x01
#define TYPE_UPDATE_TARTGET_APP			0x02

typedef struct {
	char dev_bin_path[DEV_BIN_PATH_MAX_LEN];
	uint8_t type_update;
	uint8_t source_if_type;
	uint8_t target_task_id;
	uint8_t target_if_type;
} gateway_fw_dev_update_req_t;

extern q_msg_t gw_task_fw_mailbox;
extern void* gw_task_fw_entry(void*);

#endif //__TASK_FW_H__
