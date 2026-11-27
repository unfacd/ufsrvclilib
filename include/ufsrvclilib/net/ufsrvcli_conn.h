/**
 * @file ufsrvcli_conn.h
 * @brief Generic TCP connection for ufsrv client tools.
 *
 * The reusable transport layer shared by every ufsrv class module: it opens a
 * TCP connection (delegating to uflib's ConnectToServer) and exchanges raw
 * bytes. Each class module (e.g. src/ufsrvwebsock/) layers its own protocol on
 * top of this. The handle is opaque so the transport can later be swapped
 * (TLS, or a test double) without touching callers.
 */
#ifndef UFSRVCLI_CONN_H
#define UFSRVCLI_CONN_H

#include <uflib/uflib_defs.h>
#include <stddef.h>

typedef struct UfsrvCliConn UfsrvCliConn;

/* Connect to host:port over TCP. Returns a handle or NULL on failure. */
PUBLIC_API UfsrvCliConn *UfsrvCliConnOpen(const char *host, unsigned short port);

/* Connect to host:port and establish TLS. verify_peer != 0 checks the chain and
 * the hostname; 0 accepts a self-signed development certificate. */
PUBLIC_API UfsrvCliConn *UfsrvCliConnOpenTls(const char *host, unsigned short port, int verify_peer);

/* 1 when the handle carries a TLS session. */
PUBLIC_API int UfsrvCliConnIsTls(const UfsrvCliConn *c);

/* Send exactly len bytes (retries partial writes). Returns len, or -1 on error. */
PUBLIC_API int UfsrvCliConnSendAll(UfsrvCliConn *c, const void *buf, size_t len);

/* Blocking receive of up to len bytes. Returns bytes read, 0 on peer close, -1 on error. */
PUBLIC_API int UfsrvCliConnRecv(UfsrvCliConn *c, void *buf, size_t len);

PUBLIC_API void UfsrvCliConnClose(UfsrvCliConn *c);

/* 1 when the transport holds bytes already decrypted and waiting (TLS only);
 * 0 otherwise. A caller driving epoll must drain while this is 1: the socket
 * is not readable again for data the TLS record layer has already buffered. */
PUBLIC_API int UfsrvCliConnHasPending(const UfsrvCliConn *c);

/* Return the underlying socket fd (for advanced use, e.g. epoll). -1 if invalid. */
PUBLIC_API int UfsrvCliConnFd(UfsrvCliConn *c);

#endif /* UFSRVCLI_CONN_H */
