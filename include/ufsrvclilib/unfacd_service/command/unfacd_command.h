/**
 * @file unfacd_command.h
 * @brief Register protocol command routers and dispatch decoded or wire-format WebSocket commands.
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

#ifndef UF_UNFACD_COMMAND_H
#define UF_UNFACD_COMMAND_H

#include <uflib/uflib_defs.h>

#include <ufsrvclilib/unfacd_service/command/command_protocol_message_type.h>
#include <ufsrvclilib/unfacd_service/command/unfacd_command_router_type.h>
#include <ufsrvclilib/unfacd_service/command/unfacd_command_type_enum.h>

#include "ufsrvclilib/unfacd_service/unfacd_message_type.h"

/**
 * @brief Register a callback array for one fixed protocol command type.
 *
 * @param[in] command_routes Non-NULL array of callbacks with the signature required
 *                           by command_type. StateSync and keepalive use slot zero;
 *                           fence and message arrays use their command subtype indexes.
 * @param[in] command_type Protocol enum slot to register, below UnfacdCommandTypeMax.
 * @return 0 when the array replaces the registered array for this slot.
 * @return -1 for a NULL array or an out-of-range enum value.
 * @pre Register before dispatch starts; registration is not synchronized.
 * @note The array is borrowed, not copied. Keep it valid for all dispatches and size
 *       it for every subtype the protocol can deliver; array bounds are not checked.
 *
 * @code{.c}
 * #include <ufsrvclilib/unfacd_service/command/unfacd_command_router_message_type.h>
 *
 * int OnKeepalive(UnfacdCommandMessage *command, CommandProtocolMessage *message);
 * static UnfacdCommandMessageRoute routes[] = {OnKeepalive};
 *
 * int RegisterKeepalive(void)
 * {
 *   return UnfacdCommandRegisterRouter(routes, UnfacdCommandTypeKeepalive);
 * }
 * @endcode
 */
PUBLIC_API int UnfacdCommandRegisterRouter(UnfacdCommandRouter *command_routes, UnfacdCommandTypeEnum command_type);

/**
 * @brief Dispatch a decoded command descriptor through its protocol enum slot.
 *
 * @param[in,out] protocol_message Descriptor containing a registered command_uri,
 *                                origin, borrowed payload and caller context.
 * @return The selected callback's result, or 0 for an ignored command/subtype.
 * @return -1 for an invalid descriptor, unknown URI, required payload missing or
 *         undecodable, or a URI that disagrees with the enclosed wire-command type.
 * @pre Register the applicable callback arrays before calling this function.
 * @note StateSync carries JSON; fence/message carry a serialized Envelope. Keepalive
 *       may have no body and uses origin, request_id and response_status instead.
 *       A missing wire ID or status is represented by zero in a decoded descriptor.
 * @note Caller ownership is retained. Temporary decoded records passed to callbacks
 *       are valid only during the call; copy data that must outlive dispatch.
 *
 * @code{.c}
 * CommandProtocolMessage reply = {0};
 * reply.command_uri = "/v1/keepalive";
 * reply.origin = WS_MSGR_ORIGIN_RESPONSE;
 * reply.request_id = 1;
 * reply.response_status = 200;
 * int result = UnfacdCommandRoute(&reply);
 * @endcode
 */
PUBLIC_API int UnfacdCommandRoute(CommandProtocolMessage *protocol_message);

/**
 * @brief Unpack a deframed WebSocketMessage, copy its metadata and route it to user-registered command router.
 *
 * @param[in] protocol_message Non-NULL carrier whose protocol_message_envelope.data
 *                            and size describe one complete serialized WebSocketMessage.
 *                            user_data is forwarded to the selected callback.
 * @return Nothing; malformed protobuf and nonzero routing results are logged.
 * @pre Initialize the CLI context/logger and register the protocol callback arrays.
 * @note Input excludes WebSocket frame headers and masking. Request path/body or
 *       response message/body supplies the command payload; status-only replies are
 *       dispatched without a body. Absent request IDs and response statuses become zero.
 * @note Input bytes and user_data remain caller-owned. The unpacked WebSocketMessage
 *       is freed after synchronous dispatch; callbacks must copy borrowed payloads
 *       they need to retain. Fence and message callbacks receive native record collections and metadata;
 *       both binary buffer descriptors are empty. Use UnfacdGroupCopy() to retain
 *       a fence's group and UnfacdGroupClear() to release the retained collections.
 *       Use UnfacdMessageCopy()/UnfacdMessageClear() for retained message bodies.
 *
 * @code{.c}
 * void OnWebSocketPayload(const unsigned char *data, size_t size, void *context)
 * {
 *   CommandProtocolMessage carrier = {0};
 *   carrier.protocol_message_envelope.data = (char *)data;
 *   carrier.protocol_message_envelope.size = size;
 *   carrier.user_data = context;
 *   UnfacdCommandProcessProtocolMessage(&carrier);
 * }
 * @endcode
 */
PUBLIC_API void UnfacdCommandProcessProtocolMessage(CommandProtocolMessage *protocol_message);

#endif
