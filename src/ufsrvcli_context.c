#include <standard_defs.h>
#include <stdbool.h>
#include <threads.h>
#include <ufsrvclilib/ufsrvcli_context.h>

#include "ufsrvcli_context_priv.h"

static UfsrvCliContext *sContextSingleton;

static once_flag init_flag = ONCE_FLAG_INIT;
static bool is_ufsrvclilib_initialised = false;

static void sUfsrvCliLibInit(void) {
  // one-time setup here
  is_ufsrvclilib_initialised = true;
}

static void sUfsrvCliLibEnsureInit(void) {
  call_once(&init_flag, sUfsrvCliLibInit);
}

PUBLIC_API void
UfsrvCliContextProviderLoaded(UfsrvCliContext *context_ptr)
{
  sContextSingleton = context_ptr;
  sUfsrvCliLibEnsureInit();
}

UfsrvCliContext *
UfsrvCliContextInstance(void)
{
  return sContextSingleton;
}
