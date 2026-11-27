# Change Log — ufsrvclilib

## Release-261127 — 0.1.0
`f7d8a5a` → `4914ad7` · 2026-09-04 → 2026-11-27

First mirror: the tree as of v0.1.0. There is no previous release to diff against, so this
entry describes what the library provides rather than a delta.

### New
- **Command routing** — Fence, Message and StateSync commands routed by registration, fence commands dispatched by type and arguments, native collections delivered through registered callbacks, and SAY acknowledgements correlated
- **ufsrvapi REST surface** — group and message commands, the account registration pipeline and handshake, join classification for group and fence joins, refusal of deliveries addressed to another subject, the server's cookie header, and fence records fetched over REST
- **WebSocket client** — a persistent connection mode over an epoll loop, uflib's RFC 6455 codec throughout, optional TLS for the opening handshake, and the encapsulated keepalive packet
- **Library shape** — shared and static builds with install, export and package metadata, version and configuration plumbing, and L2/L3 debug switches
- **Build** — vendored `protobuf-c`, the family preset and build-option system, and the CLI dev build script

### Fixes
- **Receive path** — the response container's request body and promise are released
- **Install** — superseded `.so` versions are pruned before `ldconfig`, and `FenceCommand` carries an identical error type
- **Build** — a build directory pinned to a scratch companion prefix is rejected, and text files are kept LF on checkout so the version generator's shebang survives

### Deprecated
- (none)

### Removed
- (none)
