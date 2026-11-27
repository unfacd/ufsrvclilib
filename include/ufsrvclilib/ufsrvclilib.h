/**
 * @file ufsrvclilib.h
 * @brief Umbrella header for the ufsrvclilib library.
 *
 * ufsrvclilib holds the reusable, class-agnostic building blocks shared by all
 * ufsrv CLI class modules (transport, and later config/logging helpers). It is
 * a thin layer over the shared ufsrv foundations (uflib, then ufsrvcorelib,
 * then ufnetcorelib) — it does not reimplement primitives. Class-specific
 * protocol logic lives under src/<class>/ (e.g. src/ufsrvwebsock/).
 */
#ifndef UFSRVCLIB_H
#define UFSRVCLIB_H

#include <uflib/uflib_defs.h>

#include <ufsrvclilib/net/ufsrvcli_conn.h>
#include <ufsrvclilib/net/ufsrvcli_http.h>

#endif /* UFSRVCLIB_H */
