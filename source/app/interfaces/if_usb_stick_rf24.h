#ifndef __IF_USB_STICK_RF24_H__
#define __IF_USB_STICK_RF24_H__

#include <stdint.h>

#include "message.h"

#include "app.h"
#include "app_data.h"
#include "app_dbg.h"

extern q_msg_t gw_task_if_usb_stick_rf24_mailbox;
extern void* gw_task_if_usb_stick_rf24_entry(void*);

#endif //__IF_USB_STICK_RF24_H__
