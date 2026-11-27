/**
 * @file uf_registration.c
 * @brief Account registration client for the ufsrvapi REST server.
 *
 * Mirrors PushServiceSocket.createAccountUfsrv(): GET /V1/Nonce for the
 * registration nonce, then POST /V1/Account/New with the unverified-account
 * JSON to obtain the pending sign-on cookie. JSON bodies are built and parsed
 * with uflib's mjson (%Q quotes + JSON-escapes); the HTTP transfer is handed
 * to UfsrvCliHttp (libcurl).
 */
#include <ufsrvclilib/registration/uf_registration.h>
#include "registration/uf_registration_handler_private.h"

#include <ufsrvclilib/net/ufsrvcli_http.h>
#include <uflib/mjson/mjson.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define UFSRVAPI_REGISTRATION_PATH_NONCE "/V1/Nonce"
#define UFSRVAPI_REGISTRATION_PATH_NEW   "/V1/Account/New"
#define UFSRVAPI_REGISTRATION_PATH_VERIFY_NEW    "/V1/Account/VerifyNew"
#define UFSRVAPI_REGISTRATION_PATH_VERIFY_STATUS "/V1/Account/VerifyStatus"
#define UFSRVAPI_REGISTRATION_MIN_REQUEST_INTERVAL_MS 1100LL

UfRegistrationHandler *
UfRegistrationGetHandler(const UfRegistrationOptions *opts)
{
  UfRegistrationHandler *r;

  if (!opts || !opts->host) return NULL;

  r = calloc(1, sizeof(UfRegistrationHandler));
  if (!r) return NULL;

  snprintf(r->base_url, sizeof(r->base_url), "%s://%s:%u", opts->tls ? "https" : "http", opts->host,
           (unsigned)opts->port);
  r->verify_peer = opts->verify_peer;
  r->timeout_sec = opts->timeout_sec;
  return r;
}

void
UfRegistrationClose(UfRegistrationHandler *r)
{
  free(r);
}

static long long
sMonotonicMilliseconds(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000LL + (long long)ts.tv_nsec / 1000000LL;
}

static void
sEnforceRequestInterval(UfRegistrationHandler *r)
{
  long long now_ms = sMonotonicMilliseconds();

  if (r->last_request_ms != 0) {
    long long wait_ms = UFSRVAPI_REGISTRATION_MIN_REQUEST_INTERVAL_MS - (now_ms - r->last_request_ms);
    if (wait_ms > 0) {
      struct timespec req = {(time_t)(wait_ms / 1000LL), (long)((wait_ms % 1000LL) * 1000000LL)};
      while (nanosleep(&req, &req) == -1 && errno == EINTR) {
      }
    }
  }

  r->last_request_ms = sMonotonicMilliseconds();
}

/* One REST call against r's base URL; json_body NULL selects GET. */
static int
sRegistrationRequestThis(UfRegistrationHandler *r, const char *path, const char *json_body, const char *userpwd, UfsrvCliHttpResponse *resp)
{
  UfsrvCliHttpRequest req;
  char                url[640];

  sEnforceRequestInterval(r);

  snprintf(url, sizeof(url), "%s%s", r->base_url, path);

  memset(&req, 0, sizeof(req));
  req.method       = json_body ? "POST" : "GET";
  req.url          = url;
  req.body         = json_body;
  req.content_type = json_body ? "application/json" : NULL;
  req.userpwd      = userpwd;
  req.verify_peer  = r->verify_peer;
  req.timeout_sec  = r->timeout_sec;

  return UfsrvCliHttpPerform(&req, resp);
}

int
UfRegistrationNonce(UfRegistrationHandler *r, char *nonce_out, size_t nonce_cap)
{
  UfsrvCliHttpResponse resp;
  int                  rc;

  if (!r || !nonce_out || nonce_cap == 0) return -1;
  nonce_out[0] = '\0';

  if (sRegistrationRequestThis(r, UFSRVAPI_REGISTRATION_PATH_NONCE, NULL, NULL, &resp) != 0) return -1;
  if (resp.status != 200) {
    UfsrvCliHttpResponseFree(&resp);
    return -1;
  }

  rc = mjson_get_string(resp.body, (int)resp.body_len, "$.nonce", nonce_out, (int)nonce_cap);
  UfsrvCliHttpResponseFree(&resp);
  return (rc >= 0) ? 0 : -1;
}

int
UfRegistrationRequestNewAccount(UfRegistrationHandler *r, const UfRegisterParams *params, UfRegisterResult *result)
{
  UfsrvCliHttpResponse resp;
  const char *         sms_retriever;
  char *               json_body;
  int                  rc;

  if (!r || !params || !params->username || !params->password || !params->nonce || !result) return -1;
  memset(result, 0, sizeof(*result));

  sms_retriever = params->android_sms_retriever ? params->android_sms_retriever : "android";

  /* %Q emits a quoted, JSON-escaped string — no hand-rolled escaping. */
  json_body = mjson_aprintf(
    "{\"username\":%Q,\"e164number\":%Q,\"password\":%Q,\"nonce\":%Q," "\"androidSmsRetriever\":%Q}", params->username,
    params->e164number ? params->e164number : "", params->password, params->nonce, sms_retriever);
  if (!json_body) return -1;

  if (sRegistrationRequestThis(r, UFSRVAPI_REGISTRATION_PATH_NEW, json_body, NULL, &resp) != 0) {
    free(json_body);
    return -1;
  }
  free(json_body);

  if (resp.status != 200) {
    UfsrvCliHttpResponseFree(&resp);
    return -1;
  }

  rc = mjson_get_string(resp.body, (int)resp.body_len, "$.cookie", result->cookie, (int)sizeof(result->cookie));
  UfsrvCliHttpResponseFree(&resp);
  return (rc >= 0) ? 0 : -1;
}

int
UfRegistrationVerifyAccountStatus(UfRegistrationHandler *r, const char *cookie, const char *verification_code)
{
  UfsrvCliHttpResponse resp;
  char                 path[640];
  int                  verified;

  if (!r || !cookie || !verification_code) return -1;

  snprintf(path, sizeof(path), "%s/%s/%s", UFSRVAPI_REGISTRATION_PATH_VERIFY_STATUS, cookie, verification_code);
  if (sRegistrationRequestThis(r, path, NULL, NULL, &resp) != 0) return -1;

  verified = (resp.status == 200 || resp.status == 302);
  UfsrvCliHttpResponseFree(&resp);
  return verified ? 0 : -1;
}

int
UfRegistrationSubmitAccountVerification(UfRegistrationHandler * r, const UfRegisterParams *params, const UfVerifyParams *verify, UfAccountResult *result)
{
  UfsrvCliHttpResponse resp;
  char                 userpwd[600];
  char *               json_body;
  char *               end = NULL;
  long                 code;
  double               userid = 0;
  int                  rc;

  if (!r || !params || !params->username || !params->password || !verify || !verify->cookie || !verify->
    verification_code || !result) return -1;
  memset(result, 0, sizeof(*result));

  errno = 0;
  code  = strtol(verify->verification_code, &end, 10);
  if (errno != 0 || end == verify->verification_code || *end != '\0') return -1;

  snprintf(userpwd, sizeof(userpwd), "%s:%s", params->username, params->password);
  json_body = mjson_aprintf("{\"cookie\":%Q,\"verificationCode\":%ld}", verify->cookie, code);
  if (!json_body) return -1;

  if (sRegistrationRequestThis(r, UFSRVAPI_REGISTRATION_PATH_VERIFY_NEW, json_body, userpwd, &resp) != 0) {
    free(json_body);
    return -1;
  }
  free(json_body);

  if (resp.status != 200) {
    UfsrvCliHttpResponseFree(&resp);
    return -1;
  }

  rc = mjson_get_string(resp.body, (int)resp.body_len, "$.cookie", result->cookie, (int)sizeof(result->cookie));
  if (rc >= 0) mjson_get_string(resp.body, (int)resp.body_len, "$.username", result->username,
                                (int)sizeof(result->username));
  if (rc >= 0) mjson_get_string(resp.body, (int)resp.body_len, "$.ufsrvuid", result->ufsrvuid,
                                (int)sizeof(result->ufsrvuid));
  if (rc >= 0) mjson_get_string(resp.body, (int)resp.body_len, "$.uuid", result->uuid, (int)sizeof(result->uuid));
  if (rc >= 0) mjson_get_string(resp.body, (int)resp.body_len, "$.e164number", result->e164number,
                                (int)sizeof(result->e164number));
  if (rc >= 0 && mjson_get_number(resp.body, (int)resp.body_len, "$.userid", &userid) >= 0) result->userid = (long)
    userid;

  UfsrvCliHttpResponseFree(&resp);
  return (rc >= 0) ? 0 : -1;
}

void
UfRegistrationThrottle(UfRegistrationHandler *r)
{
  if (r) sEnforceRequestInterval(r);
}
