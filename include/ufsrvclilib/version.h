/**
 * @file version.h
 * @brief Version macros and accessor declarations for the ufsrvclilib transport.
 *
 * MAJOR/MINOR/PATCH are maintained manually here; INTERNAL is the gate counter.
 * The function bodies are generated at build time by src/version.c.sh ->
 * version.c (a fresh compilation timestamp on every build), mirroring the
 * ufsrvscriptlib and ufsrvcli version pipelines.
 */
#ifndef UFSRVCLILIB_VERSION_H
#define UFSRVCLILIB_VERSION_H

#include <uflib/uflib_defs.h>

#define UFSRVCLILIB_MAJOR    0
#define UFSRVCLILIB_MINOR    1
#define UFSRVCLILIB_PATCH    0
#define UFSRVCLILIB_INTERNAL 0

PUBLIC_API const char *UfsrvCliLibVersion(void)              __attribute__((const));
PUBLIC_API const char *UfsrvCliLibVersionMajor(void)         __attribute__((const));
PUBLIC_API const char *UfsrvCliLibVersionMinor(void)         __attribute__((const));
PUBLIC_API const char *UfsrvCliLibVersionPatch(void)         __attribute__((const));
PUBLIC_API const char *UfsrvCliLibCompiledTimestamp(void)    __attribute__((const));
PUBLIC_API const char *UfsrvCliLibCompiledBy(void)           __attribute__((const));

#endif /* UFSRVCLILIB_VERSION_H */
