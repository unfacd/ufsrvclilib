/**
 * @file unfacd_state_sync_message_type.h
 * @brief StateSyncMessage: the Rx context plus the message it is servicing.
 */

#ifndef UFSRVCLI_UNFACD_STATE_SYNC_MESSAGE_H
#define UFSRVCLI_UNFACD_STATE_SYNC_MESSAGE_H

#include "state_sync_rx_context_type.h"
#include "unfacd_message_type.h"
#include "command/unfacd_command_header_type.h"

typedef struct UnfacdStateSyncMessage
{
  UnfacdCommandHeader      state_sync_header; /*!< base: header of the command received. */
  UnfacdStateSyncRxContext rx;                /*!< shared coroutine/future state for this request. */
  uint64_t request_when;
  uint64_t request_fid;
  UnfacdMessage *          message;           /*!< caller's message being serviced. */
} UnfacdStateSyncMessage;

#endif //UFSRVCLI_UNFACD_STATE_SYNC_MESSAGE_H
