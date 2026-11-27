//
// Created by ayman on 11/11/2026.
//

/**
 * @file command_protocol_message_type.h
 * @brief command_protocol_message_type
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

#ifndef UFSRVCLI_COMMAND_PROTOCOL_MESSAGE_TYPE_H
#define UFSRVCLI_COMMAND_PROTOCOL_MESSAGE_TYPE_H

#include <uflib/buffer_descriptor/buffer_descriptor_type.h>
#include <stdbool.h>
#include <stdint.h>

enum UfsrvWebsocketMessageOrigin
{
  WS_MSGR_ORIGIN_UNKNOWN = 0,
  WS_MSGR_ORIGIN_REQUEST,
  WS_MSGR_ORIGIN_RESPONSE
};

typedef struct CommandProtocolMessage
{
  enum UfsrvWebsocketMessageOrigin origin; ///< WebsSocket message direction request vs response
  const char *command_uri; ///< As provided in WebSocket "/v1/StateSync", "ufsrv://v1/Message", "ufsrv://v1/Fence"
  BufferDescriptor protocol_message_envelope; ///< Actual command Envelope
  uint64_t request_id;
  uint32_t response_status;
  void *user_data; ///< for example rx which is passed to user back
} CommandProtocolMessage;

#endif //UFSRVCLI_COMMAND_PROTOCOL_MESSAGE_TYPE_H
