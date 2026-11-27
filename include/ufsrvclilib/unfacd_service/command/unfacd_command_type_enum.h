//
// Created by ayman on 11/11/2026.
//

/**
 * @file unfacd_command_type_enum.h
 * @brief unfacd_command_type_enum
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

#ifndef UFSRVCLI_UNFACD_COMMAND_TYPE_ENUM_H
#define UFSRVCLI_UNFACD_COMMAND_TYPE_ENUM_H

typedef enum UnfacdCommandTypeEnum
{
  UfCommandTypeStateSync = 0,
  UnfacdCommandTypeKeepalive,
  UnfacdCommandTypeFence,
  UnfacdCommandTypeMessage,

  UnfacdCommandTypeMax
} UnfacdCommandTypeEnum;

#endif //UFSRVCLI_UNFACD_COMMAND_TYPE_ENUM_H
