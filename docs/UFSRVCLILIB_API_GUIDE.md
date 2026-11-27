# ufsrvclilib — API Guide

The companion library owns every wire-level concern for the ufsrv class it talks to:
message shaping, transport, endpoint composition and the coroutine/future plumbing.
A consumer (the CLI, a script, a test) supplies handles and credentials, calls one
entry point, and reads back a typed result. It never frames, parses or assembles a
command itself.

## Shape

```
consumer                          ufsrvclilib                         server
--------                          -----------                         ------
UfsrvCliContext  ──provider()──►  (singleton)  ── UnfacdRest* ──────►  ufsrvapi  /V1/...
UfsrvCliConn     ──websock─────►  net/         ── UfWebSocketCommand* ► ufsrvwebsock
                                  unfacd_service/  (types, builders,
                                                    Sync + Rx entry points)
```

Four rules hold the shape together:

| rule | consequence |
|---|---|
| The library reads its handles from one consumer-provided context | no globals, no config parsing inside the commands |
| Types describe the wire; commands return typed results | `UnfacdGroup`, `UnfacdStateSync`, `UnfacdStateSyncGroup` — never raw protobuf |
| A command whose answer arrives out of band exists twice: `…Sync` and `…Rx` | the caller picks blocking or awaitable, the wire work is identical |
| A command whose reply *is* the subject returns it typed, blocking | no `…Rx` twin: `UnfacdRestGroupInfo()` |

## Key seams

| seam | header | role |
|---|---|---|
| Context | `include/ufsrvclilib/ufsrvcli_context_type.h` | `UfsrvCliContext`: api base URL, cookie, `ufsrvuid:password`, verify-peer, websock handle. Owned by the consumer, installed once with `UfsrvCliContextProvider()`. The accessor `UfsrvCliContextInstance()` is library-private. |
| Transport | `include/ufsrvclilib/net/ufsrvcli_conn.h` | `UfsrvCliConnOpen`/`OpenTls`, `SendAll`, `Recv`, `HasPending`. Consumer-owned; the library never opens it on the consumer's behalf. |
| REST | `include/ufsrvclilib/unfacd_service/unfacd_rest.h` | `UnfacdRestPost`, `UnfacdRestPostBasic`, `UnfacdRestGetBasic` — synchronous, `curl_easy_perform`. Endpoints are composed by the command, from the context's base URL. |
| Service types | `include/ufsrvclilib/unfacd_service/*_type.h` | `UnfacdGroup`, `UnfacdGroupDescriptor`, `UnfacdMessage`, `UnfacdStateSync`, `UnfacdStateSyncGroup`, `UnfacdStateSyncMessage`, `UnfacdStateSyncRxContext`. Types only — no behaviour. |
| Commands | `include/ufsrvclilib/unfacd_service/unfacd_group.h`, `unfacd_message.h` | The Sync and Rx entry points, plus the Rx delivery hook. |
| REST commands | `include/ufsrvclilib/unfacd_service/rest/rest_api_fence.h` | `UnfacdRestGroupInfo()` — one blocking GET whose reply is the subject; returns a filled `UnfacdGroup`. |
| Websocket builders | `include/ufsrvclilib/websocket/uf_websocket_commands.h` | Ready-to-send frames (`UfWebSocketCommandBuildKeepalive`). |

## Join — request/response flow

The POST and the answer travel on different paths: the POST is acknowledged
synchronously, the group itself comes back out of band on the websocket.

```
 consumer                     ufsrvclilib                    ufsrvapi          ufsrvwebsock
    │                             │                             │                  │
    │ UnfacdGroupJoinRx(&group,0) │                             │                  │
    ├────────────────────────────►│ build FenceCommand(JOIN)    │                  │
    │                             ├── promise/future + coroutine│                  │
    │◄── state_sync_group ────────┤    (parked on the future)   │                  │
    │      .rx.request_body       │                             │                  │
    │                             │                             │                  │
    │ POST .rx.request_body       │                             │                  │
    ├───────────── (consumer) ────┼────────────────────────────►│ /V1/Fence        │
    │                             │                             ├─ intra ─────────►│
    │◄──────────── 200 {"needsSync": false} ────────────────────┤  (no group here) │
    │                             │                             │                  │
    │                             │                             │   fence created  │
    │                             │                             │◄── broadcast ────┤
    │ WS msg: ufsrv://v1/Fence    │                             │                  │
    │ (Envelope, FenceRecord)     │                             │                  │
    ├────────────────────────────►│ UnfacdGroupJoinRxDeliver    │                  │
    │                             │  · fname match? → else 1    │                  │
    │                             │  · fill state_sync_header   │                  │
    │                             │  · fill *group              │                  │
    │                             │  · promise set + resume     │                  │
    │◄── rx.resolved == 1 ────────┤                             │                  │
    │      group->fid, fname, …   │                             │                  │
```

Ordering facts that shape the API:

| fact | consequence |
|---|---|
| `curl_easy_perform` returns before the server has marshalled the confirmation | the Sync variant cannot return the group; it returns the POST reply only |
| the confirmation is a separate websocket message (`FenceCommand`, `args=CREATED`) | only `…RxDeliver` can fill the subject, and only the socket owner can call it |
| a session-instate `/v1/StateSync` request may precede the confirmation | a client must tolerate it before it sees the fence message |
| delivery for another fence can arrive on the same socket | `…RxDeliver` returns `1` for a non-matching subject and resolves nothing |
| a join of an existing name by name yields **no** message | the caller must bound the wait; silence is not success |

The same shape holds for `UnfacdGroupMessageRx`, with `MessageCommand` in place of
`FenceCommand` and the message's `fid` as the match key.

## Fence info — the reply is the subject

Join and leave wait for an answer that arrives out of band. `UnfacdRestGroupInfo()`
does not: the reply *is* the fence, so the command parses it and returns a typed
group in one blocking call. There is no `…Rx` twin, because there is nothing to await.

```
 consumer                          ufsrvclilib                       ufsrvapi
    │                                  │                               │
    │ UnfacdRestGroupInfo(&d, NULL)    │                               │
    ├─────────────────────────────────►│ GET <ctx base>/V1/Fence/Info/<fid>
    │                                  ├── Authorization: Basic ──────►│ /V1/Fence/Info/<fid>
    │                                  │◄────────── 200 {json object} ─┤
    │                                  │ mjson → UnfacdGroup           │
    │◄── UnfacdGroup * (filled) ───────┤                               │
```

| argument | meaning |
|---|---|
| `UnfacdGroupDescriptor *descriptor` | the request subject: `descriptor->fid` is the endpoint's path parameter. A zero fid is refused before a request is made. |
| `UnfacdGroup *provided` | caller-owned output. `NULL` makes the library allocate and return a group the caller releases with `free()`; otherwise that group is filled in place and returned. |
| return | the filled group, or `NULL` on a refused request, a transport failure, a missing context or `api_userpwd`, or an unusable reply (`fid` absent, not an unsigned integer, or a member count above `uint32_t`). |

The reply is the object `JsonFormatFenceInfoResponseCallback()` writes
(`ufsrvapi/src/ufsrvrest/fence_info_utils.c`); this is where each attribute lands:

| JSON attribute | `UnfacdGroup` member |
|---|---|
| `fid` | `fid` — required; without it the whole reply is refused |
| `eid` | `eid` |
| `when_created` | `created` |
| `members_count` | `usercount` |
| `cname` | `cname` |
| `dname` | `fname` |
| `avatar`, `fence_permission_membership` | not represented in the type |

Facts that shape the command:

| fact | consequence |
|---|---|
| the endpoint authenticates every request; a cookie alone is not enough | the command sends `ctx->api_userpwd` as HTTP Basic, and returns `NULL` when it is empty |
| the JSON is the whole answer — nothing follows on the websocket | no socket, no future, no coroutine: one call, one result |
| the object is flat and its attribute names are the server's | the parser reads them with uflib's mjson; an absent number leaves its member unchanged (zero on a fresh allocation), an absent `cname`/`dname` clears the string |
| the type has no member for `avatar` or the membership permission | they are dropped, not stashed elsewhere |

```c
UnfacdGroupDescriptor descriptor = {.fid = 1647704372550832159};

UnfacdGroup *group = UnfacdRestGroupInfo(&descriptor, NULL);  /* GET <ctx base>/V1/Fence/Info/<fid> */
if (!group) { /* refused: status, transport, missing context/credential, unusable reply */ }
/* group->fid, eid, created, usercount, cname, fname */
free(group);
```

## Idioms

### Sync

```
 consumer                     ufsrvclilib                        ufsrvapi
    │                             │                                 │
    │ UnfacdGroupJoinSync(&group) │                                 │
    ├────────────────────────────►│ ctx = UfsrvCliContextInstance() │
    │                             │ build FenceCommand(JOIN)        │
    │                             ├── curl_easy_perform ───────────►│ /V1/Fence
    │                             │◄──────────── 200 {"needsSync": false}
    │◄── reply (char *, caller frees) ──────────────────────────────┤
    │                             │                                 │
    │   no group, no fill: the fence does not exist yet             │
```

`…Sync` is one blocking call and returns the REST reply verbatim. It cannot fill the
subject and it does not wait, because the group only exists after the server has
processed the intra command — the confirmation arrives on the websocket afterwards,
and with `…Sync` nothing is listening for it.

| | `…Sync` | `…Rx` |
|---|---|---|
| returns | `char *` REST reply | `UnfacdStateSyncGroup *` container |
| blocks on | the POST only | nothing (returns immediately) |
| fills `UnfacdGroup` | never | on `…RxDeliver` |
| needs a socket | no | yes — the consumer owns it |
| use when | the reply is all you need | the group's attributes are the point |

```c
char *reply = UnfacdGroupJoinSync(&group);   /* POSTs to <ctx base>/V1/Fence */
if (!reply) { /* refused: status, transport or missing context */ }
free(reply);
```

The reply is a receipt (`{"needsSync": false}`), not the group — the response to a
join is the websocket confirmation, never the POST body.

### Rx

```c
UnfacdStateSyncGroup *gs = UnfacdGroupJoinRx(&group, NULL);  /* or a caller-owned one */
/* caller POSTs gs->rx.request_body */
UnfacdGroupJoinRxDeliver(gs, envelope_bytes, envelope_len);  /* on each received message */
/* gs->state_sync_header filled from CommandHeader, group->... filled from FenceRecord */
```

* `…Rx(subject, ctx_or_NULL)` builds the request into `rx.request_body`, links a
  promise/future, and parks a coroutine on that future. It returns immediately.
* `…RxDeliver(ctx, wire, len)` unpacks the envelope, fills the base
  `state_sync_header` from `CommandHeader`, fills the subject from the received
  record, then resolves the promise and resumes the coroutine.
* `rx.resolved` is `1` when delivered, `-1` when the future completed with an error.
* The caller owns the socket, so the caller drives delivery; the library owns the
  parsing once the bytes arrive.

### Shared pieces

* `UnfacdStateSyncRxContext` carries the members both Rx variants need
  (`request_body`, `promise`, `delivery`, `runner`, `resolved`). The derived
  containers keep `UnfacdStateSync state_sync_header` as their base member and add
  only their own subject (`group`, `message`).
* `UnfacdStateSyncFillHeader()` and `UnfacdGroupFillFromFenceRecord()` are the only
  places that translate protobuf into library types. `rest_api_fence.c` is the only
  place that translates a REST JSON reply into one (`UnfacdRestGroupInfo()`).

## Scenario notes

| scenario | what the server sends |
|---|---|
| group created | `FenceCommand` JOIN / `CREATED` |
| joined an existing group by name | nothing (server-side gap) |
| already a member | `FenceCommand` JOIN / `UNCHANGED` or `ACCEPTED` |
| session/fence membership inconsistent | a `/v1/StateSync` request to repair, before the command reply |
| group message | `MessageCommand` confirmation |
| fence info requested (REST GET) | one flat JSON object; no websocket traffic |
