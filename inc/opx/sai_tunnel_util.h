/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file sai_tunnel_util.h
 *
 * @brief This file contains the util macros for SAI Tunnel functionality.
 */

#ifndef __SAI_TUNNEL_UTIL_H__
#define __SAI_TUNNEL_UTIL_H__

#include "saitypes.h"
#include "sai_event_log.h"
#include "event_log_types.h"
#include "sai_tunnel.h"

/** Maximum DSCP value used for input validation */
#define SAI_TUNNEL_MAX_DSCP_VAL     (64)
/** Maximum TTL value used for input validation */
#define SAI_TUNNEL_MAX_TTL_VAL      (255)

/** Constant for tunnel object software index */
#define SAI_TUNNEL_OBJ_MAX_ID       (65535)
/** Constant for tunnel map object software index */
#define SAI_TUNNEL_MAP_OBJ_MAX_ID   (65535)
/** Constant for tunnel termination object software index */
#define SAI_TUNNEL_TERM_OBJ_MAX_ID  (65535)

/** Logging utility for SAI Tunnel API */
#define SAI_TUNNEL_LOG(level, msg, ...) \
    do { \
        if (sai_is_log_enabled (SAI_API_TUNNEL, level)) { \
            SAI_LOG_UTIL(ev_log_t_SAI_TUNNEL, level, msg, ##__VA_ARGS__); \
        } \
    } while (0)


/** Per log level based macros for SAI Tunnel API */
#define SAI_TUNNEL_LOG_DEBUG(msg, ...) \
        SAI_TUNNEL_LOG (SAI_LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)

#define SAI_TUNNEL_LOG_CRIT(msg, ...) \
        SAI_TUNNEL_LOG (SAI_LOG_LEVEL_CRITICAL, msg, ##__VA_ARGS__)

#define SAI_TUNNEL_LOG_ERR(msg, ...) \
        SAI_TUNNEL_LOG (SAI_LOG_LEVEL_ERROR, msg, ##__VA_ARGS__)

#define SAI_TUNNEL_LOG_INFO(msg, ...) \
        SAI_TUNNEL_LOG (SAI_LOG_LEVEL_INFO, msg, ##__VA_ARGS__)

#define SAI_TUNNEL_LOG_WARN(msg, ...) \
        SAI_TUNNEL_LOG (SAI_LOG_LEVEL_WARN, msg, ##__VA_ARGS__)

#define SAI_TUNNEL_LOG_NTC(msg, ...) \
        SAI_TUNNEL_LOG (SAI_LOG_LEVEL_NOTICE, msg, ##__VA_ARGS__)


void dn_sai_tunnel_lock (void);
void dn_sai_tunnel_unlock (void);

dn_sai_tunnel_global_t *dn_sai_tunnel_access_global_config (void);

dn_sai_tunnel_t *dn_sai_tunnel_obj_get (sai_object_id_t tunnel_id);

sai_status_t sai_tunnel_object_id_validate (sai_object_id_t tunnel_id);

sai_object_id_t dn_sai_tunnel_underlay_vr_get (sai_object_id_t tunnel_id);

sai_object_id_t dn_sai_tunnel_overlay_vr_get (sai_object_id_t tunnel_id);

dn_sai_tunnel_term_entry_t *dn_sai_tunnel_term_entry_get (
                                                sai_object_id_t tunnel_term_id);

static inline bool dn_sai_is_ip_tunnel (dn_sai_tunnel_t *p_tunnel_obj)
{
    return ((p_tunnel_obj->tunnel_type == SAI_TUNNEL_IPINIP) ||
            (p_tunnel_obj->tunnel_type == SAI_TUNNEL_IPINIP_GRE) ? true : false);
}

static inline rbtree_handle dn_sai_tunnel_tree_handle (void)
{
    return (dn_sai_tunnel_access_global_config()->tunnel_db);
}

static inline rbtree_handle dn_sai_tunnel_term_tree_handle (void)
{
    return (dn_sai_tunnel_access_global_config()->tunnel_term_table_db);
}

static inline rbtree_handle dn_sai_tunnel_map_tree_handle (void)
{
    return (dn_sai_tunnel_access_global_config()->tunnel_mapper_db);
}


#endif /* __SAI_TUNNEL_UTIL_H__ */
