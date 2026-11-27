/**
 * @file ufsrvcli_conn_private.h
 * @brief Private definition of the UfsrvCliConn handle.
 */
#ifndef UFSRVCLI_CONN_PRIVATE_H
#define UFSRVCLI_CONN_PRIVATE_H

#include <ufsrvclilib/net/ufsrvcli_conn.h>
#include <openssl/ssl.h>

struct UfsrvCliConn {
    int      fd;  /* connected TCP socket */
    SSL     *ssl; /* TLS session when the connection was opened with TLS */
    SSL_CTX *ctx;
};

#endif /* UFSRVCLI_CONN_PRIVATE_H */
