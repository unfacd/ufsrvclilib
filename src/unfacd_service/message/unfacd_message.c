#include "unfacd_service/command/unfacd_command_utils.h"
/**
 * @file unfacd_message.c
 * @brief unfacd_message
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
#include <ufsrvclilib/unfacd_service/unfacd_message.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdatomic.h>

#include <uflib/utils_base64.h>
#include <uflib/utils_time.h>

#include <unfacd_service/UnfacdService.pb-c.h>

#include <ufsrvclilib/unfacd_service/unfacd_rest.h>
#include <ufsrvclilib/unfacd_service/state_sync_message.h>

#include <ufsrvrxlib/ufsrv_coroutine/ufsrv_coroutine.h>
#include <ufsrvrxlib/ufsrv_future/ufsrv_future.h>

#include "ufsrvcli_context_priv.h"
#include "unfacd_service/state_sync/state_sync_priv.h"

#define UNFACD_MESSAGE_JSON_OVERHEAD 160u

static void
sCleanupFree(void *p_)
{
  void **p = (void **)p_;

  free(*p);
}

static char *
sBuildMessage(const UnfacdMessage *message, uint64_t when)
{
  CommandHeader    header  = COMMAND_HEADER__INIT;
  FenceRecord      record  = FENCE_RECORD__INIT;
  FenceRecord     *records[1];
  MessageRecord    body    = MESSAGE_RECORD__INIT;
  MessageRecord   *bodies[1];
  MessageCommand   msgcmd  = MESSAGE_COMMAND__INIT;
  UfsrvCommandWire wire    = UFSRV_COMMAND_WIRE__INIT;
  DataMessage      dm      = DATA_MESSAGE__INIT;
  __attribute__((cleanup(sCleanupFree))) unsigned char *packed  = NULL;
  __attribute__((cleanup(sCleanupFree))) unsigned char *b64     = NULL;
  char          *json    = NULL;
  size_t           packed_len;
  size_t           json_len;

  if (!message) return NULL;
  if (message->fid == 0) return NULL;
  if (!message->body || message->body_len == 0) return NULL;

  header.command  = MESSAGE_COMMAND__COMMAND_TYPES__SAY;
  header.has_when = 1;
  header.when     = when;

  record.has_fid = 1;
  record.fid     = message->fid;

  body.has_message = 1;
  body.message.data = (uint8_t *)message->body;
  body.message.len  = message->body_len;
  body.has_protocol_type = 1;
  body.protocol_type     = (MessageCommand__ProtocolType)message->protocol;
  if (message->ttl > 0) {
    body.has_ttl = 1;
    body.ttl     = message->ttl;
  }
  if (message->view_once) {
    body.has_view_once = 1;
    body.view_once     = 1;
  }

  records[0] = &record;
  bodies[0]  = &body;

  msgcmd.header     = &header;
  msgcmd.n_fences   = 1;
  msgcmd.fences     = records;
  msgcmd.n_messages = 1;
  msgcmd.messages   = bodies;

  wire.ufsrvtype      = UFSRV_COMMAND_WIRE__UFSRV_TYPE__UFSRV_MESSAGE;
  wire.msgcommand     = &msgcmd;

  dm.ufsrvcommand = &wire;

  packed_len = data_message__get_packed_size(&dm);
  packed     = malloc(packed_len);
  if (!packed) goto fail;
  if (data_message__pack(&dm, packed) != packed_len) goto fail;

  b64 = base64_encode(packed, (int)packed_len, NULL);
  if (!b64) goto fail;

  json_len = strlen((char *)b64) + UNFACD_MESSAGE_JSON_OVERHEAD;
  json     = malloc(json_len);
  if (!json) goto fail;
  snprintf(json, json_len,
           "{\"messages\":[{\"content\":\"%s\",\"destinationDeviceId\":0,\"destinationRegistrationId\":0}]}",
           (char *)b64);

  return json;

fail:
  free(json);
  return NULL;
}


static uint64_t
sNextMessageWhen(void)
{
  static atomic_uint_fast64_t last;
  uint_fast64_t previous = atomic_load(&last), next;
  do {
    uint_fast64_t now = (uint64_t)GetTimeNowInMillis();
    next = now > previous ? now : previous + 1;
  } while (!atomic_compare_exchange_weak(&last, &previous, next));
  return next;
}

PUBLIC_API char *
UnfacdMessageGroup(const UnfacdMessage *message)
{
  return sBuildMessage(message, sNextMessageWhen());
}

PUBLIC_API void
UnfacdMessageClear(UnfacdMessage *message)
{
  if (!message) return;
  if (message->owned_body) free((void *)message->body);
  memset(message, 0, sizeof *message);
}

PUBLIC_API int
UnfacdMessageCopy(UnfacdMessage *destination, const UnfacdMessage *source)
{
  if (!destination || !source || (source->body_len && !source->body)) return -1;
  UnfacdMessage copy = *source;
  copy.body = NULL;
  copy.owned_body = 0;
  if (source->body_len) {
    unsigned char *body = malloc(source->body_len);
    if (!body) return -1;
    memcpy(body, source->body, source->body_len);
    copy.body = body;
    copy.owned_body = 1;
  }
  UnfacdMessageClear(destination);
  *destination = copy;
  return 0;
}

PUBLIC_API int
UnfacdMessageOutcomeIsSuccess(UnfacdMessageOperation operation, UnfacdStateSyncOutcome outcome)
{
  return operation == UNFACD_MESSAGE_SAY && outcome == UNFACD_OUTCOME_ACCEPTED;
}

PUBLIC_API int
UnfacdMessageOutcomeIsRefusal(UnfacdMessageOperation operation, UnfacdStateSyncOutcome outcome)
{
  return (unsigned)operation < UNFACD_MESSAGE_COMMAND_MAX &&
    (outcome == UNFACD_OUTCOME_DENIED || outcome == UNFACD_OUTCOME_REJECTED ||
     outcome == UNFACD_OUTCOME_REJECTED_USER || outcome == UNFACD_OUTCOME_DEFERRED);
}


PUBLIC_API char *
UnfacdGroupMessageSync(const UnfacdMessage *message)
{
  UfsrvCliContext *ctx = UfsrvCliContextInstance();
  char            *body;
  char            *reply = NULL;
  char             url[UFSRVCLI_CONTEXT_URL_MAX + 32];
  int              rc;

  if (!message || !ctx || !ctx->api_url[0]) return NULL;

  body = UnfacdMessageGroup(message);
  if (!body) return NULL;

  snprintf(url, sizeof url, "%s/V1/Message", ctx->api_url);

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

static void
sGroupMessageRxEntry(void)
{
  UnfacdStateSyncMessage *msg_sync = UfsrvCoroutineGetArg();
  UfsrvFutureResult       result;

  if (!msg_sync) return;

  result = UfsrvFutureGet(msg_sync->rx.delivery);
  msg_sync->rx.resolved = result.error == 0 ? 1 : -1;
}

PUBLIC_API UnfacdStateSyncMessage *
UnfacdGroupMessageRx(UnfacdMessage *message, UnfacdStateSyncMessage *state_sync_message)
{
  UnfacdStateSyncMessage *msg_sync = state_sync_message;

  if (!message) return NULL;
  if (msg_sync && (msg_sync->rx.promise || msg_sync->rx.delivery || msg_sync->rx.runner || msg_sync->rx.request_body)) return NULL;

  if (!msg_sync) {
    msg_sync = UnfacdStateSyncMessageCreate();
    if (!msg_sync) return NULL;
  }

  msg_sync->message = message;

  msg_sync->request_fid = message->fid;
  msg_sync->request_when = sNextMessageWhen();
  msg_sync->rx.request_body = sBuildMessage(message, msg_sync->request_when);
  if (!msg_sync->rx.request_body) goto failed;

  msg_sync->rx.promise = UfsrvPromiseCreate(&msg_sync->rx.delivery);
  if (!msg_sync->rx.promise || !msg_sync->rx.delivery) goto failed;

  msg_sync->rx.runner = UfsrvCoroutineCreate(sGroupMessageRxEntry, msg_sync);
  if (!msg_sync->rx.runner) goto failed;

  return msg_sync;

failed:
  free(msg_sync->rx.request_body);
  if (msg_sync->rx.promise) UfsrvPromiseDestroy(msg_sync->rx.promise);
  if (msg_sync->rx.delivery) UfsrvFutureRelease(msg_sync->rx.delivery);
  memset(&msg_sync->rx, 0, sizeof msg_sync->rx);
  if (!state_sync_message) free(msg_sync);
  return NULL;
}

PUBLIC_API int
UnfacdGroupMessageRxDeliver(UnfacdStateSyncMessage *msg_sync, const UnfacdCommandMessage *command)
{
  if (!msg_sync || !msg_sync->rx.promise || !msg_sync->message || !command) return -1;
  if (msg_sync->rx.resolved || command->operation != UNFACD_MESSAGE_SAY) return 1;
  if (!msg_sync->rx.runner || command->unfacd_command_base.command_type != UnfacdCommandTypeMessage) return -1;
  const UnfacdCommandHeader *header = &command->unfacd_command_base.command_header;
  if (!msg_sync->request_when || header->when_client != msg_sync->request_when) return 1;
  uint64_t fid = header->fid;
  if (command->fence_records.collection_sz && !command->fence_records.collection) return -1;
  if (!fid && command->fence_records.collection_sz == 1) {
    const CommandFenceRecord *record = command->fence_records.collection[0];
    if (!record) return -1;
    fid = record->group.fid;
  }
  if (!fid || fid != msg_sync->request_fid) return 1;
  int accepted = UnfacdMessageOutcomeIsSuccess(command->operation, header->outcome);
  int refused = UnfacdMessageOutcomeIsRefusal(command->operation, header->outcome);
  if (!accepted && !refused) return 1;
  msg_sync->state_sync_header = *header;
  if (refused) UfsrvPromiseSetError(msg_sync->rx.promise, EACCES);
  else UfsrvPromiseSetValue(msg_sync->rx.promise, msg_sync, NULL);
  UfsrvCoroutineResume(msg_sync->rx.runner);
  msg_sync->rx.runner = NULL;
  return 0;
}
