#ifndef UF_UNFACD_STATE_SYNC_H
#define UF_UNFACD_STATE_SYNC_H

#include <uflib/uflib_defs.h>

#include <ufsrvclilib/unfacd_service/unfacd_state_sync_group_type.h>

PUBLIC_API const char *UnfacdStateSyncOutcomeName(UnfacdStateSyncOutcome outcome);
PUBLIC_API UnfacdStateSyncGroup *UnfacdStateSyncGroupCreate(void);
PUBLIC_API void                  UnfacdStateSyncGroupFree(UnfacdStateSyncGroup *group_sync);

#endif
