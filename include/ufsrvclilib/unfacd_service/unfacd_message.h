#ifndef UF_UNFACD_MESSAGE_H
#define UF_UNFACD_MESSAGE_H

#include <uflib/uflib_defs.h>

#include <ufsrvclilib/unfacd_service/unfacd_message_type.h>
#include <ufsrvclilib/unfacd_service/command/unfacd_command_message_type.h>
#include <ufsrvclilib/unfacd_service/unfacd_state_sync_message_type.h>

PUBLIC_API void UnfacdMessageClear(UnfacdMessage *message);
PUBLIC_API int UnfacdMessageCopy(UnfacdMessage *destination, const UnfacdMessage *source);
PUBLIC_API int UnfacdMessageOutcomeIsSuccess(UnfacdMessageOperation operation, UnfacdStateSyncOutcome outcome);
PUBLIC_API int UnfacdMessageOutcomeIsRefusal(UnfacdMessageOperation operation, UnfacdStateSyncOutcome outcome);

PUBLIC_API char *UnfacdMessageGroup(const UnfacdMessage *message);

PUBLIC_API char *UnfacdGroupMessageSync(const UnfacdMessage *message);

PUBLIC_API UnfacdStateSyncMessage *UnfacdGroupMessageRx(UnfacdMessage *message,
                                                        UnfacdStateSyncMessage *state_sync_message);

PUBLIC_API int UnfacdGroupMessageRxDeliver(UnfacdStateSyncMessage *state_sync_message,
                                           const UnfacdCommandMessage *command);

#endif
