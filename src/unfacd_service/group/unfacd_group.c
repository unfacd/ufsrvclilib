/**
 * @file unfacd_group.c
 * @brief unfacd_group
 *
 * Copyright (C) 2015-2026  unfacd works
 *
 * This file is part of ufsrvcli source code.
 * Created by ayman on 6/11/2026.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */
#include <ufsrvclilib/unfacd_service/unfacd_group.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <openssl/rand.h>

#include <uflib/utils_base64.h>
#include <uflib/utils_time.h>

#include "ufsrvcli_context_priv.h"
#include <ufsrvclilib/unfacd_service/unfacd_rest.h>

#include <unfacd_service/UnfacdService.pb-c.h>

#include <ufsrvrxlib/ufsrv_coroutine/ufsrv_coroutine.h>
#include <ufsrvrxlib/ufsrv_future/ufsrv_future.h>

#include "unfacd_service/state_sync/state_sync_priv.h"

#define UNFACD_GROUP_JSON_OVERHEAD 160u

static void
sCleanupFree(void *p_)
{
  void **p = (void **)p_;

  free(*p);
}

static UserRecord **
sUserRecordsFromCollection(const CollectionDescriptor *cd, UserRecord *records)
{
  UserRecord **ptrs;

  if (!cd || cd->collection_sz == 0) return NULL;

  ptrs = calloc(cd->collection_sz, sizeof *ptrs);
  if (!ptrs) return NULL;

  for (size_t i = 0; i < cd->collection_sz; i++) {
    const struct UnfacdUser *user = cd->collection[i];

    if (!user) {
      free(ptrs);
      return NULL;
    }

    records[i] = (UserRecord)USER_RECORD__INIT;
    records[i].ufsrvuid.len  = sizeof user->ufsrvuid.data;
    records[i].ufsrvuid.data = (uint8_t *)user->ufsrvuid.data;
    records[i].username      = (char *)user->nickname;
    ptrs[i]                  = &records[i];
  }

  return ptrs;
}

/**
 * @brief What a group command asks for beyond its command type.
 *
 * The wire header carries the action in @c args and, for commands whose request the client also
 * tracks locally while it waits, the stamp of that local request in @c when_client (the server
 * echoes it back on the fan-out). Zero leaves the corresponding field unset, which is what the
 * join and leave commands have always sent.
 */
typedef struct UnfacdGroupCommandSpec
{
  FenceCommand__CommandTypes command_type;
  CommandArgs                args;
  uint64_t                   when_client;
} UnfacdGroupCommandSpec;

static char *sBuildGroupCommand(const UnfacdGroup *group, const UnfacdGroupCommandSpec *spec);

static char *
sBuildGroupCommand(const UnfacdGroup *group, const UnfacdGroupCommandSpec *spec)
{
  CommandHeader  header = COMMAND_HEADER__INIT;
  FenceRecord    record = FENCE_RECORD__INIT;
  FenceRecord   *records[1];
  FenceCommand   fence  = FENCE_COMMAND__INIT;
  UfsrvCommandWire wire = UFSRV_COMMAND_WIRE__INIT;
  DataMessage    dm     = DATA_MESSAGE__INIT;
  UserRecord    *member_records  = NULL;
  UserRecord    *invited_records = NULL;
  __attribute__((cleanup(sCleanupFree))) UserRecord    **member_ptrs  = NULL;
  __attribute__((cleanup(sCleanupFree))) UserRecord    **invited_ptrs = NULL;
  __attribute__((cleanup(sCleanupFree))) unsigned char *packed  = NULL;
  __attribute__((cleanup(sCleanupFree))) unsigned char *b64     = NULL;
  char          *json    = NULL;
  size_t         packed_len;
  unsigned char  fkey_bytes[32];
  size_t         json_len;

  if (!group || !spec) return NULL;
  if (group->fid == 0 && group->fname[0] == '\0' && group->cname[0] == '\0') return NULL;

  header.command  = spec->command_type;
  header.has_when = 1;
  header.when     = (uint64_t)GetTimeNowInMillis();

  if (spec->args != COMMAND_ARGS__UNDEFINED) {
    header.args     = spec->args;
    header.has_args = 1;
  }

  if (spec->when_client > 0) {
    header.when_client     = spec->when_client;
    header.has_when_client = 1;
  }

  if (group->fid == 0) {
    if (RAND_bytes(fkey_bytes, sizeof fkey_bytes) != 1) goto fail;
    record.has_fkey  = 1;
    record.fkey.data = fkey_bytes;
    record.fkey.len  = sizeof fkey_bytes;
  }

  if (group->fid > 0) {
    record.has_fid = 1;
    record.fid     = group->fid;
  }
  if (group->fname[0] != '\0') record.fname = (char *)group->fname;
  if (group->cname[0] != '\0') record.cname = (char *)group->cname;
  if (group->description[0] != '\0') record.description = (char *)group->description;

  record.has_fence_type    = 1;
  record.fence_type        = (FenceRecord__FenceType)group->type;
  record.has_privacy_mode  = 1;
  record.privacy_mode      = (FenceRecord__PrivacyMode)group->privacy;
  record.has_join_mode     = 1;
  record.join_mode         = (FenceRecord__JoinMode)group->join_mode;
  record.has_delivery_mode = 1;
  record.delivery_mode     = (FenceRecord__DeliveryMode)group->delivery;

  if (group->maxmembers > 0) {
    record.has_maxmembers = 1;
    record.maxmembers     = group->maxmembers;
  }

  if (group->members.collection_sz > 0) {
    member_records = calloc(group->members.collection_sz, sizeof *member_records);
    if (!member_records) goto fail;
    member_ptrs = sUserRecordsFromCollection(&group->members, member_records);
    if (!member_ptrs) goto fail;
    record.members         = member_ptrs;
    record.n_members       = group->members.collection_sz;
  }

  if (group->invited.collection_sz > 0) {
    invited_records = calloc(group->invited.collection_sz, sizeof *invited_records);
    if (!invited_records) goto fail;
    invited_ptrs = sUserRecordsFromCollection(&group->invited, invited_records);
    if (!invited_ptrs) goto fail;
    record.invited_members     = invited_ptrs;
    record.n_invited_members   = group->invited.collection_sz;
  }

  records[0]      = &record;
  fence.header    = &header;
  fence.n_fences  = 1;
  fence.fences    = records;

  wire.ufsrvtype         = UFSRV_COMMAND_WIRE__UFSRV_TYPE__UFSRV_FENCE;
  wire.fencecommand      = &fence;

  dm.ufsrvcommand     = &wire;

  packed_len = data_message__get_packed_size(&dm);
  packed     = malloc(packed_len);
  if (!packed) goto fail;
  if (data_message__pack(&dm, packed) != packed_len) goto fail;

  b64 = base64_encode(packed, (int)packed_len, NULL);
  if (!b64) goto fail;

  json_len = strlen((char *)b64) + UNFACD_GROUP_JSON_OVERHEAD;
  json     = malloc(json_len);
  if (!json) goto fail;
  snprintf(json, json_len,
           "{\"messages\":[{\"content\":\"%s\",\"destinationDeviceId\":0,\"destinationRegistrationId\":0}]}",
           (char *)b64);

  free(member_records);
  free(invited_records);
  return json;

fail:
  free(member_records);
  free(invited_records);
  free(json);
  return NULL;
}

/**
 * @brief POST one group command to the fence endpoint and return the REST reply.
 *
 * Shared by every synchronous group command: the endpoint, the credentials and the ownership of
 * the reply are the same whatever the command asks for, so only the command spec differs.
 *
 * @param group fence the command applies to; its fid (or fname/cname) identifies it.
 * @param spec  command type, action and correlation stamp to put on the wire.
 *
 * @return the reply body, owned by the caller and released with free(), or NULL on refusal.
 */
static char *
sPostGroupCommand(const UnfacdGroup *group, const UnfacdGroupCommandSpec *spec)
{
  UfsrvCliContext *ctx = UfsrvCliContextInstance();
  char            *body;
  char            *reply = NULL;
  char             url[UFSRVCLI_CONTEXT_URL_MAX + 32];
  int              rc;

  if (!group || !ctx || !ctx->api_url[0]) return NULL;

  body = sBuildGroupCommand(group, spec);
  if (!body) return NULL;

  snprintf(url, sizeof url, "%s/V1/Fence", ctx->api_url);

  if (ctx->api_userpwd[0])
    rc = UnfacdRestPostBasic(url, ctx->api_userpwd, ctx->api_cookie, body, ctx->api_verify_peer, 0, &reply);
  else
    rc = UnfacdRestPost(url, ctx->api_cookie, body, ctx->api_verify_peer, 0, &reply);

  free(body);

  if (rc != 0) {
    free(reply);
    return NULL;
  }

  return reply;
}

char *
UnfacdGroupJoinSync(const UnfacdGroup *group)
{
  static const UnfacdGroupCommandSpec spec = {
    .command_type = FENCE_COMMAND__COMMAND_TYPES__JOIN,
    .args         = COMMAND_ARGS__UNDEFINED
  };

  return sPostGroupCommand(group, &spec);
}

/**
 * @brief Resolve the request's promise from the future the websocket fan-out resolves.
 *
 * One entry serves every group command: the wait is on the same future whichever command asked,
 * and the command itself is delivered by UnfacdGroupRxDeliver().
 */
static void
sGroupRxEntry(void)
{
  UnfacdStateSyncGroup *group_sync = UfsrvCoroutineGetArg();
  UfsrvFutureResult     result;

  if (!group_sync) return;

  result = UfsrvFutureGet(group_sync->rx.delivery);
  group_sync->rx.resolved = result.error == 0 ? 1 : -1;
}

/**
 * @brief Prepare a group command whose answer arrives out of band on the websocket.
 *
 * Shared by every asynchronous group command: it parks the request body, the promise/future pair
 * and the coroutine that waits on them. The caller POSTs the body and later hands the received
 * command to UnfacdGroupRxDeliver(), which matches it against @p operation.
 *
 * @param group             caller's group; filled with what the server sends.
 * @param state_sync_group  caller-owned container, or NULL to allocate one.
 * @param operation         fence operation the answer is expected to carry.
 * @param spec              command type, action and correlation stamp to put on the wire.
 *
 * @return the container, or NULL when the request could not be prepared.
 */
static UnfacdStateSyncGroup *
sGroupCommandRx(UnfacdGroup *group, UnfacdStateSyncGroup *state_sync_group, UnfacdFenceOperation operation,
                const UnfacdGroupCommandSpec *spec)
{
  UnfacdStateSyncGroup *group_sync = state_sync_group;

  if (!group || !spec) return NULL;
  if (group_sync && (group_sync->rx.promise || group_sync->rx.delivery || group_sync->rx.runner
                     || group_sync->rx.request_body)) return NULL;

  if (!group_sync) {
    group_sync = UnfacdStateSyncGroupCreate();
    if (!group_sync) return NULL;
  }

  group_sync->group             = group;
  group_sync->request_operation = operation;

  group_sync->rx.request_body = sBuildGroupCommand(group, spec);
  if (!group_sync->rx.request_body) goto failed;

  group_sync->rx.promise = UfsrvPromiseCreate(&group_sync->rx.delivery);
  if (!group_sync->rx.promise || !group_sync->rx.delivery) goto failed;

  group_sync->rx.runner = UfsrvCoroutineCreate(sGroupRxEntry, group_sync);
  if (!group_sync->rx.runner) goto failed;

  return group_sync;

failed:
  free(group_sync->rx.request_body);
  if (group_sync->rx.promise) UfsrvPromiseDestroy(group_sync->rx.promise);
  if (group_sync->rx.delivery) UfsrvFutureRelease(group_sync->rx.delivery);
  memset(&group_sync->rx, 0, sizeof group_sync->rx);
  if (!state_sync_group) free(group_sync);
  return NULL;
}

PUBLIC_API UnfacdStateSyncGroup *
UnfacdGroupJoinRx(UnfacdGroup *group, UnfacdStateSyncGroup *state_sync_group)
{
  static const UnfacdGroupCommandSpec spec = {
    .command_type = FENCE_COMMAND__COMMAND_TYPES__JOIN,
    .args         = COMMAND_ARGS__UNDEFINED
  };

  return sGroupCommandRx(group, state_sync_group, UNFACD_FENCE_JOIN, &spec);
}

PUBLIC_API int
UnfacdGroupRxDeliver(UnfacdStateSyncGroup *group_sync, const UnfacdCommandFence *command)
{
  if (!group_sync || !group_sync->rx.promise || !group_sync->group || !command) return -1;
  if (group_sync->rx.resolved || command->operation != group_sync->request_operation) return 1;
  if (!group_sync->rx.runner || command->unfacd_command_base.command_type != UnfacdCommandTypeFence) return -1;
  const UnfacdGroup *received = NULL;
  if (command->fence_records.collection_sz && !command->fence_records.collection) return -1;
  for (size_t i = 0; i < command->fence_records.collection_sz; ++i) {
    const CommandFenceRecord *record = command->fence_records.collection[i];
    if (!record) return -1;
    int matches = group_sync->group->fid
      ? record->group.fid == group_sync->group->fid
      : group_sync->group->fname[0] && strcmp(record->group.fname, group_sync->group->fname) == 0;
    if (matches) { received = &record->group; break; }
  }
  if (!received && !(command->fence_records.collection_sz == 0 && group_sync->group->fid &&
      group_sync->group->fid == command->unfacd_command_base.command_header.fid &&
      UnfacdFenceOutcomeIsRefusal(command->operation, command->unfacd_command_base.command_header.outcome))) return 1;
  UnfacdStateSyncOutcome outcome = command->unfacd_command_base.command_header.outcome;
  int accepted = UnfacdFenceOutcomeIsSuccess(command->operation, outcome);
  int refused = UnfacdFenceOutcomeIsRefusal(command->operation, outcome);
  if (!accepted && !refused) return 1;
  group_sync->state_sync_header = command->unfacd_command_base.command_header;
  if (refused) {
    UfsrvPromiseSetError(group_sync->rx.promise, EACCES);
  } else {
    if (!received->fid || UnfacdGroupCopy(group_sync->group, received) != 0) return -1;
    UfsrvPromiseSetValue(group_sync->rx.promise, group_sync, NULL);
  }
  UfsrvCoroutineResume(group_sync->rx.runner);
  group_sync->rx.runner = NULL;
  return 0;
}


PUBLIC_API char *
UnfacdGroupLeaveSync(const UnfacdGroup *group)
{
  static const UnfacdGroupCommandSpec spec = {
    .command_type = FENCE_COMMAND__COMMAND_TYPES__LEAVE,
    .args         = COMMAND_ARGS__UNDEFINED
  };

  return sPostGroupCommand(group, &spec);
}

PUBLIC_API UnfacdStateSyncGroup *
UnfacdGroupLeaveRx(UnfacdGroup *group, UnfacdStateSyncGroup *state_sync_group)
{
  static const UnfacdGroupCommandSpec spec = {
    .command_type = FENCE_COMMAND__COMMAND_TYPES__LEAVE,
    .args         = COMMAND_ARGS__UNDEFINED
  };

  return sGroupCommandRx(group, state_sync_group, UNFACD_FENCE_LEAVE, &spec);
}

/**
 * @brief Rename a fence synchronously.
 *
 * Builds FenceCommand(FNAME, UPDATED) carrying the new title in @p group->fname and POSTs it to
 * the fence endpoint. The reply is only a receipt: the rename itself is fanned out to the
 * members, this client included, on the websocket.
 *
 * @param group       fence to rename; its fid identifies it, its fname is the new title.
 * @param when_client stamp of the local request this rename answers for, echoed back on the
 *                    fan-out so the caller can retire its pending "requesting name" state.
 *                    Zero leaves the field unset.
 *
 * @return the reply body, owned by the caller and released with free(), or NULL on refusal.
 */
PUBLIC_API char *
UnfacdGroupFnameSync(const UnfacdGroup *group, uint64_t when_client)
{
  const UnfacdGroupCommandSpec spec = {
    .command_type = FENCE_COMMAND__COMMAND_TYPES__FNAME,
    .args         = COMMAND_ARGS__UPDATED,
    .when_client  = when_client
  };

  return sPostGroupCommand(group, &spec);
}

/**
 * @brief Begin a rename and return the container that will carry the result.
 *
 * Same shape as UnfacdGroupJoinRx(): the request body is built into
 * `state_sync_group->rx.request_body` for the caller to POST, and the promise/coroutine wait for
 * the FNAME fan-out, which UnfacdGroupRxDeliver() matches on the operation and fence id.
 *
 * @param group             fence to rename; its fname is the new title.
 * @param when_client       stamp of the local request, echoed back on the fan-out. Zero leaves
 *                          the field unset.
 * @param state_sync_group  caller-owned container, or NULL to allocate one.
 *
 * @return the container, or NULL when the request could not be built.
 */
PUBLIC_API UnfacdStateSyncGroup *
UnfacdGroupFnameRx(UnfacdGroup *group, uint64_t when_client, UnfacdStateSyncGroup *state_sync_group)
{
  const UnfacdGroupCommandSpec spec = {
    .command_type = FENCE_COMMAND__COMMAND_TYPES__FNAME,
    .args         = COMMAND_ARGS__UPDATED,
    .when_client  = when_client
  };

  return sGroupCommandRx(group, state_sync_group, UNFACD_FENCE_FNAME, &spec);
}