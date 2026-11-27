/**
 * @file state_sync_message.h
 * @brief Lifecycle of an UnfacdStateSyncMessage.
 */

#ifndef UF_UNFACD_STATE_SYNC_MESSAGE_H_API
#define UF_UNFACD_STATE_SYNC_MESSAGE_H_API

#include <uflib/uflib_defs.h>

#include <ufsrvclilib/unfacd_service/unfacd_state_sync_message_type.h>

PUBLIC_API UnfacdStateSyncMessage *UnfacdStateSyncMessageCreate(void);
PUBLIC_API void                    UnfacdStateSyncMessageFree(UnfacdStateSyncMessage *state_sync_message);

#endif
