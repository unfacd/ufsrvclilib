/**
 * @file uf_registration_handler_private.h
 * @brief Definition of the UfRegistrationHandler handle.
 */
#ifndef UF_REGISTRATION_HANDLER_PRIVATE_H
#define UF_REGISTRATION_HANDLER_PRIVATE_H

#include <ufsrvclilib/registration/uf_registration_type.h>

struct UfRegistrationHandler {
    char  base_url[512]; /* "scheme://host:port" assembled at open time */
    int   verify_peer;
    long  timeout_sec;
    long long last_request_ms; /* 0 = no request issued yet */
};

#endif /* UF_REGISTRATION_HANDLER_PRIVATE_H */
