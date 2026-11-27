#ifndef UF_UNFACD_GROUP_TYPE_H
#define UF_UNFACD_GROUP_TYPE_H

#include <stdint.h>

#include <uflib/collection_descriptor_type.h>

#include <ufsrvclilib/unfacd_service/unfacd_user_type.h>

#define UNFACD_GROUP_FNAME_MAX       128
#define UNFACD_GROUP_CNAME_MAX       128
#define UNFACD_GROUP_DESCRIPTION_MAX 512

typedef enum UnfacdGroupType
{
  UNFACD_GROUP_TYPE_UNKNOWN = 0,
  UNFACD_GROUP_TYPE_GEO     = 1,
  UNFACD_GROUP_TYPE_USER    = 2
} UnfacdGroupType;

typedef enum UnfacdGroupPrivacy
{
  UNFACD_GROUP_PRIVACY_PUBLIC  = 0,
  UNFACD_GROUP_PRIVACY_PRIVATE = 1
} UnfacdGroupPrivacy;

typedef enum UnfacdGroupJoinMode
{
  UNFACD_GROUP_JOIN_OPEN            = 0,
  UNFACD_GROUP_JOIN_INVITE          = 1,
  UNFACD_GROUP_JOIN_OPEN_WITH_KEY   = 2,
  UNFACD_GROUP_JOIN_INVITE_WITH_KEY = 3
} UnfacdGroupJoinMode;

typedef enum UnfacdGroupDeliveryMode
{
  UNFACD_GROUP_DELIVERY_MANY             = 0,
  UNFACD_GROUP_DELIVERY_BROADCAST        = 1,
  UNFACD_GROUP_DELIVERY_BROADCAST_ONEWAY = 2
} UnfacdGroupDeliveryMode;

typedef enum UnfacdGroupLongevity
{
  UNFACD_GROUP_LONGEVITY_UNTIMED = 0,
  UNFACD_GROUP_LONGEVITY_TIMED   = 1
} UnfacdGroupLongevity;

typedef struct UnfacdGroup
{
  uint64_t                fid;
  char                    fname[UNFACD_GROUP_FNAME_MAX];
  char                    cname[UNFACD_GROUP_CNAME_MAX];
  char                    description[UNFACD_GROUP_DESCRIPTION_MAX];
  UnfacdGroupType         type;
  UnfacdGroupPrivacy      privacy;
  UnfacdGroupJoinMode     join_mode;
  UnfacdGroupDeliveryMode delivery;
  uint32_t                maxmembers;
  uint32_t                usercount;
  uint64_t                eid;
  uint64_t                created;
  uint64_t                ttl;
  UnfacdGroupLongevity    longevity;
  CollectionDescriptor    members;
  CollectionDescriptor    invited;
  CollectionDescriptor    linkjoin;
  CollectionDescriptor    banned;
  unsigned                owned_collections;
} UnfacdGroup;

#endif
