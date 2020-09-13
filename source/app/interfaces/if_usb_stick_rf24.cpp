#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <semaphore.h>

#include "ak.h"

#include "sys_dbg.h"

#include "app.h"
#include "app_if.h"
#include "app_dbg.h"
#include "app_data.h"

#include "task_list.h"
#include "task_list_if.h"
#include "if_usb_stick_rf24.h"

#define IPCPU_SOP_CHAR		0xEF
#define IFCPU_DEVPATH		"/dev/usb_stick_rf24"
#define IFCPU_DATA_SIZE		254

#define RX_BUFFER_SIZE		4096

typedef struct {
	uint8_t frame_sop;
	uint32_t len;
	uint8_t data_index;
	uint8_t data[IFCPU_DATA_SIZE];
	uint8_t frame_fcs;
} if_usb_stick_rf24_frame_t;

q_msg_t gw_task_if_usb_stick_rf24_mailbox;

static int if_usb_stick_rf24_fd;

static int if_usb_stick_rf24_opentty(const char* devpath);
static uint8_t if_usb_stick_rf24_calcfcs(uint8_t len, uint8_t *data_ptr);

static pthread_t if_usb_stick_rf24_rx_thread;
static void* if_usb_stick_rf24_rx_thread_handler(void*);

#define SOP_STATE		0x00
#define LEN_STATE		0x01
#define DATA_STATE		0x02
#define FCS_STATE		0x03
static uint8_t rx_frame_state = SOP_STATE;

#define RX_FRAME_PARSER_FAILED		(-1)
#define RX_FRAME_PARSER_SUCCESS		(0)
#define RX_FRAME_PARSER_rx_remain		(1)

static if_usb_stick_rf24_frame_t if_usb_stick_rf24_frame;
static void rx_frame_parser(uint8_t* data, uint8_t len);
int usb_stick_rf24_tx_frame_post(uint8_t* data, uint8_t len);

static uint8_t tx_buffer[1024];

void* gw_task_if_usb_stick_rf24_entry(void*) {
	ak_msg_t* msg = AK_MSG_NULL;

	wait_all_tasks_started();

	APP_DBG("[STARTED] gw_task_if_usb_stick_rf24_entry\n");

	if (if_usb_stick_rf24_opentty(IFCPU_DEVPATH) < 0) {
		APP_DBG("Cannot open %s !\n", IFCPU_DEVPATH);
	}
	else {
		APP_DBG("Opened %s success !\n", IFCPU_DEVPATH);\
		pthread_create(&if_usb_stick_rf24_rx_thread, NULL, if_usb_stick_rf24_rx_thread_handler, NULL);
	}

	while (1) {
		/* get messge */
		msg = ak_msg_rev(GW_TASK_IF_USB_STICK_RF24_ID);

		switch (get_msg_type(msg)) {
		case PURE_MSG_TYPE: {
			APP_DBG("[IF USB STICK RF24][SEND] PURE_MSG_TYPE\n");
			ak_msg_pure_if_t app_if_msg;

			/* assign if message */
			app_if_msg.header.type			= PURE_MSG_TYPE;
			app_if_msg.header.if_src_type	= msg->header->if_src_type;
			app_if_msg.header.if_des_type	= msg->header->if_des_type;
			app_if_msg.header.sig			= msg->header->if_sig;
			app_if_msg.header.src_task_id	= msg->header->if_src_task_id;
			app_if_msg.header.des_task_id	= msg->header->if_des_task_id;

			usb_stick_rf24_tx_frame_post((uint8_t*)&app_if_msg, sizeof(ak_msg_pure_if_t));
		}
			break;

		case COMMON_MSG_TYPE: {
			APP_DBG("[IF USB STICK RF24][SEND] COMMON_MSG_TYPE\n");
			ak_msg_common_if_t app_if_msg;

			/* assign if message */
			app_if_msg.header.type			= COMMON_MSG_TYPE;
			app_if_msg.header.if_src_type	= msg->header->if_src_type;
			app_if_msg.header.if_des_type	= msg->header->if_des_type;
			app_if_msg.header.sig			= msg->header->if_sig;
			app_if_msg.header.src_task_id	= msg->header->if_src_task_id;
			app_if_msg.header.des_task_id	= msg->header->if_des_task_id;

			app_if_msg.len = msg->header->len;
			get_data_common_msg(msg, app_if_msg.data, msg->header->len);

			usb_stick_rf24_tx_frame_post((uint8_t*)&app_if_msg, sizeof(ak_msg_common_if_t));
		}
			break;

		case DYNAMIC_MSG_TYPE: {
			APP_DBG("[IF USB STICK RF24][SEND] COMMON_MSG_TYPE\n");
			ak_msg_dynamic_if_t app_if_msg;
			uint32_t app_if_msg_len;

			/* assign if message */
			app_if_msg.header.type			= DYNAMIC_MSG_TYPE;
			app_if_msg.header.if_src_type	= msg->header->if_src_type;
			app_if_msg.header.if_des_type	= msg->header->if_des_type;
			app_if_msg.header.sig			= msg->header->if_sig;
			app_if_msg.header.src_task_id	= msg->header->if_src_task_id;
			app_if_msg.header.des_task_id	= msg->header->if_des_task_id;

			app_if_msg.len = msg->header->len;
			app_if_msg.data = (uint8_t*)malloc(app_if_msg.len);
			get_data_dynamic_msg(msg, app_if_msg.data, app_if_msg.len);

			app_if_msg_len = sizeof(ak_msg_if_header_t) + sizeof(uint32_t) + app_if_msg.len;

			usb_stick_rf24_tx_frame_post((uint8_t*)&app_if_msg, app_if_msg_len);
			free(app_if_msg.data);
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

void* if_usb_stick_rf24_rx_thread_handler(void*) {
	APP_DBG("if_usb_stick_rf24_rx_thread_handler entry successed!\n");
	uint8_t rx_buffer[RX_BUFFER_SIZE];
	uint32_t rx_read_len;

	while(1) {
		rx_read_len = read(if_usb_stick_rf24_fd, rx_buffer, RX_BUFFER_SIZE);
		if (rx_read_len > 0) {
			rx_frame_parser(rx_buffer, rx_read_len);
		}
		usleep(1);
	}

	return (void*)0;
}

int if_usb_stick_rf24_opentty(const char* devpath) {
	struct termios options;
	APP_DBG("[IF USB STICK RF24][if_usb_stick_rf24_opentty] devpath: %s\n", devpath);

	if_usb_stick_rf24_fd = open(devpath, O_RDWR | O_NOCTTY | O_NDELAY);
	if (if_usb_stick_rf24_fd < 0) {
		return if_usb_stick_rf24_fd;
	}
	else {
		fcntl(if_usb_stick_rf24_fd, F_SETFL, 0);

		/* get current status */
		tcgetattr(if_usb_stick_rf24_fd, &options);

		cfsetispeed(&options, B115200);
		cfsetospeed(&options, B115200);

		/* No parity (8N1) */
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS8;

		options.c_cflag |= (CLOCAL | CREAD);
		options.c_cflag     &=  ~CRTSCTS;

		cfmakeraw(&options);

		tcflush(if_usb_stick_rf24_fd, TCIFLUSH);
		if (tcsetattr (if_usb_stick_rf24_fd, TCSANOW, &options) != 0) {
			SYS_DBG("error in tcsetattr()\n");
		}
	}
	return 0;
}


/* Calculate IF_usb_stick_rf24 frame FCS */
uint8_t if_usb_stick_rf24_calcfcs(uint8_t len, uint8_t *data_ptr) {
	uint8_t xor_result;
	xor_result = len;

	for (int i = 0; i < len; i++, data_ptr++) {
		xor_result = xor_result ^ *data_ptr;
	}

	return xor_result;
}

void rx_frame_parser(uint8_t* data, uint8_t len) {
	uint8_t ch;
	int rx_remain;

	while(len) {

		ch = *data++;
		len--;

		switch (rx_frame_state) {
		case SOP_STATE: {
			if (IPCPU_SOP_CHAR == ch) {
				rx_frame_state = LEN_STATE;
			}
		}
			break;

		case LEN_STATE: {
			if (ch > IFCPU_DATA_SIZE) {
				rx_frame_state = SOP_STATE;
				return;
			}
			else {
				if_usb_stick_rf24_frame.len = ch;
				if_usb_stick_rf24_frame.data_index = 0;
				rx_frame_state = DATA_STATE;
			}
		}
			break;

		case DATA_STATE: {
			if_usb_stick_rf24_frame.data[if_usb_stick_rf24_frame.data_index++] = ch;

			rx_remain = if_usb_stick_rf24_frame.len - if_usb_stick_rf24_frame.data_index;

			if (len >= rx_remain) {
				memcpy((uint8_t*)(if_usb_stick_rf24_frame.data + if_usb_stick_rf24_frame.data_index), data, rx_remain);
				if_usb_stick_rf24_frame.data_index += rx_remain;
				len -= rx_remain;
				data += rx_remain;
			}
			else {
				memcpy((uint8_t*)(if_usb_stick_rf24_frame.data + if_usb_stick_rf24_frame.data_index), data, len);
				if_usb_stick_rf24_frame.data_index += len;
				len = 0;
			}

			if (if_usb_stick_rf24_frame.data_index == if_usb_stick_rf24_frame.len) {
				rx_frame_state = FCS_STATE;
			}
		}
			break;

		case FCS_STATE: {
			rx_frame_state = SOP_STATE;

			if_usb_stick_rf24_frame.frame_fcs = ch;

			if (if_usb_stick_rf24_frame.frame_fcs \
					== if_usb_stick_rf24_calcfcs(if_usb_stick_rf24_frame.len, if_usb_stick_rf24_frame.data)) {

				ak_msg_if_header_t* if_msg_header = (ak_msg_if_header_t*)if_usb_stick_rf24_frame.data;

				switch (if_msg_header->type) {
				case PURE_MSG_TYPE: {
					APP_DBG("[IF USB STICK RF24][REV] PURE_MSG_TYPE\n");
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
					APP_DBG("[IF USB STICK RF24][REV] COMMON_MSG_TYPE\n");
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

				case DYNAMIC_MSG_TYPE: {
					APP_DBG("[IF USB STICK RF24][REV] DYNAMIC_MSG_TYPE\n");
					ak_msg_t* s_msg = get_dynamic_msg();

					set_if_src_task_id(s_msg, if_msg_header->src_task_id);
					set_if_des_task_id(s_msg, if_msg_header->des_task_id);
					set_if_src_type(s_msg, if_msg_header->if_src_type);
					set_if_des_type(s_msg, if_msg_header->if_des_type);
					set_if_sig(s_msg, if_msg_header->sig);
					set_if_data_dynamic_msg(s_msg, ((ak_msg_dynamic_if_t*)if_msg_header)->data, ((ak_msg_dynamic_if_t*)if_msg_header)->len);

					set_msg_sig(s_msg, GW_IF_DYNAMIC_MSG_IN);
					set_msg_src_task_id(s_msg, GW_TASK_IF_APP_ID);
					task_post(GW_TASK_IF_ID, s_msg);
				}
					break;

				default:
					break;
				}
			}
			else {
				/* TODO: handle checksum incorrect */
			}
		}
			break;

		default:
			break;
		}
	}
}

int usb_stick_rf24_tx_frame_post(uint8_t* data, uint8_t len) {
	tx_buffer[0] = IPCPU_SOP_CHAR;
	tx_buffer[1] = len;
	memcpy(&tx_buffer[2], data, len);
	tx_buffer[2 + len] = if_usb_stick_rf24_calcfcs(len, data);
	return write(if_usb_stick_rf24_fd, tx_buffer, (len + 3));
}
