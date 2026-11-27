/**
 * @file state_sync_rx_context_type.h
 * @brief The coroutine/future state shared by the Rx variants of the group commands.
 */

#ifndef UFSRVCLI_UNFACD_STATE_SYNC_RX_CONTEXT_H
#define UFSRVCLI_UNFACD_STATE_SYNC_RX_CONTEXT_H

#include <ufsrvrxlib/ufsrv_coroutine/ufsrv_coroutine.h>
#include <ufsrvrxlib/ufsrv_future/ufsrv_future.h>

typedef struct UnfacdStateSyncRxContext
{
  char            *request_body;     /*!< built wire body the consumer sends; owned here. */
  UfsrvPromise    *promise;          /*!< completes the delivery future; owned here. */
  UfsrvFuture     *delivery;         /*!< future the runner awaits; owned by the promise. */
  UfsrvCoroutine  *runner;           /*!< coroutine parked on the delivery future; owned here. */
  int              resolved;         /*!< 1 once delivered and filled, -1 when the future failed. */
} UnfacdStateSyncRxContext;

#endif //UFSRVCLI_UNFACD_STATE_SYNC_RX_CONTEXT_H
