/**
 * @file unfacd_command_utils.c
 * @brief unfacd_command_utils
 *
 * Copyright (C) 2015-2026  unfacd works
 *
 * This file is part of ufsrvcli source code.
 * Created by ayman on 12/11/2026.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */


#include "unfacd_service/command/unfacd_command_utils.h"
#include "unfacd_service/UnfacdService.pb-c.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <uflib/ufsrvuid.h>
#include <ufsrvclilib/unfacd_service/unfacd_group.h>
#include <ufsrvclilib/unfacd_service/unfacd_message.h>
#include "unfacd_service/state_sync/state_sync_priv.h"

_Static_assert((int)UNFACD_OUTCOME_UNDEFINED == (int)COMMAND_ARGS__UNDEFINED, "CommandArgs ordinal mismatch: UNDEFINED");
_Static_assert((int)UNFACD_OUTCOME_INVITED == (int)COMMAND_ARGS__INVITED, "CommandArgs ordinal mismatch: INVITED");
_Static_assert((int)UNFACD_OUTCOME_ACCEPTED == (int)COMMAND_ARGS__ACCEPTED, "CommandArgs ordinal mismatch: ACCEPTED");
_Static_assert((int)UNFACD_OUTCOME_DENIED == (int)COMMAND_ARGS__DENIED, "CommandArgs ordinal mismatch: DENIED");
_Static_assert((int)UNFACD_OUTCOME_DEFERRED == (int)COMMAND_ARGS__DEFERRED, "CommandArgs ordinal mismatch: DEFERRED");
_Static_assert((int)UNFACD_OUTCOME_GEO_BASED == (int)COMMAND_ARGS__GEO_BASED, "CommandArgs ordinal mismatch: GEO_BASED");
_Static_assert((int)UNFACD_OUTCOME_REJECTED == (int)COMMAND_ARGS__REJECTED, "CommandArgs ordinal mismatch: REJECTED");
_Static_assert((int)UNFACD_OUTCOME_CREATED == (int)COMMAND_ARGS__CREATED, "CommandArgs ordinal mismatch: CREATED");
_Static_assert((int)UNFACD_OUTCOME_DELETED == (int)COMMAND_ARGS__DELETED, "CommandArgs ordinal mismatch: DELETED");
_Static_assert((int)UNFACD_OUTCOME_UPDATED == (int)COMMAND_ARGS__UPDATED, "CommandArgs ordinal mismatch: UPDATED");
_Static_assert((int)UNFACD_OUTCOME_ADDED == (int)COMMAND_ARGS__ADDED, "CommandArgs ordinal mismatch: ADDED");
_Static_assert((int)UNFACD_OUTCOME_INVITED_GEO == (int)COMMAND_ARGS__INVITED_GEO, "CommandArgs ordinal mismatch: INVITED_GEO");
_Static_assert((int)UNFACD_OUTCOME_POSTED == (int)COMMAND_ARGS__POSTED, "CommandArgs ordinal mismatch: POSTED");
_Static_assert((int)UNFACD_OUTCOME_SET == (int)COMMAND_ARGS__SET, "CommandArgs ordinal mismatch: SET");
_Static_assert((int)UNFACD_OUTCOME_UNSET == (int)COMMAND_ARGS__UNSET, "CommandArgs ordinal mismatch: UNSET");
_Static_assert((int)UNFACD_OUTCOME_ACCEPTED_PARTIAL == (int)COMMAND_ARGS__ACCEPTED_PARTIAL, "CommandArgs ordinal mismatch: ACCEPTED_PARTIAL");
_Static_assert((int)UNFACD_OUTCOME_ACCEPTED_INVITE == (int)COMMAND_ARGS__ACCEPTED_INVITE, "CommandArgs ordinal mismatch: ACCEPTED_INVITE");
_Static_assert((int)UNFACD_OUTCOME_UNINVITED == (int)COMMAND_ARGS__UNINVITED, "CommandArgs ordinal mismatch: UNINVITED");
_Static_assert((int)UNFACD_OUTCOME_EXPIRED == (int)COMMAND_ARGS__EXPIRED, "CommandArgs ordinal mismatch: EXPIRED");
_Static_assert((int)UNFACD_OUTCOME_CEASED == (int)COMMAND_ARGS__CEASED, "CommandArgs ordinal mismatch: CEASED");
_Static_assert((int)UNFACD_OUTCOME_SHARED == (int)COMMAND_ARGS__SHARED, "CommandArgs ordinal mismatch: SHARED");
_Static_assert((int)UNFACD_OUTCOME_UNSHARED == (int)COMMAND_ARGS__UNSHARED, "CommandArgs ordinal mismatch: UNSHARED");
_Static_assert((int)UNFACD_OUTCOME_RESYNC == (int)COMMAND_ARGS__RESYNC, "CommandArgs ordinal mismatch: RESYNC");
_Static_assert((int)UNFACD_OUTCOME_UNCHANGED == (int)COMMAND_ARGS__UNCHANGED, "CommandArgs ordinal mismatch: UNCHANGED");
_Static_assert((int)UNFACD_OUTCOME_REJECTED_USER == (int)COMMAND_ARGS__REJECTED_USER, "CommandArgs ordinal mismatch: REJECTED_USER");
_Static_assert((int)UNFACD_OUTCOME_SYNCED == (int)COMMAND_ARGS__SYNCED, "CommandArgs ordinal mismatch: SYNCED");
_Static_assert((int)UNFACD_OUTCOME_LINK_BASED == (int)COMMAND_ARGS__LINK_BASED, "CommandArgs ordinal mismatch: LINK_BASED");

static int
sCopyStr(char *dst, size_t cap, const char *src)
{
  if (!cap || (src && strlen(src) >= cap)) return -1;
  if (!src) dst[0] = '\0';
  else memcpy(dst, src, strlen(src) + 1);
  return 0;
}

int
UnfacdCommandFillHeader(UnfacdCommandHeader *sync, const CommandHeader *header)
{
  UnfacdCommandHeader *h;

  if (!sync || !header) return -1;

  h = sync;
  memset(h, 0, sizeof *h);

  if (sCopyStr(h->path, sizeof h->path, header->path) != 0) return -1;

  if (header->ufsrvuid.len) {
    if (!header->ufsrvuid.data || header->ufsrvuid.len != sizeof(UfsrvUid)) return -1;
    UfsrvUid uid;
    memcpy(uid.data, header->ufsrvuid.data, sizeof uid.data);
    if (!UfsrvUidConvertSerialise(&uid, h->ufsrvuid)) return -1;
  }

  h->cid      = header->has_cid ? header->cid : 0;
  h->gid      = header->has_gid ? header->gid : 0;
  h->eid      = header->has_eid ? header->eid : 0;
  h->fid      = header->has_fid ? header->fid : 0;
  h->eid_client = header->has_eid_client ? header->eid_client : 0;
  h->when     = header->has_when ? header->when : 0;
  if (sCopyStr(h->sid, sizeof h->sid, header->sid) != 0) return -1;
  h->command  = header->command;
  h->args     = header->has_args ? header->args : 0;
  h->args_client = header->has_args_client ? header->args_client : 0;
  h->args_error = header->has_args_error ? header->args_error : 0;
  h->args_error_client = header->has_args_error_client ? header->args_error_client : 0;
  if (sCopyStr(h->uname, sizeof h->uname, header->uname) != 0) return -1;
  h->when_eid = header->has_when_eid ? header->when_eid : 0;
  h->when_client = header->has_when_client ? header->when_client : 0;
  if (sCopyStr(h->cookie, sizeof h->cookie, header->cookie) != 0) return -1;
  h->destination_device_id = header->has_destination_device_id ? header->destination_device_id : 0;

  if (h->args > (uint32_t)UNFACD_OUTCOME_LINK_BASED) return -1;
  h->outcome = (UnfacdStateSyncOutcome)h->args;

  return 0;
}

static int
sUnfacdGroupFillFromFenceRecord(UnfacdGroup *group, const FenceRecord *record)
{
  if (!group || !record) return -1;

  if (record->has_fid) group->fid = record->fid;
  sCopyStr(group->fname, sizeof group->fname, record->fname);
  sCopyStr(group->cname, sizeof group->cname, record->cname);
  sCopyStr(group->description, sizeof group->description, record->description);

  if (record->has_fence_type) group->type = (UnfacdGroupType)record->fence_type;
  if (record->has_privacy_mode) group->privacy = (UnfacdGroupPrivacy)record->privacy_mode;
  if (record->has_join_mode) group->join_mode = (UnfacdGroupJoinMode)record->join_mode;
  if (record->has_delivery_mode) group->delivery = (UnfacdGroupDeliveryMode)record->delivery_mode;
  if (record->has_longevity_mode) group->longevity = (UnfacdGroupLongevity)record->longevity_mode;

  if (record->has_maxmembers) group->maxmembers = record->maxmembers;
  if (record->has_usercount) group->usercount = record->usercount;
  if (record->has_eid) group->eid = record->eid;
  if (record->has_created) group->created = record->created;
  if (record->has_ttl) group->ttl = record->ttl;

  return 0;
}

PUBLIC_API int
UnfacdFenceOutcomeIsSuccess(UnfacdFenceOperation operation, UnfacdStateSyncOutcome outcome)
{
  if (operation == UNFACD_FENCE_JOIN) {
    switch (outcome) {
      case UNFACD_OUTCOME_CREATED:
      case UNFACD_OUTCOME_ACCEPTED:
      case UNFACD_OUTCOME_ACCEPTED_PARTIAL:
      case UNFACD_OUTCOME_ACCEPTED_INVITE:
      case UNFACD_OUTCOME_GEO_BASED:
      case UNFACD_OUTCOME_LINK_BASED:
      case UNFACD_OUTCOME_UNCHANGED: return 1;
      default: return 0;
    }
  }
  if (operation == UNFACD_FENCE_LEAVE) {
    switch (outcome) {
      case UNFACD_OUTCOME_ACCEPTED:
      case UNFACD_OUTCOME_UNINVITED:
      case UNFACD_OUTCOME_DELETED:
      case UNFACD_OUTCOME_CEASED: return 1;
      default: return 0;
    }
  }
  if (operation == UNFACD_FENCE_FNAME) {
    /* UPDATED is the fan-out to the other members, ACCEPTED the originator's own copy; both
       carry the new title. A rejected rename carries the restored title instead and is a refusal. */
    switch (outcome) {
      case UNFACD_OUTCOME_UPDATED:
      case UNFACD_OUTCOME_ACCEPTED: return 1;
      default: return 0;
    }
  }
  return 0;
}

PUBLIC_API int
UnfacdFenceOutcomeIsRefusal(UnfacdFenceOperation operation, UnfacdStateSyncOutcome outcome)
{
  if ((unsigned)operation >= UNFACD_FENCE_COMMAND_MAX) return 0;
  return outcome == UNFACD_OUTCOME_DENIED || outcome == UNFACD_OUTCOME_REJECTED ||
         outcome == UNFACD_OUTCOME_REJECTED_USER || outcome == UNFACD_OUTCOME_DEFERRED;
}

static CollectionDescriptor *
sGroupCollection(UnfacdGroup *group, unsigned index)
{
  switch (index) {
    case 0: return &group->members;
    case 1: return &group->invited;
    case 2: return &group->linkjoin;
    default: return &group->banned;
  }
}

PUBLIC_API void
UnfacdGroupClear(UnfacdGroup *group)
{
  if (!group) return;
  for (unsigned index = 0; index < 4; ++index) {
    CollectionDescriptor *collection = sGroupCollection(group, index);
    if (!(group->owned_collections & (1u << index))) continue;
    for (size_t i = 0; i < collection->collection_sz; ++i) free(collection->collection[i]);
    free(collection->collection);
  }
  memset(group, 0, sizeof *group);
}

PUBLIC_API int
UnfacdGroupCopy(UnfacdGroup *destination, const UnfacdGroup *source)
{
  if (!destination || !source) return -1;
  if (destination == source) return 0;
  UnfacdGroup copy = *source;
  copy.owned_collections = 0;
  for (unsigned index = 0; index < 4; ++index) {
    const CollectionDescriptor *input = sGroupCollection((UnfacdGroup *)source, index);
    CollectionDescriptor *output = sGroupCollection(&copy, index);
    memset(output, 0, sizeof *output);
    if (!input->collection_sz) continue;
    if (!input->collection || input->collection_base_offset || input->collection_sz > SIZE_MAX / sizeof(void *)) goto failed;
    output->collection = calloc(input->collection_sz, sizeof(void *));
    if (!output->collection) goto failed;
    copy.owned_collections |= 1u << index;
    for (size_t i = 0; i < input->collection_sz; ++i) {
      if (!input->collection[i]) goto failed;
      output->collection[i] = malloc(sizeof(struct UnfacdUser));
      if (!output->collection[i]) goto failed;
      memcpy(output->collection[i], input->collection[i], sizeof(struct UnfacdUser));
      ++output->collection_sz;
    }
  }
  UnfacdGroupClear(destination);
  *destination = copy;
  return 0;
failed:
  UnfacdGroupClear(&copy);
  return -1;
}

int
UnfacdCommandFillFenceGroup(UnfacdGroup *group, const FenceRecord *wire)
{
  if (!group || !wire) return -1;

  if ((wire->fname && strlen(wire->fname) >= sizeof group->fname) ||
      (wire->cname && strlen(wire->cname) >= sizeof group->cname) ||
      (wire->description && strlen(wire->description) >= sizeof group->description)) return -1;

  if (sUnfacdGroupFillFromFenceRecord(group, wire) != 0) return -1;

  size_t sizes[] = {wire->n_members, wire->n_invited_members, wire->n_linkjoin_members, wire->n_banned_members};
  UserRecord **lists[] = {wire->members, wire->invited_members, wire->linkjoin_members, wire->banned_members};
  for (unsigned index = 0; index < 4; ++index) {
    CollectionDescriptor *output = sGroupCollection(group, index);
    if (!sizes[index]) continue;
    if (!lists[index] || sizes[index] > SIZE_MAX / sizeof(void *)) goto failed;
    output->collection = calloc(sizes[index], sizeof(void *));
    if (!output->collection) goto failed;
    group->owned_collections |= 1u << index;
    for (size_t i = 0; i < sizes[index]; ++i) {
      UserRecord *input = lists[index][i];
      if (!input || !input->ufsrvuid.data || input->ufsrvuid.len != sizeof(UfsrvUid) ||
          !input->username || strlen(input->username) >= sizeof(((struct UnfacdUser *)0)->username)) goto failed;
      struct UnfacdUser *user = calloc(1, sizeof *user);
      if (!user) goto failed;
      output->collection[i] = user;
      ++output->collection_sz;
      if (UnfacdCommandFillUser(user, input) != 0) goto failed;
    }
  }
  return 0;
failed:
  UnfacdGroupClear(group);
  return -1;
}

int
UnfacdCommandFillUser(UnfacdUser *user, const UserRecord *wire)
{
  if (!user || !wire || !wire->ufsrvuid.data || wire->ufsrvuid.len != sizeof user->ufsrvuid) return -1;
  if (sCopyStr(user->username, sizeof user->username, wire->username) != 0) return -1;
  memcpy(user->ufsrvuid.data, wire->ufsrvuid.data, sizeof user->ufsrvuid.data);
  snprintf(user->nickname, sizeof user->nickname, "%.*s", (int)sizeof user->nickname - 1, user->username);
  return 0;
}

int
UnfacdCommandFillMessageRecord(UnfacdMessage *message, const MessageRecord *wire, uint64_t fid, const UnfacdUser *from)
{
  if (!message || !wire || !from) return -1;
  if (wire->message.len && (!wire->has_message || !wire->message.data)) return -1;
  if ((unsigned)wire->protocol_type > UNFACD_MESSAGE_PROTOCOL_PREKEY_BUNDLE) return -1;
  UnfacdMessage native = {0};
  native.fid = fid;
  native.from = *from;
  native.body = wire->has_message ? wire->message.data : NULL;
  native.body_len = wire->has_message ? wire->message.len : 0;
  native.protocol = wire->has_protocol_type ? (UnfacdMessageProtocolType)wire->protocol_type : UNFACD_MESSAGE_PROTOCOL_UNKNOWN;
  native.ttl = wire->has_ttl ? wire->ttl : 0;
  native.view_once = wire->has_view_once && wire->view_once;
  return UnfacdMessageCopy(message, &native);
}

_Static_assert((int)UNFACD_MESSAGE_SAY == (int)MESSAGE_COMMAND__COMMAND_TYPES__SAY, "Message operation ordinal mismatch: SAY");
_Static_assert((int)UNFACD_MESSAGE_CONTACTS == (int)MESSAGE_COMMAND__COMMAND_TYPES__CONTACTS, "Message operation ordinal mismatch: CONTACTS");
_Static_assert((int)UNFACD_MESSAGE_FLAG == (int)MESSAGE_COMMAND__COMMAND_TYPES__FLAG, "Message operation ordinal mismatch: FLAG");
_Static_assert((int)UNFACD_MESSAGE_LIKE == (int)MESSAGE_COMMAND__COMMAND_TYPES__LIKE, "Message operation ordinal mismatch: LIKE");
_Static_assert((int)UNFACD_MESSAGE_FOLLOW == (int)MESSAGE_COMMAND__COMMAND_TYPES__FOLLOW, "Message operation ordinal mismatch: FOLLOW");
_Static_assert((int)UNFACD_MESSAGE_PREVIEW == (int)MESSAGE_COMMAND__COMMAND_TYPES__PREVIEW, "Message operation ordinal mismatch: PREVIEW");
_Static_assert((int)UNFACD_MESSAGE_INTRO == (int)MESSAGE_COMMAND__COMMAND_TYPES__INTRO, "Message operation ordinal mismatch: INTRO");
_Static_assert((int)UNFACD_MESSAGE_STICKER == (int)MESSAGE_COMMAND__COMMAND_TYPES__STICKER, "Message operation ordinal mismatch: STICKER");
_Static_assert((int)UNFACD_MESSAGE_GUARDIAN_REQUEST == (int)MESSAGE_COMMAND__COMMAND_TYPES__GUARDIAN_REQUEST, "Message operation ordinal mismatch: GUARDIAN_REQUEST");
_Static_assert((int)UNFACD_MESSAGE_GUARDIAN_LINK == (int)MESSAGE_COMMAND__COMMAND_TYPES__GUARDIAN_LINK, "Message operation ordinal mismatch: GUARDIAN_LINK");
_Static_assert((int)UNFACD_MESSAGE_GUARDIAN_UNLINK == (int)MESSAGE_COMMAND__COMMAND_TYPES__GUARDIAN_UNLINK, "Message operation ordinal mismatch: GUARDIAN_UNLINK");
_Static_assert((int)UNFACD_MESSAGE_EFFECT == (int)MESSAGE_COMMAND__COMMAND_TYPES__EFFECT, "Message operation ordinal mismatch: EFFECT");
_Static_assert((int)UNFACD_MESSAGE_REACTION == (int)MESSAGE_COMMAND__COMMAND_TYPES__REACTION, "Message operation ordinal mismatch: REACTION");
_Static_assert((int)UNFACD_MESSAGE_REVOKE == (int)MESSAGE_COMMAND__COMMAND_TYPES__REVOKE, "Message operation ordinal mismatch: REVOKE");
_Static_assert((int)UNFACD_MESSAGE_INTRO_USER_RESPONSE == (int)MESSAGE_COMMAND__COMMAND_TYPES__INTRO_USER_RESPONSE, "Message operation ordinal mismatch: INTRO_USER_RESPONSE");
