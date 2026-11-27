#ifndef UF_UNFACD_MESSAGE_TYPE_H
#define UF_UNFACD_MESSAGE_TYPE_H

#include <stddef.h>
#include <stdint.h>

#include "unfacd_user_type.h"

typedef enum UnfacdMessageProtocol
{
  UNFACD_MESSAGE_PROTOCOL_UNKNOWN       = 0,
  UNFACD_MESSAGE_PROTOCOL_CIPHERTEXT    = 1,
  UNFACD_MESSAGE_PROTOCOL_KEY_EXCHANGE  = 2,
  UNFACD_MESSAGE_PROTOCOL_PREKEY_BUNDLE = 3
} UnfacdMessageProtocolType;

typedef struct UnfacdMessage
{
  uint64_t              fid;  ///< target group
  const unsigned char  *body;
  size_t                body_len;
  UnfacdMessageProtocolType protocol;
  uint64_t              ttl;
  int                   view_once;
  int owned_body;
  UnfacdUser          from; ///< message sender
} UnfacdMessage;

#endif
