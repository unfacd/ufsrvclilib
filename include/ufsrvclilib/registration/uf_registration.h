/**
 * @file uf_registration.h
 * @brief Account registration client for the ufsrvapi REST server.
 *
 * Drives the registration pipeline the ufsrvapi REST server exposes
 * (src/ufsrvrest/api_endpoint_v1_account.c). Mirrors the Android client's
 * PushServiceSocket.createAccountUfsrv(): fetch a registration nonce
 * (GET /V1/Nonce), then submit a new pending account (POST /V1/Account/New)
 * and capture the pending sign-on cookie the server grants in reply.
 *
 * Transport is UfsrvCliHttp (libcurl); JSON is built/parsed with uflib's
 * mjson. Verification (Account/VerifyNew) and sign-on (Account/SignOn) are
 * follow-ups layered onto the same handle.
 */
#ifndef UF_REGISTRATION_H
#define UF_REGISTRATION_H

#include <uflib/uflib_defs.h>
#include <stddef.h>

#include <ufsrvclilib/registration/uf_registration_type.h>

PUBLIC_API UfRegistrationHandler *UfRegistrationGetHandler(const UfRegistrationOptions *opts);
PUBLIC_API void            UfRegistrationClose(UfRegistrationHandler *r);

/* GET /V1/Nonce → registration nonce into nonce_out. Returns 0/-1. */
PUBLIC_API int UfRegistrationNonce(UfRegistrationHandler *r, char *nonce_out, size_t nonce_cap);

/* POST /V1/Account/New → pending cookie into result. Returns 0/-1. */
PUBLIC_API int UfRegistrationRequestNewAccount(UfRegistrationHandler *r, const UfRegisterParams *params, UfRegisterResult *result);

/* GET /V1/Account/VerifyStatus/<cookie>/<code> → 0 on a 302/200 outcome. */
PUBLIC_API int UfRegistrationVerifyAccountStatus(UfRegistrationHandler *r, const char *cookie, const char *verification_code);

/* POST /V1/Account/VerifyNew (HTTP Basic) → upgraded account into result. */
PUBLIC_API int UfRegistrationSubmitAccountVerification(UfRegistrationHandler * r, const UfRegisterParams *params, const UfVerifyParams *verify,
                                       UfAccountResult *result);

/* Wait out the connection-spacing window before opening another socket. */
PUBLIC_API void UfRegistrationThrottle(UfRegistrationHandler *r);

#endif /* UF_REGISTRATION_H */
