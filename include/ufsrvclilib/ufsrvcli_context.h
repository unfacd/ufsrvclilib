/**
 * @file ufsrvcli_context.h
 * @brief The consumer-provided context singleton the library reads its handles from.
 */

#ifndef UFSRVCLI_CONTEXT_H
#define UFSRVCLI_CONTEXT_H

#include <uflib/uflib_defs.h>

#include <ufsrvclilib/ufsrvcli_context_type.h>

PUBLIC_API void UfsrvCliContextProviderLoaded(UfsrvCliContext *context_ptr);

PUBLIC_API UfsrvCliContext *UfsrvCliContextInstance(void);

#endif
