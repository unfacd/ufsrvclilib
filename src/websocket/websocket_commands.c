/**
 * @file websocket_commands.c
 * @brief Client commands for the ufsrvwebsock protocol, packed for the wire.
 *
 * Copyright (C) 2015-2026  unfacd works
 *
 * This file is part of ufsrvcli source code.
 * Created by ayman on 19/10/2026.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */
#include <ufsrvclilib/websocket/uf_websocket_commands.h>

#include <stdlib.h>
#include <string.h>

#include <uflib/utils_crypto.h>

#include <uflib/msg_framing/websocket/ws_frame.h>

#include <websocket/WebSocketMessage.pb-c.h>

#define UF_WEBSOCKET_KEEPALIVE_COMMAND "/v1/keepalive"

/* One keepalive message is a few dozen bytes; the payload buffer never grows. */
#define UF_WEBSOCKET_KEEPALIVE_PAYLOAD_CAP 128u

/* Initial capacity of a backing buffer the builder has to allocate itself. */
#define UF_WEBSOCKET_COMMAND_FRAME_CAP 128u

static size_t
sFrameLength(size_t payload_len)
{
  if (payload_len < 126u) return 2u + 4u + payload_len;
  if (payload_len <= 0xffffu) return 4u + 4u + payload_len;
  return 10u + 4u + payload_len;
}

/* Guarantee room for frame_len bytes plus the trailing NUL. */
static int
sEnsureSpace(BufferDescriptor *bd, size_t frame_len)
{
  char  *grown;
  size_t cap;

  if (bd->data && bd->size_max > frame_len) return 0;

  cap = bd->size_max;
  if (cap <= frame_len) cap = frame_len + 1u;
  if (cap < UF_WEBSOCKET_COMMAND_FRAME_CAP) cap = UF_WEBSOCKET_COMMAND_FRAME_CAP;

  grown = realloc(bd->data, cap);
  if (!grown) return -1;

  bd->data     = grown;
  bd->size_max = cap;
  return 0;
}

BufferDescriptor *
UfWebSocketCommandBuildKeepalive(uint64_t request_id, BufferDescriptor *out)
{
  unsigned char           payload[UF_WEBSOCKET_KEEPALIVE_PAYLOAD_CAP];
  unsigned char           mask[4];
  WebSocketRequestMessage request = WEB_SOCKET_REQUEST_MESSAGE__INIT;
  WebSocketMessage        message = WEB_SOCKET_MESSAGE__INIT;
  BufferDescriptor       *bd      = out;
  size_t                  payload_len;
  int                     own     = 0;
  int                     frame_len;

  if (!bd) {
    bd = calloc(1, sizeof *bd);
    if (!bd) return NULL;
    own = 1;
  }

  request.has_id = 1;
  request.id     = request_id;

  message.has_type = 1;
  message.type     = WEB_SOCKET_MESSAGE__TYPE__REQUEST;
  message.command  = (char *)UF_WEBSOCKET_KEEPALIVE_COMMAND;
  message.request  = &request;

  payload_len = web_socket_message__get_packed_size(&message);
  if (payload_len > sizeof payload) goto fail;
  if (web_socket_message__pack(&message, payload) != payload_len) goto fail;

  if (sEnsureSpace(bd, sFrameLength(payload_len)) < 0) goto fail;
  if (GenerateSecureRandom(mask, sizeof mask) < 0) goto fail;

  frame_len = (int)WsFrameEncode((unsigned char *)bd->data, bd->size_max, payload, payload_len,
                                WS_OPCODE_BINARY, 1, mask);
  if (frame_len < 0) goto fail;

  bd->size           = (size_t)frame_len;
  bd->data[bd->size] = '\0';
  return bd;

fail:
  if (own) {
    BufferDescriptorRelease(bd);
    free(bd);
  }
  return NULL;
}
