#ifndef UF_REST_API_FENCE_H
#define UF_REST_API_FENCE_H

#include <uflib/uflib_defs.h>

#include <ufsrvclilib/unfacd_service/unfacd_group_descriptor_type.h>
#include <ufsrvclilib/unfacd_service/unfacd_group_type.h>

PUBLIC_API UnfacdGroup *UnfacdRestGroupInfo(UnfacdGroupDescriptor *descriptor, UnfacdGroup *provided);

#endif
