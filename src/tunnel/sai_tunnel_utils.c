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
 * @file sai_tunnel_utils.c
 *
 * @brief This file contains the util functions for SAI Tunnel component.
 */

#include "saitypes.h"
#include "sai_tunnel.h"
#include "sai_tunnel_util.h"
#include "sai_l3_common.h"
#include "sai_oid_utils.h"
#include "std_mutex_lock.h"
#include <string.h>
#include <inttypes.h>

/* Simple Mutex lock for accessing Tunnel resources */
static std_mutex_lock_create_static_init_fast (g_sai_tunnel_lock);

static dn_sai_tunnel_global_t g_tunnel_global_info;

void dn_sai_tunnel_lock (void)
{
    std_mutex_lock (&g_sai_tunnel_lock);
}

void dn_sai_tunnel_unlock (void)
{
    std_mutex_unlock (&g_sai_tunnel_lock);
}

dn_sai_tunnel_global_t *dn_sai_tunnel_access_global_config (void)
{
    return &g_tunnel_global_info;
}

dn_sai_tunnel_t *dn_sai_tunnel_obj_get (sai_object_id_t tunnel_id)
{
    dn_sai_tunnel_t  tunnel_obj;

    memset (&tunnel_obj, 0, sizeof (dn_sai_tunnel_t));
    tunnel_obj.tunnel_id = tunnel_id;

    return ((dn_sai_tunnel_t *) std_rbtree_getexact (
            dn_sai_tunnel_tree_handle(), &tunnel_obj));
}

dn_sai_tunnel_term_entry_t *dn_sai_tunnel_term_entry_get (
                                                sai_object_id_t tunnel_term_id)
{
    dn_sai_tunnel_term_entry_t  tunnel_term;

    memset (&tunnel_term, 0, sizeof (dn_sai_tunnel_term_entry_t));
    tunnel_term.term_entry_id = tunnel_term_id;

    return ((dn_sai_tunnel_term_entry_t *) std_rbtree_getexact (
            dn_sai_tunnel_term_tree_handle(), &tunnel_term));
}

sai_object_id_t dn_sai_tunnel_underlay_vr_get (sai_object_id_t tunnel_id)
{
    dn_sai_tunnel_t *tunnel_obj = NULL;
    sai_object_id_t  vr_id = SAI_NULL_OBJECT_ID;

    tunnel_obj = dn_sai_tunnel_obj_get (tunnel_id);

    if (tunnel_obj != NULL) {
        vr_id = tunnel_obj->underlay_vrf;
    }

    return vr_id;
}

sai_object_id_t dn_sai_tunnel_overlay_vr_get (sai_object_id_t tunnel_id)
{
    dn_sai_tunnel_t *tunnel_obj = NULL;
    sai_object_id_t  vr_id = SAI_NULL_OBJECT_ID;

    tunnel_obj = dn_sai_tunnel_obj_get (tunnel_id);

    if (tunnel_obj != NULL) {
        vr_id = tunnel_obj->overlay_vrf;
    }

    return vr_id;
}

sai_status_t sai_tunnel_object_id_validate (sai_object_id_t tunnel_id)
{
    sai_status_t  status = SAI_STATUS_SUCCESS;

    if (!sai_is_obj_id_tunnel (tunnel_id)) {
        SAI_TUNNEL_LOG_ERR ("0x%"PRIx64" is not a valid Tunnel obj Id.",
                            tunnel_id);

        return SAI_STATUS_INVALID_OBJECT_TYPE;
    }

    dn_sai_tunnel_lock();

    if (dn_sai_tunnel_obj_get (tunnel_id) == NULL) {
        SAI_TUNNEL_LOG_ERR ("0x%"PRIx64" is not a valid Tunnel obj Id.",
                             tunnel_id);

        status = SAI_STATUS_INVALID_OBJECT_ID;
    }

    dn_sai_tunnel_unlock();

    return status;
}
