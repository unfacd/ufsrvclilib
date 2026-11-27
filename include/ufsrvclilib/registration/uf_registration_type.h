/**
 * @file uf_registration_type.h
 * @brief Types for the ufsrv account registration client.
 *
 * Type-only header: the handle, its options, the new-account and verification
 * inputs, the two reply records, and the pending-account database options.
 * The API that operates on them is ufsrvapi_registration.h.
 */
#ifndef UF_REGISTRATION_TYPE_H
#define UF_REGISTRATION_TYPE_H

typedef struct UfRegistrationHandler UfRegistrationHandler;

/* Target ufsrvapi backend and TLS policy. */
typedef struct UfRegistrationOptions {
    const char    *host;        /* e.g. "127.0.0.1" */
    unsigned short port;        /* e.g. 20081 */
    int            tls;         /* 1 = https, 0 = http */
    int            verify_peer; /* 1 = verify cert, 0 = self-signed (dev) */
    long           timeout_sec; /* 0 = default */
} UfRegistrationOptions;

/* New-account inputs (mirrors JsonEntityUnverifiedAccount). */
typedef struct UfRegisterParams {
    const char *username;              /* account e-mail (validated server-side) */
    const char *e164number;            /* E.164 phone number; may be NULL */
    const char *password;              /* account password */
    const char *nonce;                 /* registration nonce (from /V1/Nonce) */
    const char *android_sms_retriever; /* "android" | "android-ng"; may be NULL */
} UfRegisterParams;

/* Outcome of a successful POST /V1/Account/New. */
typedef struct UfRegisterResult {
    char cookie[256]; /* pending registration (sign-on) cookie granted */
} UfRegisterResult;

/* Pending-account verification inputs. */
typedef struct UfVerifyParams {
    const char *cookie;            /* pending cookie from /V1/Account/New */
    const char *verification_code; /* code stored with the pending account */
} UfVerifyParams;

/* Outcome of a successful POST /V1/Account/VerifyNew. */
typedef struct UfAccountResult {
    char cookie[256];
    char username[256];
    char ufsrvuid[64];
    char uuid[64];
    char e164number[64];
    long userid;
} UfAccountResult;

/* MariaDB connection used to read the pending account's verification code. */
typedef struct UfMysqlOptions {
    const char    *host;
    unsigned short port;
    const char    *user;
    const char    *password;
    const char    *database;
    int            ssl_verify;
} UfMysqlOptions;

#endif /* UF_REGISTRATION_TYPE_H */
