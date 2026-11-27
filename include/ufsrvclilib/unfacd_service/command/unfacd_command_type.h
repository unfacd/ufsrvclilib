/**
 * @file unfacd_command_type.h
 * @brief unfacd_command_type
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

#ifndef UFSRVCLI_UNFACD_COMMAND_TYPE_H
#define UFSRVCLI_UNFACD_COMMAND_TYPE_H

#include "unfacd_command_type_enum.h"
#include "unfacd_command_header_type.h"

typedef void UnfacdCommandMessagePayload;

///< Common type base for all commands
typedef struct UnfacdCommand
{
  UnfacdCommandTypeEnum command_type;
  UnfacdCommandHeader command_header;
  UnfacdCommandMessagePayload  *command_message; ///< Aliased type for type-specific commands: Fence, Message etc...
} UnfacdCommand;

#endif //UFSRVCLI_UNFACD_COMMAND_TYPE_H
