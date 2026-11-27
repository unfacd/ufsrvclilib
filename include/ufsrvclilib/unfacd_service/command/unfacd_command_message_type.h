//
// Created by ayman on 11/11/2026.
//

/**
 * @file unfacd_command_message_type.h
 * @brief unfacd_command_message_type
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

#ifndef UFSRVCLI_UNFACD_COMMAND_MESSAGE_TYPE_H
#define UFSRVCLI_UNFACD_COMMAND_MESSAGE_TYPE_H

#include <ufsrvclilib/unfacd_service/unfacd_message_type.h>

#include "unfacd_command_type.h"
#include "unfacd_command_fence_type.h"

typedef enum UnfacdMessageOperation {
  UNFACD_MESSAGE_SAY = 0,
  UNFACD_MESSAGE_CONTACTS = 1,
  UNFACD_MESSAGE_FLAG = 2,
  UNFACD_MESSAGE_LIKE = 3,
  UNFACD_MESSAGE_FOLLOW = 4,
  UNFACD_MESSAGE_PREVIEW = 5,
  UNFACD_MESSAGE_INTRO = 6,
  UNFACD_MESSAGE_STICKER = 7,
  UNFACD_MESSAGE_GUARDIAN_REQUEST = 8,
  UNFACD_MESSAGE_GUARDIAN_LINK = 9,
  UNFACD_MESSAGE_GUARDIAN_UNLINK = 10,
  UNFACD_MESSAGE_EFFECT = 11,
  UNFACD_MESSAGE_REACTION = 12,
  UNFACD_MESSAGE_REVOKE = 13,
  UNFACD_MESSAGE_INTRO_USER_RESPONSE = 14,
  UNFACD_MESSAGE_COMMAND_MAX
} UnfacdMessageOperation;

typedef struct CommandMessageRecord
{
  UnfacdMessage message;
} CommandMessageRecord;

typedef struct UnfacdCommandMessage
{
  UnfacdCommand unfacd_command_base;
  UnfacdMessageOperation operation;
  UnfacdUser originator;
  CollectionDescriptor message_records;
  CollectionDescriptor fence_records;
} UnfacdCommandMessage;

#endif //UFSRVCLI_UNFACD_COMMAND_MESSAGE_TYPE_H
