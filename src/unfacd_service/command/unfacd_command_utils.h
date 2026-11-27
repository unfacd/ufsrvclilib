#ifndef UFSRVCLILIB_UNFACD_COMMAND_UTILS_H
#define UFSRVCLILIB_UNFACD_COMMAND_UTILS_H

#include <ufsrvclilib/unfacd_service/command/unfacd_command_fence_type.h>
#include <ufsrvclilib/unfacd_service/command/unfacd_command_message_type.h>
#include <unfacd_service/UnfacdService.pb-c.h>

int UnfacdCommandFillHeader(UnfacdCommandHeader *header, const CommandHeader *wire);
int UnfacdCommandFillFenceGroup(UnfacdGroup *group, const FenceRecord *wire);

int UnfacdCommandFillMessageRecord(UnfacdMessage *message, const MessageRecord *wire, uint64_t fid, const UnfacdUser *from);
int UnfacdCommandFillUser(UnfacdUser *user, const UserRecord *wire);

#endif
