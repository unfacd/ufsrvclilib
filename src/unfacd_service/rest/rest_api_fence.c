/**
 * @file rest_api_fence.c
 * @brief rest_api_fence
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
#include <ufsrvclilib/unfacd_service/rest/rest_api_fence.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uflib/mjson/mjson.h>

#include "ufsrvcli_context_priv.h"
#include <ufsrvclilib/unfacd_service/unfacd_rest.h>

static int
sJsonUInt64(const char *json, int len, const char *path, uint64_t *out)
{
  const char *token     = NULL;
  int         token_len = 0;
  char        digits[24];

  if (mjson_find(json, len, path, &token, &token_len) != MJSON_TOK_NUMBER) return -1;
  if (!token || token_len <= 0 || (size_t)token_len >= sizeof digits) return -1;

  for (int i = 0; i < token_len; i++) {
    if (token[i] < '0' || token[i] > '9') return -1;
  }

  memcpy(digits, token, (size_t)token_len);
  digits[token_len] = '\0';
  *out              = strtoull(digits, NULL, 10);

  return 0;
}

PUBLIC_API UnfacdGroup *
UnfacdRestGroupInfo(UnfacdGroupDescriptor *descriptor, UnfacdGroup *provided)
{
  UfsrvCliContext *ctx   = UfsrvCliContextInstance();
  UnfacdGroup     *group = provided;
  char            *reply = NULL;
  char             url[UFSRVCLI_CONTEXT_URL_MAX + 64];
  uint64_t         value;
  size_t           reply_len;
  int              url_len;

  if (!descriptor || descriptor->fid == 0) return NULL;
  if (!ctx || !ctx->api_url[0] || !ctx->api_userpwd[0]) return NULL;

  if (!group) {
    group = calloc(1, sizeof *group);
    if (!group) return NULL;
  }

  url_len = snprintf(url, sizeof url, "%s/V1/Fence/Info/%zu", ctx->api_url, descriptor->fid);
  if (url_len < 0 || (size_t)url_len >= sizeof url) goto failed;

  if (UnfacdRestGetBasic(url, ctx->api_userpwd, ctx->api_cookie, ctx->api_verify_peer, 0, &reply) != 0) goto failed;

  reply_len = strlen(reply);
  if (reply_len > INT_MAX) goto failed;

  if (sJsonUInt64(reply, (int)reply_len, "$.fid", &value) != 0) goto failed;
  group->fid = value;

  if (sJsonUInt64(reply, (int)reply_len, "$.eid", &value) == 0) group->eid = value;
  if (sJsonUInt64(reply, (int)reply_len, "$.when_created", &value) == 0) group->created = value;

  if (sJsonUInt64(reply, (int)reply_len, "$.members_count", &value) == 0) {
    if (value > UINT32_MAX) goto failed;
    group->usercount = (uint32_t)value;
  }

  if (mjson_get_string(reply, (int)reply_len, "$.cname", group->cname, (int)sizeof group->cname) < 0)
    group->cname[0] = '\0';
  if (mjson_get_string(reply, (int)reply_len, "$.dname", group->fname, (int)sizeof group->fname) < 0)
    group->fname[0] = '\0';

  free(reply);

  return group;

failed:
  free(reply);
  if (!provided) free(group);

  return NULL;
}
