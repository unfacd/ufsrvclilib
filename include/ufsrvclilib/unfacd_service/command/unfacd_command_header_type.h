/**
 * @file unfacd_command_header_type.h
 * @brief unfacd_command_header_type
 *
 * Copyright (C) 2015-2026  unfacd works
 *
 * This file is part of ufsrvcli source code.
 * Created by ayman on 8/11/2026.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef UFSRVCLI_UNFACD_COMMAND_HEADER_TYPE_H
#define UFSRVCLI_UNFACD_COMMAND_HEADER_TYPE_H

#include <stddef.h>
#include <stdint.h>

#include <ufsrvclilib/unfacd_service/unfacd_group_type.h>

#define UNFACD_COMMAND_PATH_MAX     512
#define UNFACD_COMMAND_UFSRVUID_MAX 64
#define UNFACD_COMMAND_SID_MAX      64
#define UNFACD_COMMAND_UNAME_MAX    128
#define UNFACD_COMMAND_COOKIE_MAX   256

typedef enum UnfacdStateSyncOutcome
{
  UNFACD_OUTCOME_UNDEFINED = 0,
  UNFACD_OUTCOME_INVITED = 1,
  UNFACD_OUTCOME_ACCEPTED = 2,
  UNFACD_OUTCOME_DENIED = 3,
  UNFACD_OUTCOME_DEFERRED = 4,
  UNFACD_OUTCOME_GEO_BASED = 5,
  UNFACD_OUTCOME_REJECTED = 6,
  UNFACD_OUTCOME_CREATED = 7,
  UNFACD_OUTCOME_DELETED = 8,
  UNFACD_OUTCOME_UPDATED = 9,
  UNFACD_OUTCOME_ADDED = 10,
  UNFACD_OUTCOME_INVITED_GEO = 11,
  UNFACD_OUTCOME_POSTED = 12,
  UNFACD_OUTCOME_SET = 13,
  UNFACD_OUTCOME_UNSET = 14,
  UNFACD_OUTCOME_ACCEPTED_PARTIAL = 15,
  UNFACD_OUTCOME_ACCEPTED_INVITE = 16,
  UNFACD_OUTCOME_UNINVITED = 17,
  UNFACD_OUTCOME_EXPIRED = 18,
  UNFACD_OUTCOME_CEASED = 19,
  UNFACD_OUTCOME_SHARED = 20,
  UNFACD_OUTCOME_UNSHARED = 21,
  UNFACD_OUTCOME_RESYNC = 22,
  UNFACD_OUTCOME_UNCHANGED = 23,
  UNFACD_OUTCOME_REJECTED_USER = 24,
  UNFACD_OUTCOME_SYNCED = 25,
  UNFACD_OUTCOME_LINK_BASED = 26,
} UnfacdStateSyncOutcome;

typedef struct UnfacdCommandHeader
{
  char     path[UNFACD_COMMAND_PATH_MAX];
  char     ufsrvuid[UNFACD_COMMAND_UFSRVUID_MAX];
  uint64_t cid;
  uint64_t gid;
  uint64_t eid;
  uint64_t fid;
  uint64_t eid_client;
  uint64_t when;
  char     sid[UNFACD_COMMAND_SID_MAX];
  uint32_t command;
  uint32_t args;
  uint32_t args_client;
  uint32_t args_error;
  uint32_t args_error_client;
  char     uname[UNFACD_COMMAND_UNAME_MAX];
  uint64_t when_eid;
  uint64_t when_client;
  char     cookie[UNFACD_COMMAND_COOKIE_MAX];
  uint32_t destination_device_id;
  UnfacdStateSyncOutcome outcome;
} UnfacdCommandHeader;

#endif //UFSRVCLI_UNFACD_COMMAND_HEADER_TYPE_H
