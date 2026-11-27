/**
 * @file unfacd_group_type_priv.h
 * @brief unfacd_group_type_priv
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

#ifndef UFSRVCLI_UNFACD_USER_TYPE_H
#define UFSRVCLI_UNFACD_USER_TYPE_H

#include <uflib/ufsrvuid.h>

typedef struct UnfacdUser
{
  UfsrvUid ufsrvuid;
  char nickname[32];
  char username[256];
} UnfacdUser;

#endif //UFSRVCLI_UNFACD_USER_TYPE_H
