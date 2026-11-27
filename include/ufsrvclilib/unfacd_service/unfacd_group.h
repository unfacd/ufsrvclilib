#ifndef UF_UNFACD_GROUP_H
#define UF_UNFACD_GROUP_H

#include <uflib/uflib_defs.h>

#include <ufsrvclilib/unfacd_service/unfacd_group_type.h>
#include <ufsrvclilib/unfacd_service/state_sync.h>
#include <ufsrvclilib/unfacd_service/command/unfacd_command_fence_type.h>

PUBLIC_API int UnfacdFenceOutcomeIsSuccess(UnfacdFenceOperation operation, UnfacdStateSyncOutcome outcome);
PUBLIC_API int UnfacdFenceOutcomeIsRefusal(UnfacdFenceOperation operation, UnfacdStateSyncOutcome outcome);

PUBLIC_API void UnfacdGroupClear(UnfacdGroup *group);
PUBLIC_API int UnfacdGroupCopy(UnfacdGroup *destination, const UnfacdGroup *source);

/**
 * @brief Join (or create) a fence synchronously.
 *
 * Builds a FenceCommand(JOIN) for @p group, POSTs it to
 * `<context base>/V1/Fence` and returns the REST reply. The endpoint and
 * credentials come from the context singleton; the caller must have installed
 * one with UfsrvCliContextProvider().
 *
 * The reply is a receipt (`{"needsSync": false}`), not the fence: the group
 * itself arrives later out-of-band on the websocket, which this call does not watch.
 *
 * @param group fence to join or create; its fname (or fid) identifies it.
 *
 * @return the reply body, owned by the caller and released with free(), or
 *         NULL when the context is missing or the POST is refused.
 */
PUBLIC_API char *UnfacdGroupJoinSync(const UnfacdGroup *group);

/**
 * @brief Begin a join and return the container that will carry the result.
 *
 * Builds the protocol request into `state_sync_group->rx.request_body` (the caller
 * POSTs it), creates the linked promise/future and parks a coroutine on that
 * future. Returns immediately.
 *
 * @param group             fence to join; filled with what the server sends.
 * @param state_sync_group  caller-owned container, or NULL to allocate one.
 *
 * @return the container, or NULL when the request could not be built. Release
 *         it with UnfacdStateSyncGroupFree().
 */
PUBLIC_API UnfacdStateSyncGroup *UnfacdGroupJoinRx(UnfacdGroup *group, UnfacdStateSyncGroup *state_sync_group);

PUBLIC_API int UnfacdGroupRxDeliver(UnfacdStateSyncGroup *state_sync_group,
                                        const UnfacdCommandFence *command);

/**
 * @brief Leave a fence synchronously.
 *
 * Same shape as UnfacdGroupJoinSync(): builds FenceCommand(LEAVE) and POSTs it
 * to `<context base>/V1/Fence`.
 *
 * @param group fence to leave; its fname (or fid) identifies it.
 *
 * @return the reply body (caller frees), or NULL on refusal.
 */
PUBLIC_API char *UnfacdGroupLeaveSync(const UnfacdGroup *group);

/**
 * @brief Begin a leave and return the container that will carry the result.
 *
 * @param group             fence to leave; filled from the server's reply.
 * @param state_sync_group  caller-owned container, or NULL to allocate one.
 *
 * @return the container, or NULL when the request could not be built.
 */
PUBLIC_API UnfacdStateSyncGroup *UnfacdGroupLeaveRx(UnfacdGroup *group,
                                                   UnfacdStateSyncGroup *state_sync_group);

/**
 * @brief Rename a fence synchronously.
 *
 * Builds FenceCommand(FNAME, UPDATED) from @p group (its fid identifies the fence, its fname is
 * the new title) and POSTs it to `<context base>/V1/Fence`. The reply is only a receipt: the
 * rename is fanned out to the members, this client included, on the websocket.
 *
 * @param group       fence to rename; its fname is the new title.
 * @param when_client stamp of the local request this rename answers for. The server echoes it on
 *                    the fan-out, which lets the caller retire its pending "requesting name"
 *                    state. Zero leaves the field unset.
 *
 * @return the reply body (caller frees), or NULL on refusal.
 */
PUBLIC_API char *UnfacdGroupFnameSync(const UnfacdGroup *group, uint64_t when_client);

/**
 * @brief Begin a rename and return the container that will carry the result.
 *
 * Same shape as UnfacdGroupJoinRx(): builds the request into
 * `state_sync_group->rx.request_body` for the caller to POST, and parks the promise/coroutine
 * that the FNAME fan-out resolves through UnfacdGroupRxDeliver().
 *
 * @param group             fence to rename; its fname is the new title.
 * @param when_client       stamp of the local request, echoed back on the fan-out. Zero leaves
 *                          the field unset.
 * @param state_sync_group  caller-owned container, or NULL to allocate one.
 *
 * @return the container, or NULL when the request could not be built.
 */
PUBLIC_API UnfacdStateSyncGroup *UnfacdGroupFnameRx(UnfacdGroup *group, uint64_t when_client,
                                                    UnfacdStateSyncGroup *state_sync_group);

#endif
