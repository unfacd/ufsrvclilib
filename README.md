# ufsrvclilib

The C client library for the unfacd messaging network. It provides protocol and messaging abstractions SDK. The 
cli `ufsrvcli` is developed on its back. It is WIP and will evolve primarily as a integration testing control plane 
until further notice.


## What it provides

- **Command routing** — Fence, Message and StateSync commands 
- **ufsrvapi REST surface** — group and message commands, the account
  registration pipeline and handshake
- **WebSocket client** — a persistent connection mode over an epoll loop, uflib's
  RFC 6455 codec throughout, optional TLS for the opening handshake, and the
  encapsulated keepalive packet.

`packages/protobuf-c` is vendored, with its own licence and provenance note.

## Build

```bash
cmake --preset dev && cmake --build --preset dev
```

## Licence

No licence file is present at the repository root yet; the vendored
`protobuf-c` keeps its own.
