//
// Created by ayman on 11/11/2026.
//

/**
 * @file unfacd_command_fence_type.h
 * @brief unfacd_command_fence_type
 *
 * Copyright (C) 2015-2026  unfacd works
 *
 * This file is part of ufsrvcli source code.
 * Created by ayman on 11/11/2026.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef UFSRVCLI_UNFACD_COMMAND_FENCE_TYPE_H
#define UFSRVCLI_UNFACD_COMMAND_FENCE_TYPE_H

#include <ufsrvclilib/unfacd_service/unfacd_group_type.h>

#include "unfacd_command_type.h"

typedef enum UnfacdFenceOperation {
  UNFACD_FENCE_JOIN = 0,
  UNFACD_FENCE_LEAVE = 1,
  UNFACD_FENCE_INVITE = 2,
  UNFACD_FENCE_BLOCK = 3,
  UNFACD_FENCE_FNAME = 4,
  UNFACD_FENCE_BANNER = 5,
  UNFACD_FENCE_AVATAR = 6,
  UNFACD_FENCE_MAXMEMBERS = 7,
  UNFACD_FENCE_TTL = 8,
  UNFACD_FENCE_FKEY = 9,
  UNFACD_FENCE_JOIN_MODE = 10,
  UNFACD_FENCE_TAGS = 11,
  UNFACD_FENCE_MAKE = 12,
  UNFACD_FENCE_STATE = 13,
  UNFACD_FENCE_EXPIRY = 14,
  UNFACD_FENCE_MUTE = 15,
  UNFACD_FENCE_PERMISSION = 16,
  UNFACD_FENCE_NICKNAME = 17,
  UNFACD_FENCE_KICK = 18,
  UNFACD_FENCE_DESTRUCT = 19,
  UNFACD_FENCE_PRIVACY_MODE = 20,
  UNFACD_FENCE_DELIVERY_MODE = 21,
  UNFACD_FENCE_PERMISSION_LIST_SEMANTICS = 22,
  UNFACD_FENCE_INVITE_REJECTED = 23,
  UNFACD_FENCE_INVITE_DELETED = 24,
  UNFACD_FENCE_LINKJOIN = 25,
  UNFACD_FENCE_DESCRIPTION = 26,
  UNFACD_FENCE_COMMAND_MAX
} UnfacdFenceOperation;

typedef struct CommandFenceRecord
{
  UnfacdGroup group;
} CommandFenceRecord;

typedef struct UnfacdCommandFence
{
  UnfacdCommand unfacd_command_base;
  CollectionDescriptor fence_records;
  UnfacdFenceOperation operation;
} UnfacdCommandFence;

#endif //UFSRVCLI_UNFACD_COMMAND_FENCE_TYPE_H
