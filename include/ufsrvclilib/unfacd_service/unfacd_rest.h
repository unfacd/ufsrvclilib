#ifndef UF_UNFACD_REST_H
#define UF_UNFACD_REST_H

#include <uflib/uflib_defs.h>

PUBLIC_API int UnfacdRestPost(const char *url, const char *cookie, const char *json_body,
                              int verify_peer, long timeout_sec, char **reply_out);

PUBLIC_API int UnfacdRestPostBasic(const char *url, const char *userpwd, const char *cookie,
                                   const char *json_body, int verify_peer, long timeout_sec, char **reply_out);

PUBLIC_API int UnfacdRestGetBasic(const char *url, const char *userpwd, const char *cookie,
                                  int verify_peer, long timeout_sec, char **reply_out);

#endif
