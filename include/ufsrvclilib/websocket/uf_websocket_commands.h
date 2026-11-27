/**
 * @file uf_websocket_commands.h
 * @brief Client commands for the ufsrvwebsock protocol, ready for the wire.
 *
 * Each builder produces one complete client-to-server packet: the RFC 6455
 * binary frame (masked, as clients must mask) wrapping a packed
 * WebSocketMessage. The caller hands the descriptor's bytes straight to its
 * socket — no framing or protobuf work is left to the caller.
 */
#ifndef UF_WEBSOCKET_COMMANDS_H
#define UF_WEBSOCKET_COMMANDS_H

#include <uflib/uflib_defs.h>
#include <uflib/buffer_descriptor/buffer_descriptor.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Build a /v1/keepalive request packet.
 *
 * The packet is a masked binary WebSocket frame whose payload is
 * WebSocketMessage{type=REQUEST, command="/v1/keepalive", request{id}}.
 * A fresh random mask is drawn per call.
 *
 * @param request_id value of WebSocketRequestMessage.id.
 * @param out        Descriptor to fill, always last. When NULL the builder
 *                   calloc()s one, together with its backing buffer.
 *
 * @return the descriptor holding the frame, or NULL on failure. On success
 *         out->size is the frame length and out->data is NUL-terminated at
 *         out->size.
 *
 * Ownership:
 *  - caller-supplied descriptor: the backing buffer is grown (realloc) when it
 *    is missing or too small. Nothing has to be freed by the builder; the
 *    caller keeps ownership and releases it as usual.
 *  - calloc()ed descriptor: the caller releases it with
 *    BufferDescriptorRelease() followed by free().
 *
 * @code{.c}
 * BufferDescriptor *bd = UfWebSocketCommandBuildKeepalive(request_id, NULL);
 * if (bd) {
 *     send(sock, bd->data, bd->size, 0);
 *     BufferDescriptorRelease(bd);
 *     free(bd);
 * }
 * @endcode
 */
PUBLIC_API BufferDescriptor *UfWebSocketCommandBuildKeepalive(uint64_t request_id, BufferDescriptor *out);

#endif
