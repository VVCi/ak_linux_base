#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "app_if.h"
#include "app_data.h"
#include "app_dbg.h"

#include "task_list.h"
#include "task_list_if.h"
#include "if_rf24.h"
#include "task_debug_msg.h"

#include "link.h"
#include "link_sig.h"

q_msg_t gw_task_debug_msg_mailbox;

void* gw_task_debug_msg_entry(void*) {
	ak_msg_t* msg = AK_MSG_NULL;

	wait_all_tasks_started();

	APP_DBG("[STARTED] gw_task_debug_msg_entry\n");

//	timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_2, 110, TIMER_ONE_SHOT);
//	timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_4, 9, TIMER_ONE_SHOT);
//	timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_6, 100, TIMER_ONE_SHOT);
//	timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_5, 1, TIMER_PERIODIC);
//	timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_8, 1, TIMER_PERIODIC);
//	timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_9, 1, TIMER_PERIODIC);
//	timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_10, 1, TIMER_PERIODIC);
//	timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_11, 1, TIMER_ONE_SHOT);
//	timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_12, 1, TIMER_ONE_SHOT);

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_DEBUG_MSG_ID);

		switch (msg->header->sig) {
		case GW_DEBUG_MSG_1: {
			APP_DBG_SIG("GW_DEBUG_MSG_1\n");
			uint8_t data_len = get_data_len_dynamic_msg(msg);
			APP_DBG_SIG("data_len: %d\n", data_len);

			uint8_t* rev_data = (uint8_t*)malloc(data_len);
			get_data_dynamic_msg(msg, rev_data, data_len);

			APP_DBG_SIG("rev_data:");
			for (uint32_t i = 0; i < data_len; i++) {
				RAW_DBG(" %02X", *(rev_data + i));
			}
			RAW_DBG("\n");
		}
			break;

		case GW_DEBUG_MSG_2: {
			APP_DBG_SIG("GW_DEBUG_MSG_2\n");
			timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_3, 2, TIMER_ONE_SHOT);
		}
			break;

		case GW_DEBUG_MSG_3: {
			APP_DBG_SIG("GW_DEBUG_MSG_3\n");
			uint8_t test_buf[64];
			for (int i = 0; i < 64; i++) {
				test_buf[i] = 0xAA;
			}

			ak_msg_t* s_msg = get_common_msg();
			set_if_des_type(s_msg, IF_TYPE_UART_AC);
			set_if_src_type(s_msg, IF_TYPE_UART_GW);
			set_if_des_task_id(s_msg, AC_TASK_DBG_ID);
			set_if_sig(s_msg, AC_DBG_TEST_1);
			set_if_data_common_msg(s_msg, test_buf, 64);

			set_msg_sig(s_msg, GW_LINK_SEND_COMMON_MSG);
			task_post(GW_LINK_ID, s_msg);
		}
			break;

		case GW_DEBUG_MSG_4: {
			static int gw_debug_msg_4_counter = 0;
			if (gw_debug_msg_4_counter++ > 100) {
				gw_debug_msg_4_counter = 0;
				APP_DBG_SIG("GW_DEBUG_MSG_4\n");
			}
			timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_4, 10, TIMER_ONE_SHOT);
		}
			break;

		case GW_DEBUG_MSG_5: {
			static int gw_debug_msg_5_counter = 0;
			if (gw_debug_msg_5_counter++ > 1000) {
				gw_debug_msg_5_counter = 0;
				APP_DBG_SIG("GW_DEBUG_MSG_5\n");
			}
			timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_7, 1, TIMER_ONE_SHOT);
		}
			break;

		case GW_DEBUG_MSG_6: {
			APP_DBG_SIG("GW_DEBUG_MSG_6\n");
			uint8_t test_buf[256];
			for (int i = 0; i < 256; i++) {
				test_buf[i] = i;
			}

			ak_msg_t* s_msg = get_dynamic_msg();
			set_if_des_type(s_msg, IF_TYPE_UART_AC);
			set_if_src_type(s_msg, IF_TYPE_UART_GW);
			set_if_des_task_id(s_msg, AC_TASK_DBG_ID);
			set_if_sig(s_msg, AC_DBG_TEST_2);
			set_if_data_dynamic_msg(s_msg, test_buf, 256);

			set_msg_sig(s_msg, GW_LINK_SEND_DYNAMIC_MSG);
			task_post(GW_LINK_ID, s_msg);
		}
			break;

		case GW_DEBUG_MSG_7: {
			static int gw_debug_msg_7_counter = 0;
			if (gw_debug_msg_7_counter++ > 1000) {
				gw_debug_msg_7_counter = 0;
				APP_DBG_SIG("GW_DEBUG_MSG_7\n");
			}
			task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_DBG_4);
		}
			break;

		case GW_DEBUG_MSG_8: {
			static int gw_debug_msg_8_counter = 0;
			if (gw_debug_msg_8_counter++ > 1000) {
				gw_debug_msg_8_counter = 0;
				APP_DBG_SIG("GW_DEBUG_MSG_8\n");
			}
			task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_DBG_3);
		}
			break;

		case GW_DEBUG_MSG_9: {
			static int gw_debug_msg_9_counter = 0;
			if (gw_debug_msg_9_counter++ > 1000) {
				gw_debug_msg_9_counter = 0;
				APP_DBG_SIG("GW_DEBUG_MSG_9\n");
			}
			task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_DBG_2);
		}
			break;

		case GW_DEBUG_MSG_10: {
			static int gw_debug_msg_10_counter = 0;
			if (gw_debug_msg_10_counter++ > 1000) {
				gw_debug_msg_10_counter = 0;
				APP_DBG_SIG("GW_DEBUG_MSG_10\n");
			}
			timer_set(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_REPORT_REQ, 1, TIMER_ONE_SHOT);
		}
			break;

		case GW_DEBUG_MSG_11: {
			static int gw_debug_msg_11_counter = 0;
			if (gw_debug_msg_11_counter++ > 1000) {
				gw_debug_msg_11_counter = 0;
				APP_DBG_SIG("GW_DEBUG_MSG_11\n");
			}
			timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_11, 2, TIMER_ONE_SHOT);
		}
			break;

		case GW_DEBUG_MSG_12: {
			static int gw_debug_msg_12_counter = 0;
			if (gw_debug_msg_12_counter++ > 1000) {
				gw_debug_msg_12_counter = 0;
				APP_DBG_SIG("GW_DEBUG_MSG_12\n");
			}
			timer_set(GW_TASK_DEBUG_MSG_ID, GW_DEBUG_MSG_12, 1, TIMER_ONE_SHOT);
			task_post_pure_msg(GW_TASK_SYS_ID, GW_SYS_WATCH_DOG_DBG_1);
		}
			break;

		default:
			break;
		}

		/* free message */
		ak_msg_free(msg);
	}

	return (void*)0;
}
