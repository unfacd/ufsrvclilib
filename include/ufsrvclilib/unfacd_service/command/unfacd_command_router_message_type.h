/**
 * @file unfacd_command_router_message_type.h
 * @brief unfacd_command_router_message_type.h
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

#ifndef UFSRVCLI_UNFACD_COMMAND_ROUTER_MESSAGE_TYPE_H
#define UFSRVCLI_UNFACD_COMMAND_ROUTER_MESSAGE_TYPE_H

#include "command_protocol_message_type.h"
#include "unfacd_command_message_type.h"

typedef int (*UnfacdCommandMessageRoute)(UnfacdCommandMessage * command,
                                         CommandProtocolMessage *protocol_message);

#endif //UFSRVCLI_UNFACD_COMMAND_ROUTER_MESSAGE_TYPE_H
