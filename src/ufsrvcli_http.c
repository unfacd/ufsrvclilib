/**
 * @file ufsrvcli_http.c
 * @brief Minimal HTTP(S) client for ufsrv REST endpoints.
 *
 * Delegates the actual transfer to libcurl (easy interface); only buffers the
 * response body into a growable accumulator and maps libcurl's result onto the
 * small {status, body} result struct.
 */
#include <ufsrvclilib/net/ufsrvcli_http.h>

#include <curl/curl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Growable response-body accumulator fed to libcurl's write callback. */
typedef struct http_body_buf {
    char  *data;
    size_t len;
    size_t cap;
} http_body_buf;

static size_t
sHttpWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    http_body_buf *b = userdata;
    size_t n = size * nmemb;

    if (b->len + n + 1 > b->cap) {
        size_t newcap = b->cap ? b->cap : 4096;
        char  *nd;

        while (newcap < b->len + n + 1)
            newcap *= 2;
        nd = realloc(b->data, newcap);
        if (!nd)
            return 0; /* OOM → abort the transfer */
        b->data = nd;
        b->cap  = newcap;
    }
    memcpy(b->data + b->len, ptr, n);
    b->len += n;
    b->data[b->len] = '\0';
    return n;
}

int
UfsrvCliHttpPerform(const UfsrvCliHttpRequest *req, UfsrvCliHttpResponse *resp)
{
    CURL *curl;
    CURLcode rc;
    struct curl_slist *headers = NULL;
    http_body_buf body = { 0 };
    long http_code = 0;

    if (!req || !req->url || !resp)
        return -1;
    memset(resp, 0, sizeof(*resp));

    curl = curl_easy_init();
    if (!curl)
        return -1;

    curl_easy_setopt(curl, CURLOPT_URL, req->url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sHttpWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ufsrvcli");
    if (req->userpwd)
        curl_easy_setopt(curl, CURLOPT_USERPWD, req->userpwd);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, req->verify_peer ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, req->verify_peer ? 2L : 0L);
    if (req->timeout_sec > 0)
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, req->timeout_sec);

    if (req->cookie) {
        char hdr[512];
        snprintf(hdr, sizeof(hdr), "X-Ufsrv-Cookie: %s", req->cookie);
        headers = curl_slist_append(headers, hdr);
    }
    if (req->content_type) {
        char hdr[256];
        snprintf(hdr, sizeof(hdr), "Content-Type: %s", req->content_type);
        headers = curl_slist_append(headers, hdr);
    }
    if (headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (strcmp(req->method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (req->body) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->body);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(req->body));
        }
    }

    rc = curl_easy_perform(curl);
    if (rc == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        free(body.data);
        return -1;
    }

    resp->status   = http_code;
    resp->body     = body.data ? body.data : calloc(1, 1);
    resp->body_len = body.len;
    if (!resp->body)
        return -1;
    return 0;
}

void
UfsrvCliHttpResponseFree(UfsrvCliHttpResponse *resp)
{
    if (!resp)
        return;
    free(resp->body);
    memset(resp, 0, sizeof(*resp));
}
