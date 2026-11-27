#include <ufsrvclilib/unfacd_service/state_sync.h>

#include "state_sync_priv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void
sCopyStr(char *dst, size_t cap, const char *src)
{
  if (!cap) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  snprintf(dst, cap, "%s", src);
}

const char *
UnfacdStateSyncOutcomeName(UnfacdStateSyncOutcome outcome)
{
  switch (outcome) {
    case UNFACD_OUTCOME_UNDEFINED: return "UNDEFINED";
    case UNFACD_OUTCOME_INVITED: return "INVITED";
    case UNFACD_OUTCOME_ACCEPTED: return "ACCEPTED";
    case UNFACD_OUTCOME_DENIED: return "DENIED";
    case UNFACD_OUTCOME_DEFERRED: return "DEFERRED";
    case UNFACD_OUTCOME_GEO_BASED: return "GEO_BASED";
    case UNFACD_OUTCOME_REJECTED: return "REJECTED";
    case UNFACD_OUTCOME_CREATED: return "CREATED";
    case UNFACD_OUTCOME_DELETED: return "DELETED";
    case UNFACD_OUTCOME_UPDATED: return "UPDATED";
    case UNFACD_OUTCOME_ADDED: return "ADDED";
    case UNFACD_OUTCOME_INVITED_GEO: return "INVITED_GEO";
    case UNFACD_OUTCOME_POSTED: return "POSTED";
    case UNFACD_OUTCOME_SET: return "SET";
    case UNFACD_OUTCOME_UNSET: return "UNSET";
    case UNFACD_OUTCOME_ACCEPTED_PARTIAL: return "ACCEPTED_PARTIAL";
    case UNFACD_OUTCOME_ACCEPTED_INVITE: return "ACCEPTED_INVITE";
    case UNFACD_OUTCOME_UNINVITED: return "UNINVITED";
    case UNFACD_OUTCOME_EXPIRED: return "EXPIRED";
    case UNFACD_OUTCOME_CEASED: return "CEASED";
    case UNFACD_OUTCOME_SHARED: return "SHARED";
    case UNFACD_OUTCOME_UNSHARED: return "UNSHARED";
    case UNFACD_OUTCOME_RESYNC: return "RESYNC";
    case UNFACD_OUTCOME_UNCHANGED: return "UNCHANGED";
    case UNFACD_OUTCOME_REJECTED_USER: return "REJECTED_USER";
    case UNFACD_OUTCOME_SYNCED: return "SYNCED";
    case UNFACD_OUTCOME_LINK_BASED: return "LINK_BASED";
    default: return "UNKNOWN";
  }
}

UnfacdStateSyncGroup *
UnfacdStateSyncGroupCreate(void)
{
  return calloc(1, sizeof(UnfacdStateSyncGroup));
}

void
UnfacdStateSyncGroupFree(UnfacdStateSyncGroup *group_sync)
{
  if (!group_sync) return;

  if (group_sync->rx.runner && !group_sync->rx.resolved && group_sync->rx.promise) {
    UfsrvPromiseSetError(group_sync->rx.promise, ECANCELED);
    UfsrvCoroutineResume(group_sync->rx.runner);
    group_sync->rx.runner = NULL;
  }
  free(group_sync->rx.request_body);
  if (group_sync->rx.promise) UfsrvPromiseDestroy(group_sync->rx.promise);
  if (group_sync->rx.delivery) UfsrvFutureRelease(group_sync->rx.delivery);
  free(group_sync);
}
