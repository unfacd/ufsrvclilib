#include "unfacd_service/command/unfacd_command_utils.h"
/**
 * @file state_sync_message.c
 * @brief Lifecycle of an UnfacdStateSyncMessage.
 */

#include <ufsrvclilib/unfacd_service/state_sync_message.h>

#include <stdlib.h>
#include <errno.h>

UnfacdStateSyncMessage *
UnfacdStateSyncMessageCreate(void)
{
  return calloc(1, sizeof(UnfacdStateSyncMessage));
}

void
UnfacdStateSyncMessageFree(UnfacdStateSyncMessage *state_sync_message)
{
  if (!state_sync_message) return;

  if (state_sync_message->rx.runner && !state_sync_message->rx.resolved && state_sync_message->rx.promise) {
    UfsrvPromiseSetError(state_sync_message->rx.promise, ECANCELED);
    UfsrvCoroutineResume(state_sync_message->rx.runner);
    state_sync_message->rx.runner = NULL;
  }
  free(state_sync_message->rx.request_body);
  if (state_sync_message->rx.promise) UfsrvPromiseDestroy(state_sync_message->rx.promise);
  if (state_sync_message->rx.delivery) UfsrvFutureRelease(state_sync_message->rx.delivery);
  free(state_sync_message);
}
