#ifndef __LINK_SIG_H__
#define __LINK_SIG_H__

#include "ak.h"

/*****************************************************************************/
/*  LINK_PHY task define
 */
/*****************************************************************************/
/* private define */
/* define timer */
/* define signal */
enum {
	/* public */
	GW_LINK_PHY_INIT = AK_USER_DEFINE_SIG,
	GW_LINK_PHY_FRAME_SEND_REQ,

	/* private */
	GW_LINK_PHY_FRAME_SEND_TO,
	GW_LINK_PHY_FRAME_REV,
	GW_LINK_PHY_FRAME_REV_TO,
	GW_LINK_PHY_FRAME_REV_CS_ERR,
};

/*****************************************************************************/
/*  LINK_MAC task define
 */
/*****************************************************************************/
/* private define */
/* define timer */
/* define signal */
enum {
	/* public */
	GW_LINK_MAC_INIT = AK_USER_DEFINE_SIG,
	GW_LINK_MAC_PHY_LAYER_STARTED,
	GW_LINK_MAC_FRAME_SEND_REQ,

	/* private */
	GW_LINK_MAC_FRAME_SEND_START,
	GW_LINK_MAC_FRAME_SEND_DONE,
	GW_LINK_MAC_FRAME_SEND_TO,
	GW_LINK_MAC_FRAME_SEND_ERR,
	GW_LINK_MAC_FRAME_REV,
	GW_LINK_MAC_FRAME_REV_TO,
};

/*****************************************************************************/
/*  LINK task define
 */
/*****************************************************************************/
/* private define */
/* define timer */
/* define signal */
enum {
	/* public */
	GW_LINK_INIT = AK_USER_DEFINE_SIG,
	GW_LINK_MAC_LAYER_STARTED,
	GW_LINK_SEND_PURE_MSG,
	GW_LINK_SEND_COMMON_MSG,
	GW_LINK_SEND_DYNAMIC_MSG,
	GW_LINK_SEND_DATA,

	/* private */
	GW_LINK_SEND_DONE,
	GW_LINK_SEND_ERR,

	GW_LINK_REV_MSG,
};

#endif //__LINK_SIG_H__
