/**
 * @file ufsrv_cli_context_type.h
 * @brief ufsrv_cli_context_type
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

#ifndef UFSRVCLI_UFSRV_CLI_CONTEXT_TYPE_H
#define UFSRVCLI_UFSRV_CLI_CONTEXT_TYPE_H

#include <stdint.h>

#include <uflib/logger/logger.h>

#include <ufsrvclilib/net/ufsrvcli_conn.h>

#define UFSRVCLI_CONTEXT_URL_MAX     512
#define UFSRVCLI_CONTEXT_COOKIE_MAX  256
#define UFSRVCLI_CONTEXT_USERPWD_MAX 512

typedef void UserContextData;
#define AS_USER_CONTEXT_DATA(x) ((UserContextData *)x)

typedef struct UfsrvCliContext
{
  char          api_url[UFSRVCLI_CONTEXT_URL_MAX];         /*!< ufsrvapi base, e.g. https://host:port; no path. */
  char          api_cookie[UFSRVCLI_CONTEXT_COOKIE_MAX];   /*!< session cookie when basic auth is not used. */
  char          api_userpwd[UFSRVCLI_CONTEXT_USERPWD_MAX]; /*!< ufsrvuid:password; takes precedence. */
  int           api_verify_peer;                           /*!< 1 verifies the TLS peer, 0 accepts the dev cert. */
  UfsrvCliConn *websock;                                   /*!< consumer-owned websocket transport. */
  UfLogger *    logger;                                    ///< app-wide loggers
  UserContextData *user_context_data;                      ///< opaque user provided app-context for convenirence
} UfsrvCliContext;

#endif //UFSRVCLI_UFSRV_CLI_CONTEXT_TYPE_H
