#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>

#include "ak.h"
#include "timer.h"

#include "firmware.h"

#include "sys_boot.h"
#include "sys_dbg.h"

#include "app.h"
#include "app_if.h"
#include "app_dbg.h"
#include "app_data.h"

#include "task_list.h"
#include "task_list_if.h"
#include "task_fw.h"
#include "task_if.h"

using namespace std;

typedef struct {
	firmware_header_t fw_header;
	int transfer;
} transfer_fw_status_t;

q_msg_t gw_task_fw_mailbox;

static uint32_t fw_bin_index;
static uint32_t fw_bin_packet_len;

firmware_header_t current_firmware_info;
firmware_header_t file_firmware_info;

string firmware_file_name;

gateway_fw_dev_update_req_t gateway_fw_dev_update_req;

#define GW_FW_PACKED_TIMEOUT_RETRY_COUNTER_MAX		9

static uint8_t gw_fw_packed_timeout_retry_counter;

static void as_sm_release_firmware_update();

static void fw_update_err(uint32_t);
static void fw_no_need_to_update(firmware_header_t*);
static void fw_started_transfer(firmware_header_t*);
static void fw_device_busy();
static void fw_packed_time_out();
static void fw_transfer_status(transfer_fw_status_t*);
static void fw_checksum_err();
static void fw_device_internal_update_started();
static void fw_update_completed();

void* gw_task_fw_entry(void*) {
	ak_msg_t* msg = AK_MSG_NULL;

	string firmware_binary_path = static_cast<string>(APP_ROOT_PATH_DISK) + static_cast<string>("/dev_firmware");

	struct stat st = {0};
	if (stat(firmware_binary_path.data(), &st) == -1) {
		mkdir(firmware_binary_path.data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}

	wait_all_tasks_started();

	APP_DBG("[STARTED] gw_task_fw_entry\n");

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_FW_ID);

		/* handler message */
		switch (msg->header->sig) {
		case GW_FW_UPDATE_COMPLETED: {
			APP_DBG_SIG("GW_FW_UPDATE_COMPLETED\n");
			timer_remove_attr(GW_TASK_FW_ID, GW_FW_DEV_INTERNAL_UPDATE_TIMEOUT);
			as_sm_release_firmware_update();
			fw_update_completed();
		}
			break;

		case GW_FW_DEV_INTERNAL_UPDATE_TIMEOUT: {
			APP_DBG_SIG("GW_FW_DEV_INTERNAL_UPDATE_TIMEOUT\n");
			as_sm_release_firmware_update();
			fw_update_err(3);
		}
			break;

		case GW_FW_PACKED_TIMEOUT: {
			APP_DBG_SIG("GW_FW_PACKED_TIMEOUT\n");

			if (gw_fw_packed_timeout_retry_counter++ > GW_FW_PACKED_TIMEOUT_RETRY_COUNTER_MAX) {
				as_sm_release_firmware_update();
				fw_packed_time_out();
			}
			else {
				/* resend chain firmware packet */
				fw_bin_index -= fw_bin_packet_len;

				task_post_pure_msg(GW_TASK_FW_ID, GW_TASK_FW_ID, GW_FW_TRANFER_REQ);
			}
		}
			break;

		case GW_FW_OTA_REQ: {
			APP_DBG_SIG("GW_FW_OTA_REQ\n");
			get_data_dynamic_msg(msg, (uint8_t*)&gateway_fw_dev_update_req, sizeof(gateway_fw_dev_update_req_t));

			firmware_file_name.assign((const char*)gateway_fw_dev_update_req.dev_bin_path);

			APP_DBG("FILE:%s\n", firmware_file_name.data());

			if (stat(firmware_file_name.data(), &st) == -1) {
				APP_DBG("binary file:%s does not exist !\n", firmware_file_name.data());
				fw_update_err(1);
			}
			else {
				task_post_pure_msg(GW_TASK_FW_ID, GW_TASK_FW_ID, GW_FW_SM_UPDATE_RES_OK);
			}
		}
			break;

		case GW_FW_SM_UPDATE_RES_OK: {
			APP_DBG_SIG("GW_FW_SM_UPDATE_RES_OK\n");
			ak_msg_t* s_msg = get_pure_msg();
			set_msg_sig(s_msg, GW_IF_PURE_MSG_OUT);

			set_if_src_task_id(s_msg, GW_TASK_FW_ID);
			set_if_des_task_id(s_msg, gateway_fw_dev_update_req.target_task_id);
			set_if_src_type(s_msg, gateway_fw_dev_update_req.source_if_type);
			set_if_des_type(s_msg, gateway_fw_dev_update_req.target_if_type);

			if (gateway_fw_dev_update_req.type_update == TYPE_UPDATE_TARTGET_APP) {
				set_if_sig(s_msg, FW_CRENT_APP_FW_INFO_REQ);
			}
			else if (gateway_fw_dev_update_req.type_update == TYPE_UPDATE_TARTGET_BOOT) {
				set_if_sig(s_msg, FW_CRENT_BOOT_FW_INFO_REQ);
			}
			else {
				FATAL("GFW", 0x01);
			}

			set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
			task_post(GW_TASK_IF_ID, s_msg);

			timer_set(GW_TASK_FW_ID, GW_FW_GET_FIRMWARE_INFO_TIMEOUT, GW_TIMER_FIRMWARE_GET_FIRMWARE_INFO_TIMEOUT, TIMER_ONE_SHOT);
		}
			break;

		case GW_FW_GET_FIRMWARE_INFO_TIMEOUT: {
			APP_DBG_SIG("GW_FW_GET_FIRMWARE_INFO_TIMEOUT\n");
			as_sm_release_firmware_update();
			fw_update_err(2);
		}
			break;

		case GW_FW_CURRENT_INFO_RES: {
			timer_remove_attr(GW_TASK_FW_ID, GW_FW_GET_FIRMWARE_INFO_TIMEOUT);

			APP_DBG_SIG("GW_FW_CURRENT_INFO_RES\n");
			get_data_common_msg(msg, (uint8_t*)&current_firmware_info, sizeof(firmware_header_t));
			APP_PRINT("current firmware checksum: %04X\n", current_firmware_info.checksum);
			APP_PRINT("current firmware bin_len: %d\n", current_firmware_info.bin_len);

			firmware_get_info(&file_firmware_info, firmware_file_name.data());
			APP_PRINT("file firmware checksum: %04X\n", file_firmware_info.checksum);
			APP_PRINT("file firmware bin_len: %d\n", file_firmware_info.bin_len);

			if (current_firmware_info.checksum == file_firmware_info.checksum) {
				as_sm_release_firmware_update();
				fw_no_need_to_update(&current_firmware_info);
			}
			else {
				ak_msg_t* s_msg = get_common_msg();

				set_if_src_task_id(s_msg, GW_TASK_FW_ID);
				set_if_src_type(s_msg, gateway_fw_dev_update_req.source_if_type);
				set_if_des_task_id(s_msg, gateway_fw_dev_update_req.target_task_id);
				set_if_des_type(s_msg, gateway_fw_dev_update_req.target_if_type);
				set_if_sig(s_msg, FW_UPDATE_REQ);
				set_if_data_common_msg(s_msg, (uint8_t*)&file_firmware_info, sizeof(firmware_header_t));

				set_msg_sig(s_msg, GW_IF_COMMON_MSG_OUT);
				set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
				task_post(GW_TASK_IF_ID, s_msg);
			}
		}
			break;

		case GW_FW_UPDATE_RES_OK: {
			APP_DBG_SIG("GW_FW_UPDATE_RES_OK\n");
			fw_bin_index = 0;

			task_post_pure_msg(GW_TASK_FW_ID, GW_TASK_FW_ID, GW_FW_TRANFER_REQ);

			fw_started_transfer(&file_firmware_info);
		}
			break;

		case GW_FW_TRANFER_REQ: {
			uint8_t data_temp[AK_COMMON_MSG_DATA_SIZE];
			uint32_t remain;
			memset(data_temp, 0, AK_COMMON_MSG_DATA_SIZE);

			remain = file_firmware_info.bin_len - fw_bin_index;

			if (remain < AK_COMMON_MSG_DATA_SIZE) {
				fw_bin_packet_len = (uint8_t)remain;
			}
			else {
				fw_bin_packet_len = AK_COMMON_MSG_DATA_SIZE;
			}

			firmware_read(data_temp, fw_bin_index, fw_bin_packet_len, firmware_file_name.data());
			fw_bin_index += fw_bin_packet_len;

			/* reset counter retry */
			gw_fw_packed_timeout_retry_counter = 0;

			if (fw_bin_index < file_firmware_info.bin_len) {
				timer_set(GW_TASK_FW_ID, GW_FW_PACKED_TIMEOUT, GW_TIMER_FIRMWARE_PACKED_TIMEOUT_INTERVAL, TIMER_ONE_SHOT);
			}

			ak_msg_t* s_msg = get_common_msg();

			set_if_src_task_id(s_msg, GW_TASK_FW_ID);
			set_if_src_type(s_msg, gateway_fw_dev_update_req.source_if_type);
			set_if_des_task_id(s_msg, gateway_fw_dev_update_req.target_task_id);
			set_if_des_type(s_msg, gateway_fw_dev_update_req.target_if_type);
			set_if_sig(s_msg, FW_TRANSFER_REQ);
			set_if_data_common_msg(s_msg, data_temp, AK_COMMON_MSG_DATA_SIZE);

			set_msg_sig(s_msg, GW_IF_COMMON_MSG_OUT);
			set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
			task_post(GW_TASK_IF_ID, s_msg);

			float percent = ((float)fw_bin_index / (float)file_firmware_info.bin_len) * (float)100;
			APP_PRINT("[transfer] %d bytes %d %c\n", fw_bin_index, (uint32_t)percent, '%');
			(void)percent;

			transfer_fw_status_t fw_stt;
			fw_stt.fw_header.bin_len = file_firmware_info.bin_len;
			fw_stt.fw_header.checksum = file_firmware_info.checksum;
			fw_stt.fw_header.psk = file_firmware_info.psk;
			fw_stt.transfer = fw_bin_index;
			fw_transfer_status(&fw_stt);
		}
			break;

		case GW_FW_TRANSFER_RES_OK: {
			/* clear packed timeout and trigger next sequence */
			timer_remove_attr(GW_TASK_FW_ID, GW_FW_PACKED_TIMEOUT);

			task_post_pure_msg(GW_TASK_FW_ID, GW_TASK_FW_ID, GW_FW_TRANFER_REQ);
		}
			break;

		case GW_FW_INTERNAL_UPDATE_REQ: {
			APP_DBG_SIG("GW_FW_INTERNAL_UPDATE_REQ\n");

			timer_remove_attr(GW_TASK_FW_ID, GW_FW_PACKED_TIMEOUT);

			ak_msg_t* s_msg = get_pure_msg();

			set_if_src_task_id(s_msg, GW_TASK_FW_ID);
			set_if_src_type(s_msg, gateway_fw_dev_update_req.source_if_type);
			set_if_des_task_id(s_msg, gateway_fw_dev_update_req.target_task_id);
			set_if_des_type(s_msg, gateway_fw_dev_update_req.target_if_type);

			if (gateway_fw_dev_update_req.type_update == TYPE_UPDATE_TARTGET_APP) {
				set_if_sig(s_msg, FW_INTERNAL_UPDATE_APP_RES_OK);
			}
			else if (gateway_fw_dev_update_req.type_update == TYPE_UPDATE_TARTGET_BOOT) {
				set_if_sig(s_msg, FW_INTERNAL_UPDATE_BOOT_RES_OK);
			}
			else {
				FATAL("GFW", 0x02);
			}

			set_msg_sig(s_msg, GW_IF_PURE_MSG_OUT);
			set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
			task_post(GW_TASK_IF_ID, s_msg);

			timer_set(GW_TASK_FW_ID, GW_FW_DEV_INTERNAL_UPDATE_TIMEOUT, GW_TIMER_FIRMWARE_DEV_INTERNAL_UPDATE_TIMEOUT, TIMER_ONE_SHOT);
			fw_device_internal_update_started();
		}
			break;

		case GW_FW_UPDATE_BUSY: {
			APP_DBG_SIG("GW_FW_UPDATE_BUSY\n");
			as_sm_release_firmware_update();
			fw_device_busy();
		}
			break;

		case GW_FW_TRANSFER_CHECKSUM_ERR: {
			APP_DBG_SIG("GW_FW_TRANSFER_CHECKSUM_ERR\n");
			timer_remove_attr(GW_TASK_FW_ID, GW_FW_PACKED_TIMEOUT);
			as_sm_release_firmware_update();
			fw_checksum_err();
		}
			break;

		case GW_FW_SM_RELEASE_RES_OK: {
			APP_DBG_SIG("GW_FW_SM_RELEASE_RES_OK\n");
		}

		default:
			break;
		}

		ak_msg_free(msg);
	}

	return (void*)0;
}

void as_sm_release_firmware_update() {
	APP_PRINT("[FW] AS_SM_RELEASE_FIRMWARE_UPDATE\n");

	//	ak_msg_t* s_msg = get_pure_msg();
	//	set_msg_sig(s_msg, GW_AC_SM_FIRMWARE_UPDATE_RELEASE_REQ);

	//	set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
	//	task_post(GW_TASK_AC_SM_ID, s_msg);
}

void fw_update_err(uint32_t err) {
	(void)err;
	APP_PRINT("[FW] FW_UPDATE_ERR: %d\n", err);
	//	ak_msg_t* s_msg = get_common_msg();

	//	set_if_src_task_id(s_msg, GW_TASK_FW_ID);
	//	set_if_des_task_id(s_msg, GI_TASK_FIRMWARE_ID);
	//	set_if_des_type(s_msg, IF_TYPE_APP_GI);
	//	set_if_sig(s_msg, GI_FIRMWARE_UPDATE_ERR);
	//	set_if_data_common_msg(s_msg, (uint8_t*)&err, sizeof(uint32_t));

	//	set_msg_sig(s_msg, GW_IF_COMMON_MSG_OUT);
	//	set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
	//	task_post(GW_TASK_IF_ID, s_msg);
}

void fw_no_need_to_update(firmware_header_t* fw) {
	(void)fw;
	APP_PRINT("[FW] FW_NO_NEED_TO_UPDATE\n");
	//	ak_msg_t* s_msg = get_common_msg();

	//	set_if_src_task_id(s_msg, GW_TASK_FW_ID);
	//	set_if_des_task_id(s_msg, GI_TASK_FIRMWARE_ID);
	//	set_if_des_type(s_msg, IF_TYPE_APP_GI);
	//	set_if_sig(s_msg, GI_FIRMWARE_NO_NEED_TO_UPDATE);
	//	set_if_data_common_msg(s_msg, (uint8_t*)fw, sizeof(firmware_header_t));

	//	set_msg_sig(s_msg, GW_IF_COMMON_MSG_OUT);
	//	set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
	//	task_post(GW_TASK_IF_ID, s_msg);
}

void fw_started_transfer(firmware_header_t* fw) {
	(void)fw;
	APP_PRINT("[FW] FW_STARTED_TRANSFER\n");
	//	ak_msg_t* s_msg = get_common_msg();

	//	set_if_src_task_id(s_msg, GW_TASK_FW_ID);
	//	set_if_des_task_id(s_msg, GI_TASK_FIRMWARE_ID);
	//	set_if_des_type(s_msg, IF_TYPE_APP_GI);
	//	set_if_sig(s_msg, GI_FIRMWARE_STARTED_TRANSFER);
	//	set_if_data_common_msg(s_msg, (uint8_t*)fw, sizeof(firmware_header_t));

	//	set_msg_sig(s_msg, GW_IF_COMMON_MSG_OUT);
	//	set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
	//	task_post(GW_TASK_IF_ID, s_msg);
}

void fw_device_busy() {
	APP_PRINT("[FW] FW_DEVICE_BUSY\n");
	//	ak_msg_t* s_msg = get_pure_msg();

	//	set_if_src_task_id(s_msg, GW_TASK_FW_ID);
	//	set_if_des_task_id(s_msg, GI_TASK_FIRMWARE_ID);
	//	set_if_des_type(s_msg, IF_TYPE_APP_GI);
	//	set_if_sig(s_msg, GI_FIRMWARE_DEVICE_BUSY);

	//	set_msg_sig(s_msg, GW_IF_PURE_MSG_OUT);
	//	set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
	//	task_post(GW_TASK_IF_ID, s_msg);
}

void fw_packed_time_out() {
	APP_PRINT("[FW] FW_PACKED_TIME_OUT\n");
	//	ak_msg_t* s_msg = get_pure_msg();

	//	set_if_src_task_id(s_msg, GW_TASK_FW_ID);
	//	set_if_des_task_id(s_msg, GI_TASK_FIRMWARE_ID);
	//	set_if_des_type(s_msg, IF_TYPE_APP_GI);
	//	set_if_sig(s_msg, GI_FIRMWARE_PACKED_TIMEOUT);

	//	set_msg_sig(s_msg, GW_IF_PURE_MSG_OUT);
	//	set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
	//	task_post(GW_TASK_IF_ID, s_msg);
}

void fw_transfer_status(transfer_fw_status_t* stt) {
	(void)stt;
	APP_PRINT("[FW] FW_TRANSFER_STATUS\n");
	//	ak_msg_t* s_msg = get_common_msg();

	//	set_if_src_task_id(s_msg, GW_TASK_FW_ID);
	//	set_if_des_task_id(s_msg, GI_TASK_FIRMWARE_ID);
	//	set_if_des_type(s_msg, IF_TYPE_APP_GI);
	//	set_if_sig(s_msg, GI_FIRMWARE_TRANSFER_STATUS);
	//	set_if_data_common_msg(s_msg, (uint8_t*)stt, sizeof(transfer_fw_status_t));

	//	set_msg_sig(s_msg, GW_IF_COMMON_MSG_OUT);
	//	set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
	//	task_post(GW_TASK_IF_ID, s_msg);
}

void fw_checksum_err() {
	APP_PRINT("[FW] FW_CHECKSUM_ERR\n");
	//	ak_msg_t* s_msg = get_pure_msg();

	//	set_if_src_task_id(s_msg, GW_TASK_FW_ID);
	//	set_if_des_task_id(s_msg, GI_TASK_FIRMWARE_ID);
	//	set_if_des_type(s_msg, IF_TYPE_APP_GI);
	//	set_if_sig(s_msg, GI_FIRMWARE_CHECKSUM_ERR);

	//	set_msg_sig(s_msg, GW_IF_PURE_MSG_OUT);
	//	set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
	//	task_post(GW_TASK_IF_ID, s_msg);
}

void fw_device_internal_update_started() {
	APP_PRINT("[FW] FW_DEVICE_INTERNAL_UPDATE_STARTED\n");
	//	ak_msg_t* s_msg = get_pure_msg();

	//	set_if_src_task_id(s_msg, GW_TASK_FW_ID);
	//	set_if_des_task_id(s_msg, GI_TASK_FIRMWARE_ID);
	//	set_if_des_type(s_msg, IF_TYPE_APP_GI);
	//	set_if_sig(s_msg, GI_FIRMWARE_DEVICE_INTERNAL_UPDATE_STARTED);

	//	set_msg_sig(s_msg, GW_IF_PURE_MSG_OUT);
	//	set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
	//	task_post(GW_TASK_IF_ID, s_msg);
}

void fw_update_completed() {
	APP_PRINT("[FW] FW_UPDATE_COMPLETED\n");
	//	ak_msg_t* s_msg = get_pure_msg();

	//	set_if_src_task_id(s_msg, GW_TASK_FW_ID);
	//	set_if_des_task_id(s_msg, GI_TASK_FIRMWARE_ID);
	//	set_if_des_type(s_msg, IF_TYPE_APP_GI);
	//	set_if_sig(s_msg, GI_FIRMWARE_UPDATE_COMPLETED);

	//	set_msg_sig(s_msg, GW_IF_PURE_MSG_OUT);
	//	set_msg_src_task_id(s_msg, GW_TASK_FW_ID);
	//	task_post(GW_TASK_IF_ID, s_msg);
}
