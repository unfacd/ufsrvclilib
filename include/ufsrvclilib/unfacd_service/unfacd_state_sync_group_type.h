//
// Created by ayman on 8/11/2026.
//

/**
 * @file unfacd_state_sync_group.h
 * @brief unfacd_state_sync_group
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

#ifndef UFSRVCLI_UNFACD_STATE_SYNC_GROUP_H
#define UFSRVCLI_UNFACD_STATE_SYNC_GROUP_H

#include "state_sync_rx_context_type.h"
#include "unfacd_group_type.h"
#include "command/unfacd_command_header_type.h"
#include "command/unfacd_command_fence_type.h"

typedef struct UnfacdStateSyncGroup
{
  UnfacdCommandHeader          state_sync_header; /*!< base: header of the command received. */
  UnfacdStateSyncRxContext rx;    /*!< shared coroutine/future state for this request. */
  UnfacdFenceOperation request_operation;
  UnfacdGroup             *group; /*!< caller's group, filled with the attributes received. */
} UnfacdStateSyncGroup;

#endif //UFSRVCLI_UNFACD_STATE_SYNC_GROUP_H
