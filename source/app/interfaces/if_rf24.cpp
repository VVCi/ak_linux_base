#if (RF24==1)
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ak.h"

#include "fifo.h"

#include "exor.h"

#include "app.h"
#include "app_data.h"
#include "app_dbg.h"

#include "if_rf24.h"
#include "task_list.h"
#include "task_if.h"

#define SEND_FIFO_BUFFER_SIZE			64

const uint16_t getway_node_address =	00;

uint16_t if_rf24_table[IF_TYPE_RF24_MAX] {
	00,	/* rf24 GW module address */
	01,	/* rf24 AC module address */
	02,	/* rf24 WR module address */
	03	/* rf24 DR module address */
};

q_msg_t gw_task_if_rf24_mailbox;

RF24 radio(RPI_V2_GPIO_P1_15, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_8MHZ);

RF24Network network(radio);

static uint8_t rf24_buffer[RF24_BUFFER_SIZE];

static uint8_t rf24_encypt_decrypt_buffer[RF24_BUFFER_SIZE];

static ak_msg_common_if_t send_common_fifo_buffer[SEND_FIFO_BUFFER_SIZE];
static fifo_t send_common_fifo;

static ak_msg_pure_if_t send_pure_fifo_buffer[SEND_FIFO_BUFFER_SIZE];
static fifo_t send_pure_fifo;

static uint8_t rf_pending_flag = APP_FLAG_OFF;

void* gw_task_if_rf24_entry(void*) {
	radio.begin();

	delay(5);

	network.begin(90, getway_node_address);
	radio.printDetails();

	fifo_init(&send_common_fifo, send_common_fifo_buffer, SEND_FIFO_BUFFER_SIZE, sizeof(ak_msg_common_if_t));
	fifo_init(&send_pure_fifo, send_pure_fifo_buffer, SEND_FIFO_BUFFER_SIZE, sizeof(ak_msg_pure_if_t));

	wait_all_tasks_started();

	APP_DBG("[STARTED] gw_task_if_rf24_entry\n");

	while (1) {
		network.update();

		/* check receive rf24 message */
		while(network.available()) {
			RF24NetworkHeader rf24_header;
			memset(rf24_buffer, 0, RF24_BUFFER_SIZE);

			network.read(rf24_header, rf24_buffer, RF24_BUFFER_SIZE);

			switch (rf24_header.type) {
			case RF24_DATA_COMMON_MSG_TYPE: {
				rf_pending_flag = APP_FLAG_ON;

				rf24_encrypt_decrypt_key[0] = (uint8_t)(rf24_header.id % 0xFF);
				rf24_encrypt_decrypt_key[1] = (uint8_t)((rf24_header.id >> 8) % 0xFF);
				rf24_encrypt_decrypt_key[2] = rf24_header.type;
				exor_encrypt_decrypt(rf24_buffer, rf24_encypt_decrypt_buffer, sizeof(ak_msg_common_if_t), rf24_encrypt_decrypt_key, RF24_ENCRYPT_DECRYPT_KEY_SIZE);

				ak_msg_if_header_t* if_msg_header = (ak_msg_if_header_t*)rf24_encypt_decrypt_buffer;

				ak_msg_t* s_msg = get_common_msg();

				set_if_src_task_id(s_msg, if_msg_header->src_task_id);
				set_if_des_task_id(s_msg, if_msg_header->des_task_id);
				set_if_src_type(s_msg, if_msg_header->if_src_type);
				set_if_des_type(s_msg, if_msg_header->if_des_type);
				set_if_sig(s_msg, if_msg_header->sig);
				set_if_data_common_msg(s_msg, ((ak_msg_common_if_t*)if_msg_header)->data, ((ak_msg_common_if_t*)if_msg_header)->len);

				set_msg_sig(s_msg, GW_IF_COMMON_MSG_IN);
				set_msg_src_task_id(s_msg, GW_TASK_IF_RF24_ID);
				task_post(GW_TASK_IF_ID, s_msg);

				timer_set(GW_TASK_IF_RF24_ID, GW_RF24_IF_TIMER_PACKET_DELAY, GW_RF24_IF_TIMER_PACKET_DELAY_INTERVAL, TIMER_ONE_SHOT);
			}
				break;

			case RF24_DATA_PURE_MSG_TYPE: {
				rf_pending_flag = APP_FLAG_ON;

				rf24_encrypt_decrypt_key[0] = (uint8_t)(rf24_header.id % 0xFF);
				rf24_encrypt_decrypt_key[1] = (uint8_t)((rf24_header.id >> 8) % 0xFF);
				rf24_encrypt_decrypt_key[2] = rf24_header.type;
				exor_encrypt_decrypt(rf24_buffer, rf24_encypt_decrypt_buffer, sizeof(ak_msg_pure_if_t), rf24_encrypt_decrypt_key, RF24_ENCRYPT_DECRYPT_KEY_SIZE);\

				ak_msg_if_header_t* if_msg_header = (ak_msg_if_header_t*)rf24_encypt_decrypt_buffer;

				ak_msg_t* s_msg = get_pure_msg();

				set_if_src_task_id(s_msg, if_msg_header->src_task_id);
				set_if_des_task_id(s_msg, if_msg_header->des_task_id);
				set_if_src_type(s_msg, if_msg_header->if_src_type);
				set_if_des_type(s_msg, if_msg_header->if_des_type);
				set_if_sig(s_msg, if_msg_header->sig);

				set_msg_sig(s_msg, GW_IF_PURE_MSG_IN);
				set_msg_src_task_id(s_msg, GW_TASK_IF_RF24_ID);
				task_post(GW_TASK_IF_ID, s_msg);

				timer_set(GW_TASK_IF_RF24_ID, GW_RF24_IF_TIMER_PACKET_DELAY, GW_RF24_IF_TIMER_PACKET_DELAY_INTERVAL, TIMER_ONE_SHOT);
			}
				break;

			case RF24_DATA_REMOTE_CMD_TYPE: {
				uint16_t st_if_des_type_rf24 = 0;
				for (int i = 0; i < IF_TYPE_RF24_MAX; i++) {
					if (if_rf24_table[i] == rf24_header.from_node) {
						st_if_des_type_rf24 = i;
						break;
					}
				}

				if (st_if_des_type_rf24 == IF_TYPE_RF24_AC) {
					uint32_t str_len = strlen((const char*)rf24_buffer);
					ak_msg_t* s_msg = get_dynamic_msg();
					set_msg_sig(s_msg, GW_CONSOLE_AC_LOGIN_CMD);
					set_data_dynamic_msg(s_msg, rf24_buffer, str_len);
					set_msg_src_task_id(s_msg, GW_TASK_IF_RF24_ID);
					task_post(GW_TASK_CONSOLE_ID, s_msg);
				}
			}
				break;

			default:
				break;
			}
		}

		while (msg_available(GW_TASK_IF_RF24_ID)) {
			/* get messge */
			ak_msg_t* msg = ak_msg_rev(GW_TASK_IF_RF24_ID);

			/* handler message */
			switch (msg->header->sig) {
			case GW_RF24_IF_PURE_MSG_OUT: {
				ak_msg_pure_if_t if_msg;
				memset(&if_msg, 0, sizeof(ak_msg_pure_if_t));

				if_msg.header.src_task_id = msg->header->if_src_task_id;
				if_msg.header.des_task_id = msg->header->if_des_task_id;
				if_msg.header.if_src_type = IF_TYPE_RF24_GW;
				if_msg.header.if_des_type = msg->header->if_des_type;
				if_msg.header.sig = msg->header->if_sig;

				if (rf_pending_flag == APP_FLAG_OFF) {
					RF24NetworkHeader send_header(if_rf24_table[msg->header->if_des_type], RF24_DATA_PURE_MSG_TYPE);

					rf24_encrypt_decrypt_key[0] = (uint8_t)(send_header.id % 0xFF);
					rf24_encrypt_decrypt_key[1] = (uint8_t)((send_header.id >> 8) % 0xFF);
					rf24_encrypt_decrypt_key[2] = send_header.type;
					exor_encrypt_decrypt((uint8_t*)&if_msg, rf24_encypt_decrypt_buffer, sizeof(ak_msg_pure_if_t), rf24_encrypt_decrypt_key, RF24_ENCRYPT_DECRYPT_KEY_SIZE);\

					network.write(send_header, rf24_encypt_decrypt_buffer, sizeof(ak_msg_pure_if_t));
				}
				else {
					fifo_put(&send_pure_fifo, &if_msg);
				}
			}
				break;

			case GW_RF24_IF_COMMON_MSG_OUT: {
				ak_msg_common_if_t if_msg;
				memset(&if_msg, 0, sizeof(ak_msg_common_if_t));

				if_msg.header.src_task_id = msg->header->if_src_task_id;
				if_msg.header.des_task_id = msg->header->if_des_task_id;
				if_msg.header.if_src_type = IF_TYPE_RF24_GW;
				if_msg.header.if_des_type = msg->header->if_des_type;
				if_msg.header.sig = msg->header->if_sig;

				if_msg.len = get_data_len_common_msg(msg);
				get_data_common_msg(msg, if_msg.data, if_msg.len);

				if (rf_pending_flag == APP_FLAG_OFF) {
					RF24NetworkHeader send_header(if_rf24_table[msg->header->if_des_type], RF24_DATA_COMMON_MSG_TYPE);

					rf24_encrypt_decrypt_key[0] = (uint8_t)(send_header.id % 0xFF);
					rf24_encrypt_decrypt_key[1] = (uint8_t)((send_header.id >> 8) % 0xFF);
					rf24_encrypt_decrypt_key[2] = send_header.type;
					exor_encrypt_decrypt((uint8_t*)&if_msg, rf24_encypt_decrypt_buffer, sizeof(ak_msg_common_if_t), rf24_encrypt_decrypt_key, RF24_ENCRYPT_DECRYPT_KEY_SIZE);\

					network.write(send_header, rf24_encypt_decrypt_buffer, sizeof(ak_msg_common_if_t));
				}
				else {
					fifo_put(&send_common_fifo, &if_msg);
				}
			}
				break;

			case GW_RF24_IF_TIMER_PACKET_DELAY: {
				/* clear pending flag */
				rf_pending_flag = APP_FLAG_OFF;

				/* TODO: check data queue */
				if (!fifo_is_empty(&send_pure_fifo)) {
					ak_msg_pure_if_t if_msg;
					fifo_get(&send_pure_fifo, &if_msg);

					RF24NetworkHeader send_header(if_rf24_table[if_msg.header.if_des_type], RF24_DATA_PURE_MSG_TYPE);

					rf24_encrypt_decrypt_key[0] = (uint8_t)(send_header.id % 0xFF);
					rf24_encrypt_decrypt_key[1] = (uint8_t)((send_header.id >> 8) % 0xFF);
					rf24_encrypt_decrypt_key[2] = send_header.type;
					exor_encrypt_decrypt((uint8_t*)&if_msg, rf24_encypt_decrypt_buffer, sizeof(ak_msg_pure_if_t), rf24_encrypt_decrypt_key, RF24_ENCRYPT_DECRYPT_KEY_SIZE);\

					network.write(send_header, rf24_encypt_decrypt_buffer, sizeof(ak_msg_pure_if_t));

					rf_pending_flag = APP_FLAG_ON;
					timer_set(GW_TASK_IF_RF24_ID, GW_RF24_IF_TIMER_PACKET_DELAY, GW_RF24_IF_TIMER_PACKET_DELAY_INTERVAL, TIMER_ONE_SHOT);
				}
				else if (!fifo_is_empty(&send_common_fifo)) {
					ak_msg_common_if_t if_msg;
					fifo_get(&send_common_fifo, &if_msg);

					RF24NetworkHeader send_header(if_rf24_table[if_msg.header.if_des_type], RF24_DATA_COMMON_MSG_TYPE);

					rf24_encrypt_decrypt_key[0] = (uint8_t)(send_header.id % 0xFF);
					rf24_encrypt_decrypt_key[1] = (uint8_t)((send_header.id >> 8) % 0xFF);
					rf24_encrypt_decrypt_key[2] = send_header.type;
					exor_encrypt_decrypt((uint8_t*)&if_msg, rf24_encypt_decrypt_buffer, sizeof(ak_msg_common_if_t), rf24_encrypt_decrypt_key, RF24_ENCRYPT_DECRYPT_KEY_SIZE);\

					network.write(send_header, rf24_encypt_decrypt_buffer, sizeof(ak_msg_common_if_t));

					rf_pending_flag = APP_FLAG_ON;
					timer_set(GW_TASK_IF_RF24_ID, GW_RF24_IF_TIMER_PACKET_DELAY, GW_RF24_IF_TIMER_PACKET_DELAY_INTERVAL, TIMER_ONE_SHOT);
				}
			}
				break;

			default:
				break;
			}

			/* free message */
			ak_msg_free(msg);
		}

		usleep(100);
	}

	return (void*)0;
}

#endif
