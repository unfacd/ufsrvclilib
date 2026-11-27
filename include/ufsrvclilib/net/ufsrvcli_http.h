/**
 * @file ufsrvcli_http.h
 * @brief Minimal HTTP(S) client for ufsrv REST endpoints.
 *
 * ufsrv's REST surface (ufsrvapi) is served over HTTPS; the registration and
 * sign-on flows the CLI drives are plain HTTP requests with JSON bodies. This
 * module is the reusable transport for those flows — a thin wrapper over
 * libcurl (it does not reimplement TLS/HTTP). Class-specific REST logic lives
 * in src/ufsrvapi/ and layers its JSON on top of this.
 */
#ifndef UFSRVCLI_HTTP_H
#define UFSRVCLI_HTTP_H

#include <uflib/uflib_defs.h>
#include <stddef.h>

/* Parameters for a single HTTP request. */
typedef struct UfsrvCliHttpRequest {
    const char *method;       /* "GET" or "POST" */
    const char *url;          /* full URL, e.g. "https://127.0.0.1:20081/V1/Nonce" */
    const char *body;         /* request body (POST); NULL for GET */
    const char *content_type; /* e.g. "application/json"; may be NULL */
    const char *userpwd;      /* "user:password" for HTTP Basic; may be NULL */
    const char *cookie;       /* sent verbatim as the Cookie header; may be NULL */
    int         verify_peer;  /* 1 = verify TLS cert, 0 = self-signed (dev) */
    long        timeout_sec;  /* 0 = libcurl default */
} UfsrvCliHttpRequest;

/* Result of an HTTP request. */
typedef struct UfsrvCliHttpResponse {
    long   status;    /* HTTP status code (0 if none received) */
    char  *body;      /* NUL-terminated response body; caller frees */
    size_t body_len;  /* length of body excluding NUL */
} UfsrvCliHttpResponse;

/* Perform an HTTP(S) request. Returns 0 on success and allocates resp->body
 * (free with UfsrvCliHttpResponseFree); returns -1 on transport error. */
PUBLIC_API int UfsrvCliHttpPerform(const UfsrvCliHttpRequest *req,
                                   UfsrvCliHttpResponse *resp);

/* Free the response body and zero the struct. */
PUBLIC_API void UfsrvCliHttpResponseFree(UfsrvCliHttpResponse *resp);

#endif /* UFSRVCLI_HTTP_H */
