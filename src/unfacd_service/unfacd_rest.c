/**
 * @file unfacd_rest.c
 * @brief unfacd_rest
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
#include <ufsrvclilib/unfacd_service/unfacd_rest.h>

#include <ufsrvclilib/net/ufsrvcli_http.h>

#include <stdlib.h>
#include <string.h>

static int
sPerform(const UfsrvCliHttpRequest *req, char **reply_out)
{
  UfsrvCliHttpResponse resp;

  if (UfsrvCliHttpPerform(req, &resp) != 0) return -1;

  if (resp.status < 200 || resp.status >= 300) {
    if (reply_out) *reply_out = resp.body;
    else UfsrvCliHttpResponseFree(&resp);
    return (int)resp.status;
  }

  if (reply_out) *reply_out = resp.body;
  else UfsrvCliHttpResponseFree(&resp);

  return 0;
}

int
UnfacdRestPost(const char *url, const char *cookie, const char *json_body,
               int verify_peer, long timeout_sec, char **reply_out)
{
  UfsrvCliHttpRequest req;

  if (!url || !json_body) return -1;
  if (reply_out) *reply_out = NULL;

  memset(&req, 0, sizeof req);
  req.method       = "POST";
  req.url          = url;
  req.body         = json_body;
  req.content_type = "application/json";
  req.cookie       = cookie;
  req.verify_peer  = verify_peer;
  req.timeout_sec  = timeout_sec;

  return sPerform(&req, reply_out);
}

int
UnfacdRestPostBasic(const char *url, const char *userpwd, const char *cookie,
                    const char *json_body, int verify_peer, long timeout_sec, char **reply_out)
{
  UfsrvCliHttpRequest req;

  if (!url || !json_body || !userpwd) return -1;
  if (reply_out) *reply_out = NULL;

  memset(&req, 0, sizeof req);
  req.method       = "POST";
  req.url          = url;
  req.body         = json_body;
  req.content_type = "application/json";
  req.userpwd      = userpwd;
  req.cookie       = cookie;
  req.verify_peer  = verify_peer;
  req.timeout_sec  = timeout_sec;

  return sPerform(&req, reply_out);
}

int
UnfacdRestGetBasic(const char *url, const char *userpwd, const char *cookie,
                   int verify_peer, long timeout_sec, char **reply_out)
{
  UfsrvCliHttpRequest req;

  if (!url || !userpwd) return -1;
  if (reply_out) *reply_out = NULL;

  memset(&req, 0, sizeof req);
  req.method      = "GET";
  req.url         = url;
  req.userpwd     = userpwd;
  req.cookie      = cookie;
  req.verify_peer = verify_peer;
  req.timeout_sec = timeout_sec;

  return sPerform(&req, reply_out);
}
