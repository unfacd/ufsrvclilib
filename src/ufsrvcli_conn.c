/**
 * @file ufsrvcli_conn.c
 * @brief Generic TCP connection for ufsrv client tools.
 *
 * Connection is delegated to uflib (ConnectToServer); only raw send/recv of the
 * resulting fd is handled here. No protocol knowledge lives in this module.
 */
#include "ufsrvcli_conn_private.h"

#include <uflib/net/utils_net.h>    /* ConnectToServer */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

static int
sStartTls(UfsrvCliConn *c, const char *host, int verify_peer)
{
    c->ctx = SSL_CTX_new(TLS_client_method());
    if (c->ctx == NULL) return -1;

    if (verify_peer) {
        SSL_CTX_set_verify(c->ctx, SSL_VERIFY_PEER, NULL);
        if (SSL_CTX_set_default_verify_paths(c->ctx) != 1) {
            SSL_CTX_free(c->ctx);
            c->ctx = NULL;
            return -1;
        }
    } else {
        SSL_CTX_set_verify(c->ctx, SSL_VERIFY_NONE, NULL);
    }

    c->ssl = SSL_new(c->ctx);
    if (c->ssl == NULL) {
        SSL_CTX_free(c->ctx);
        c->ctx = NULL;
        return -1;
    }

    if (SSL_set_fd(c->ssl, c->fd) != 1 || SSL_set_tlsext_host_name(c->ssl, host) != 1) goto error_ssl;

    if (SSL_connect(c->ssl) != 1) goto error_ssl;

    if (verify_peer && SSL_get_verify_result(c->ssl) != X509_V_OK) goto error_ssl;

    return 0;

error_ssl:
    SSL_free(c->ssl);
    c->ssl = NULL;
    SSL_CTX_free(c->ctx);
    c->ctx = NULL;
    return -1;
}

static UfsrvCliConn *
sOpenTcp(const char *host, unsigned short port)
{
    UfsrvCliConn *c;
    int fd;

    if (!host)
        return NULL;

    /* uflib: DNS-resolve + connect; returns the connected fd, or <= 0 on error. */
    fd = ConnectToServer(host, (unsigned long)port, NULL);
    if (fd <= 0)
        return NULL;

    c = calloc(1, sizeof(UfsrvCliConn));
    if (!c) {
        close(fd);
        return NULL;
    }
    c->fd = fd;
    return c;
}

UfsrvCliConn *
UfsrvCliConnOpen(const char *host, unsigned short port)
{
    return sOpenTcp(host, port);
}

UfsrvCliConn *
UfsrvCliConnOpenTls(const char *host, unsigned short port, int verify_peer)
{
    UfsrvCliConn *c = sOpenTcp(host, port);

    if (c == NULL)
        return NULL;

    if (sStartTls(c, host, verify_peer) != 0) {
        UfsrvCliConnClose(c);
        return NULL;
    }

    return c;
}

int
UfsrvCliConnIsTls(const UfsrvCliConn *c)
{
    return (c && c->ssl) ? 1 : 0;
}

int
UfsrvCliConnSendAll(UfsrvCliConn *c, const void *buf, size_t len)
{
    size_t off = 0;

    if (!c || c->fd < 0)
        return -1;

    while (off < len) {
        ssize_t n;

        if (c->ssl) {
            n = (ssize_t)SSL_write(c->ssl, (const char *)buf + off, (int)(len - off));
            if (n <= 0) {
                if (SSL_get_error(c->ssl, (int)n) == SSL_ERROR_WANT_WRITE)
                    continue;
                return -1;
            }
        } else {
            n = send(c->fd, (const char *)buf + off, len - off, 0);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                return -1;
            }
            if (n == 0)
                return -1;
        }
        off += (size_t)n;
    }
    return (int)off;
}

int
UfsrvCliConnRecv(UfsrvCliConn *c, void *buf, size_t len)
{
    ssize_t n;

    if (!c || c->fd < 0)
        return -1;

    if (c->ssl) {
        int rc;

        do {
            rc = SSL_read(c->ssl, buf, (int)len);
        } while (rc < 0 && SSL_get_error(c->ssl, rc) == SSL_ERROR_WANT_READ);

        return rc;
    }

    do {
        n = recv(c->fd, buf, len, 0);
    } while (n < 0 && errno == EINTR);

    return (int)n;
}

void
UfsrvCliConnClose(UfsrvCliConn *c)
{
    if (!c)
        return;

    if (c->ssl) {
        SSL_shutdown(c->ssl);
        SSL_free(c->ssl);
    }

    if (c->ctx) SSL_CTX_free(c->ctx);

    if (c->fd >= 0)
        close(c->fd);

    free(c);
}

int
UfsrvCliConnHasPending(const UfsrvCliConn *c)
{
    if (!c || !c->ssl) return 0;

    return SSL_pending(c->ssl) > 0 ? 1 : 0;
}

int
UfsrvCliConnFd(UfsrvCliConn *c)
{
    return (c && c->fd >= 0) ? c->fd : -1;
}
