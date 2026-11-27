/**
 * @file unfacd_comand.c
 * @brief unfacd_comand
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

#include <ufsrvclilib/unfacd_service/command/unfacd_command.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <uflib/logger/logger.h>

#include <ufsrvclilib/unfacd_service/command/unfacd_command_router_fence_type.h>
#include <ufsrvclilib/unfacd_service/command/unfacd_command_router_message_type.h>
#include <ufsrvclilib/unfacd_service/unfacd_message_type.h>
#include <ufsrvclilib/unfacd_service/unfacd_group.h>
#include <ufsrvclilib/unfacd_service/unfacd_message.h>

#include <unfacd_service/UnfacdService.pb-c.h>

#include "ufsrvcli_context_priv.h"
#include "ufsrvclilib/ufsrvcli_context_type.h"
#include "unfacd_service/command/unfacd_command_utils.h"
#include "websocket/WebSocketMessage.pb-c.h"

static UnfacdCommandRouter *unfacd_command_router_table[UnfacdCommandTypeMax];

// IMPORTANT MUST BE INDEXED ON ENUM
static const char *const sCommandRoutes[UnfacdCommandTypeMax] = {
  [UfCommandTypeStateSync] = "/v1/StateSync", [UnfacdCommandTypeKeepalive]  = "/v1/keepalive",
  [UnfacdCommandTypeFence] = "ufsrv://v1/Fence", [UnfacdCommandTypeMessage] = "ufsrv://v1/Message",
};

int
UnfacdCommandRegisterRouter(UnfacdCommandRouter *command_routes, UnfacdCommandTypeEnum command_type)
{
  if (!command_routes) return -1;
  if ((unsigned)command_type >= UnfacdCommandTypeMax) return -1;

  unfacd_command_router_table[command_type] = command_routes;

  return 0;
}

/**
 * @brief Build the lib scaffolding around the through-the-wire received FenceMessage command, providing
 * protocol-free abstraction.
 *
 * @param command
 * @param protocol_message
 * @return
 */
static int
sRouteFence(const FenceCommand *command, CommandProtocolMessage *protocol_message)
{
  if (!command || !command->header) return -1;

  UnfacdCommandFenceRoute *routes = unfacd_command_router_table[UnfacdCommandTypeFence];
  if (!routes) return -1;

  uint32_t type = command->header->command;
  if (type >= UNFACD_FENCE_COMMAND_MAX) return -1;

  UnfacdCommandFenceRoute route = routes[type];
  if (!route) return 0;

  if (command->n_fences && !command->fences) return -1;

  UnfacdCommandHeader header = {0};
  if (UnfacdCommandFillHeader(&header, command->header) != 0) return -1;

  UnfacdCommandFence fence                  = {0};
  fence.unfacd_command_base.command_type    = UnfacdCommandTypeFence;
  fence.unfacd_command_base.command_header  = header;
  fence.unfacd_command_base.command_message = &fence.fence_records;
  fence.operation                           = (UnfacdFenceOperation)type;
  size_t count                              = command->n_fences;
  if (count > SIZE_MAX / sizeof(void*)) return -1;
  if (count) {
    fence.fence_records.collection = calloc(count, sizeof(void*));
    if (!fence.fence_records.collection) return -1;
  }
  int result = -1;
  if (!count && UnfacdFenceOutcomeIsSuccess(fence.operation, header.outcome)) goto cleanup;
  for (size_t index = 0; index < count; ++index) {
    CommandFenceRecord *record = calloc(1, sizeof *record);
    if (!record) goto cleanup;
    fence.fence_records.collection[index] = record;
    ++fence.fence_records.collection_sz;
    if (!command->fences[index] || UnfacdCommandFillFenceGroup(&record->group, command->fences[index]) != 0) goto
      cleanup;
    if (!record->group.fid) record->group.fid = header.fid;
    if (fence.operation == UNFACD_FENCE_JOIN && UnfacdFenceOutcomeIsSuccess(fence.operation, header.outcome) && !record
      ->group.fid) goto cleanup;
  }
  CommandProtocolMessage metadata = *protocol_message;
  result = route(&fence, &metadata);
cleanup:
  for (size_t index = 0; index < fence.fence_records.collection_sz; ++index) {
    CommandFenceRecord *record = fence.fence_records.collection[index];
    UnfacdGroupClear(&record->group);
    free(record);
  }
  free(fence.fence_records.collection);
  return result;
}

static int
sRouteMessage(const MessageCommand *command, CommandProtocolMessage *protocol_message)
{
  if (!command || !command->header) return -1;
  UnfacdCommandMessageRoute *routes = unfacd_command_router_table[UnfacdCommandTypeMessage];
  unsigned                   type   = command->header->command;
  if (!routes || type >= UNFACD_MESSAGE_COMMAND_MAX) return -1;
  UnfacdCommandMessageRoute route = routes[type];
  if (!route) return 0;
  if ((command->n_messages && !command->messages) || (command->n_fences && !command->fences)) return -1;
  if (command->n_messages > SIZE_MAX / sizeof(void*) || command->n_fences > SIZE_MAX / sizeof(void*)) return -1;
  UnfacdCommandMessage message                = {0};
  message.operation                           = (UnfacdMessageOperation)type;
  message.unfacd_command_base.command_type    = UnfacdCommandTypeMessage;
  message.unfacd_command_base.command_message = &message.message_records;
  UnfacdCommandHeader *header                 = &message.unfacd_command_base.command_header;
  if (UnfacdCommandFillHeader(header, command->header) != 0) return -1;
  if (command->originator && UnfacdCommandFillUser(&message.originator, command->originator) != 0) return -1;
  int result = -1;
  if (command->n_fences) {
    message.fence_records.collection = calloc(command->n_fences, sizeof(void*));
    if (!message.fence_records.collection) goto cleanup;
  }
  for (size_t i = 0; i < command->n_fences; ++i) {
    CommandFenceRecord *record = calloc(1, sizeof *record);
    if (!record) goto cleanup;
    message.fence_records.collection[i] = record;
    ++message.fence_records.collection_sz;
    if (!command->fences[i] || UnfacdCommandFillFenceGroup(&record->group, command->fences[i]) != 0) goto cleanup;
    if (!record->group.fid) record->group.fid = header->fid;
    if (header->fid && record->group.fid && record->group.fid != header->fid) goto cleanup;
  }
  uint64_t fid = header->fid;
  if (!fid && message.fence_records.collection_sz == 1) fid = ((CommandFenceRecord*)message.fence_records.collection[0])
                                                              ->group.fid;
  if (command->n_messages) {
    message.message_records.collection = calloc(command->n_messages, sizeof(void*));
    if (!message.message_records.collection) goto cleanup;
  }
  for (size_t i = 0; i < command->n_messages; ++i) {
    CommandMessageRecord *record = calloc(1, sizeof *record);
    if (!record) goto cleanup;
    message.message_records.collection[i] = record;
    ++message.message_records.collection_sz;
    if (!command->messages[i] || UnfacdCommandFillMessageRecord(&record->message, command->messages[i], fid,
                                                                &message.originator) != 0) goto cleanup;
  }
  CommandProtocolMessage metadata = *protocol_message;
  memset(&metadata.protocol_message_envelope, 0, sizeof metadata.protocol_message_envelope);
  result = route(&message, &metadata);
cleanup:
  for (size_t i = 0; i < message.message_records.collection_sz; ++i) {
    CommandMessageRecord *record = message.message_records.collection[i];
    UnfacdMessageClear(&record->message);
    free(record);
  }
  for (size_t i = 0; i < message.fence_records.collection_sz; ++i) {
    CommandFenceRecord *record = message.fence_records.collection[i];
    UnfacdGroupClear(&record->group);
    free(record);
  }
  free(message.message_records.collection);
  free(message.fence_records.collection);
  return result;
}

static int
sRouteStateSync(const MessageCommand *command, CommandProtocolMessage *protocol_message)
{
  UnfacdCommandMessage       message = {0};
  UnfacdCommandMessageRoute *routes  = unfacd_command_router_table[UfCommandTypeStateSync];
  UnfacdCommandMessageRoute  route;

  (void)command;

  if (!routes) return -1;

  route = routes[0];
  if (!route) return 0;

  return route(&message, protocol_message);
}

static int
sRouteKeepalive(CommandProtocolMessage *protocol_message)
{
  UnfacdCommandMessage       message = {0};
  UnfacdCommandMessageRoute *routes  = unfacd_command_router_table[UnfacdCommandTypeKeepalive];
  if (!routes) return -1;
  return routes[0] ? routes[0](&message, protocol_message) : 0;
}

int
UnfacdCommandRoute(CommandProtocolMessage *protocol_message)
{
  if (!protocol_message || !protocol_message->command_uri) return -1;
  UnfacdCommandTypeEnum type = UnfacdCommandTypeMax;
  for (unsigned i = 0; i < UnfacdCommandTypeMax; ++i) {
    if (sCommandRoutes[i] && strcmp(protocol_message->command_uri, sCommandRoutes[i]) == 0) {
      type = (UnfacdCommandTypeEnum)i;
      break;
    }
  }
  if (type == UnfacdCommandTypeMax) return -1;
  if (type == UnfacdCommandTypeKeepalive) return sRouteKeepalive(protocol_message);

  BufferDescriptor *buffer = &protocol_message->protocol_message_envelope;
  if (!buffer->data || buffer->size == 0) return -1;
  if (type == UfCommandTypeStateSync) return sRouteStateSync(NULL, protocol_message);

  Envelope *envelope = envelope__unpack(NULL, buffer->size, (const uint8_t*)buffer->data);
  if (!envelope) return -1;
  UfsrvCommandWire *wire = envelope->ufsrvcommand;
  int               rc   = -1;
  if (wire && type == UnfacdCommandTypeFence && wire->ufsrvtype == UFSRV_COMMAND_WIRE__UFSRV_TYPE__UFSRV_FENCE)
    rc = sRouteFence(wire->fencecommand, protocol_message);
  else if (wire && type == UnfacdCommandTypeMessage && wire->ufsrvtype == UFSRV_COMMAND_WIRE__UFSRV_TYPE__UFSRV_MESSAGE)
    rc = sRouteMessage(wire->msgcommand, protocol_message);
  envelope__free_unpacked(envelope, NULL);
  return rc;
}

static const char *
sMessageTypeName(WebSocketMessage__Type type)
{
  switch (type) {
  case WEB_SOCKET_MESSAGE__TYPE__REQUEST: return "REQUEST";
  case WEB_SOCKET_MESSAGE__TYPE__RESPONSE: return "RESPONSE";
  default: return "UNKNOWN";
  }
}

static void
sLogMessage(UfLogger *logger, const WebSocketMessage *msg)
{
  size_t i;

  UF_LOGGER_INFO(logger, "websocket message: type=%s has_type=%d command=%s request=%s response=%s",
                 sMessageTypeName(msg->type), msg->has_type, msg->command ? msg->command : "",
                 msg->request ? "yes" : "no", msg->response ? "yes" : "no");
  if (msg->request)
    UF_LOGGER_INFO(logger, "  request: path=%zu bytes body=%zu bytes",
                 msg->request->path ? strlen(msg->request->path) : 0u, (size_t)msg->request->body.len);
  if (msg->response)
    UF_LOGGER_INFO(logger, "  response: message=%zu bytes body=%zu bytes",
                 msg->response->message ? strlen(msg->response->message) : 0u, (size_t)msg->response->body.len);
  for (i = 0; i < msg->n_headers; i++)
    UF_LOGGER_INFO(logger, "  header: %s", msg->headers[i]);
}

/* Where ufsrvwebsock puts a command's JSON: _COMMAND_REQUEST writes it to the
 * request's path, _COMMAND_RESPONSE to the response's message. A body, when a
 * caller uses one, is the fallback. */
static const char *
sProvideWebSocketMessagePayload(const WebSocketMessage *msg, size_t *len_out)
{
  if (msg->request) {
    if (msg->request->path && msg->request->path[0]) {
      *len_out = strlen(msg->request->path);
      return msg->request->path;
    }
    if (msg->request->body.len > 0) {
      *len_out = msg->request->body.len;
      return (const char*)msg->request->body.data;
    }
  }
  if (msg->response) {
    if (msg->response->message && msg->response->message[0]) {
      *len_out = strlen(msg->response->message);
      return msg->response->message;
    }
    if (msg->response->body.len > 0) {
      *len_out = msg->response->body.len;
      return (const char*)msg->response->body.data;
    }
  }
  return NULL;
}

void
UnfacdCommandProcessProtocolMessage(CommandProtocolMessage *protocol_message)
{
  const char *payload     = NULL;
  size_t      payload_len = 0;
  UfLogger *  logger      = UfsrvCliContextInstance()->logger;

  WebSocketMessage *msg = web_socket_message__unpack(NULL, protocol_message->protocol_message_envelope.size,
                                                     (const uint8_t*)protocol_message->protocol_message_envelope.data);
  if (IS_EMPTY(msg)) {
    UF_LOGGER_ERROR(logger, "websocket: payload is not a WebSocketMessage");
    return;
  }

  sLogMessage(logger, msg);

  //  The actual command payload as saved  in WebSocket (another packed protobuf)
  payload = sProvideWebSocketMessagePayload(msg, &payload_len);

  CommandProtocolMessage protocol_message_envelope = {0};
  protocol_message_envelope.origin                 = msg->type == WEB_SOCKET_MESSAGE__TYPE__REQUEST ?
                                                       WS_MSGR_ORIGIN_REQUEST :
                                                       WS_MSGR_ORIGIN_RESPONSE;
  protocol_message_envelope.command_uri = msg->command; // "/v1/StateSync", "ufsrv://v1/Message", "ufsrv://v1/Fence"
  protocol_message_envelope.protocol_message_envelope.data = (char*)payload;
  //maybe json str direct (as in statesync or binary protobuf Envelope)
  protocol_message_envelope.protocol_message_envelope.size = payload_len;

  if (msg->response && msg->type == WEB_SOCKET_MESSAGE__TYPE__RESPONSE) {
    protocol_message_envelope.request_id      = msg->response->has_id ? msg->response->id : 0;
    protocol_message_envelope.response_status = msg->response->has_status ? msg->response->status : 0;
  }
  else if (msg->request && msg->type == WEB_SOCKET_MESSAGE__TYPE__REQUEST) {
    protocol_message_envelope.request_id = msg->request->has_id ? msg->request->id : 0;
  }
  protocol_message_envelope.user_data = protocol_message->user_data;

  if (UnfacdCommandRoute(&protocol_message_envelope) != 0)
    UF_LOGGER_WARN(logger, "websocket: command route rejected %s", msg->command ? msg->command : "");

  web_socket_message__free_unpacked(msg, NULL);
}
