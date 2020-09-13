#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/un.h>

#include "ak.h"

#include "sys_dbg.h"

#include "app.h"
#include "app_if.h"
#include "app_dbg.h"
#include "app_data.h"

#include "if_console.h"
#include "task_list.h"
#include "task_list_if.h"

#include "if_app.h"

#define DEBUG_REV_MSG			0

q_msg_t gw_task_if_app_mailbox;

#define APP_IF_REV_BUF_SIZE				1024
#define APP_IF_BACKLOG					5

static int sfd;
static pthread_t app_rev_thread_id;
static void* app_rev_thread_entry(void*);

static bool is_if_app_id_existed(uint32_t id);
static int send_message_to_app(uint32_t if_des_type, uint8_t* data, uint32_t len);
static char* get_if_socket_path(uint32_t id);

void* gw_task_if_app_entry(void*) {
	ak_msg_t* msg = AK_MSG_NULL;

	struct sockaddr_un addr;

	/* create gateway server socket */
	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd == -1) {
		APP_DBG("[ERR] socket\n");
	}

	/* remove exist path */
	if (remove(get_if_socket_path(IF_TYPE_APP_GW)) == -1 && errno != ENOENT) {
		APP_DBG("[ERR]remove-%s", get_if_socket_path(IF_TYPE_APP_GW));
	}

	/* bind socket */
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, get_if_socket_path(IF_TYPE_APP_GW), sizeof(addr.sun_path) - 1);
	if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
		APP_DBG("[ERR] bind\n");
	}

	/* start listen socket */
	if (listen(sfd, APP_IF_BACKLOG) == -1) {
		APP_DBG("[ERR]listen\n");
	}

	wait_all_tasks_started();

	APP_DBG("[STARTED] GW_task_if_app_entry\n");

	pthread_create(&app_rev_thread_id, NULL, app_rev_thread_entry, NULL);

	while (1) {

		msg = ak_msg_rev(GW_TASK_IF_APP_ID);

		if (is_if_app_id_existed(msg->header->if_des_type)) {
			switch (get_msg_type(msg)) {
			case PURE_MSG_TYPE: {
				ak_msg_pure_if_t app_if_msg;
				memset(&app_if_msg, 0, sizeof(ak_msg_pure_if_t));

				/* assign if message */
				app_if_msg.header.type = PURE_MSG_TYPE;
				app_if_msg.header.if_src_type = IF_TYPE_APP_GW;
				app_if_msg.header.if_des_type = msg->header->if_des_type;
				app_if_msg.header.sig = msg->header->if_sig;
				app_if_msg.header.src_task_id = msg->header->if_src_task_id;
				app_if_msg.header.des_task_id = msg->header->if_des_task_id;

				send_message_to_app(msg->header->if_des_type, (uint8_t*)&app_if_msg, sizeof(ak_msg_pure_if_t));
			}
				break;

			case COMMON_MSG_TYPE: {
				ak_msg_common_if_t app_if_msg;
				memset(&app_if_msg, 0, sizeof(ak_msg_common_if_t));

				/* assign if message */
				app_if_msg.header.type = COMMON_MSG_TYPE;
				app_if_msg.header.if_src_type = IF_TYPE_APP_GW;
				app_if_msg.header.if_des_type = msg->header->if_des_type;
				app_if_msg.header.sig = msg->header->if_sig;
				app_if_msg.header.src_task_id = msg->header->if_src_task_id;
				app_if_msg.header.des_task_id = msg->header->if_des_task_id;

				app_if_msg.len = msg->header->len;
				get_data_common_msg(msg, app_if_msg.data, msg->header->len);

				send_message_to_app(msg->header->if_des_type, (uint8_t*)&app_if_msg, sizeof(ak_msg_common_if_t));
			}
				break;

			default:
				break;
			}
		}
		else {
			APP_DBG("[ERR] app id does not exist !\n");
		}

		/* free message */
		ak_msg_free(msg);
	}

	return (void*)0;
}

void* app_rev_thread_entry(void*) {
	APP_DBG("app_rev_thread_entry\n");
	int cfd;
	ssize_t nums_read;
	char rev_buf[APP_IF_REV_BUF_SIZE];

	while (1) {
		/* accept client connection */
		cfd = accept(sfd, NULL, NULL);

		if (cfd == -1) {
			APP_DBG("[ERR] accept\n");
		}

		while ((nums_read = read(cfd, rev_buf, APP_IF_REV_BUF_SIZE)) > 0) {
#if (DEBUG_REV_MSG == 1)
			APP_DBG("received: %d\n", (int)nums_read);
			for (int i = 0; i < nums_read; i++) {
				APP_DBG("%02X ", (uint8_t)rev_buf[i]);
			}
			APP_DBG("\n");
#endif

			ak_msg_if_header_t* if_msg_header = (ak_msg_if_header_t*)rev_buf;
			switch (if_msg_header->type) {
			case PURE_MSG_TYPE: {
				ak_msg_t* s_msg = get_pure_msg();

				set_if_src_task_id(s_msg, if_msg_header->src_task_id);
				set_if_des_task_id(s_msg, if_msg_header->des_task_id);
				set_if_src_type(s_msg, if_msg_header->if_src_type);
				set_if_des_type(s_msg, if_msg_header->if_des_type);
				set_if_sig(s_msg, if_msg_header->sig);

				set_msg_sig(s_msg, GW_IF_PURE_MSG_IN);
				set_msg_src_task_id(s_msg, GW_TASK_IF_APP_ID);
				task_post(GW_TASK_IF_ID, s_msg);
			}
				break;

			case COMMON_MSG_TYPE: {
				ak_msg_t* s_msg = get_common_msg();

				set_if_src_task_id(s_msg, if_msg_header->src_task_id);
				set_if_des_task_id(s_msg, if_msg_header->des_task_id);
				set_if_src_type(s_msg, if_msg_header->if_src_type);
				set_if_des_type(s_msg, if_msg_header->if_des_type);
				set_if_sig(s_msg, if_msg_header->sig);
				set_if_data_common_msg(s_msg, ((ak_msg_common_if_t*)if_msg_header)->data, ((ak_msg_common_if_t*)if_msg_header)->len);

				set_msg_sig(s_msg, GW_IF_COMMON_MSG_IN);
				set_msg_src_task_id(s_msg, GW_TASK_IF_APP_ID);
				task_post(GW_TASK_IF_ID, s_msg);
			}
				break;

			default:
				break;
			}
		}

		if (nums_read == -1) {
			APP_DBG("[ERR] read\n");
		}

		if (close(cfd) == -1) {
			APP_DBG("[ERR] close\n");
		}

		usleep(10000);
	}
}

bool is_if_app_id_existed(uint32_t id) {
	for (uint32_t i = 0; i < if_app_list_size; i++) {
		if (id == if_app_list[i].id) {
			return true;
		}
	}
	return false;
}

int send_message_to_app(uint32_t if_des_type, uint8_t* data, uint32_t len) {
	struct sockaddr_un addr;
	int sfd;

	if (!is_if_app_id_existed(if_des_type)) {
		APP_DBG("[ERR] send_message_to_app if_des_type does not supported\n");
		return -1;
	}

	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd < 0) {
		APP_DBG("[ERR] send_message_to_app socket\n");
		return -1;
	}

	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;

	strncpy(addr.sun_path, get_if_socket_path(if_des_type), sizeof(addr.sun_path) - 1);
	if (connect(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
		APP_DBG("[ERR] send_message_to_app connect\n");
		return -1;
	}

	if (write(sfd, data, len) != (ssize_t)len) {
		APP_DBG("[ERR] send_message_to_app write\n");
		return -1;
	}

	close(sfd);

	return 1;
}

char* get_if_socket_path(uint32_t id) {
	for (uint32_t i = 0; i < if_app_list_size; i++) {
		if (id == if_app_list[i].id) {
			return if_app_list[i].socket_path;
		}
	}
	return (char*)0;
}
