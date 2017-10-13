/*
 * Copyright (c) 2017 Dell Inc.
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
* @file sai_bridge_utils.c
*
* @brief This file contains utility APIs for SAI BRIDGE module
*************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "saibridge.h"
#include "saistatus.h"
#include "saitypes.h"
#include "std_llist.h"
#include "sai_bridge_api.h"
#include "sai_bridge_common.h"
#include "std_mutex_lock.h"
#include "std_assert.h"
#include "sai_switch_utils.h"
#include "sai_port_utils.h"
#include "sai_oid_utils.h"
#include "sai_gen_utils.h"
#include "sai_map_utl.h"

static std_mutex_lock_create_static_init_fast(bridge_lock);

void sai_bridge_lock(void)
{
    std_mutex_lock(&bridge_lock);
}

void sai_bridge_unlock(void)
{
    std_mutex_unlock(&bridge_lock);
}

void sai_bridge_init_default_bridge_info(dn_sai_bridge_info_t *bridge_info)
{
    if(bridge_info == NULL) {
        SAI_BRIDGE_LOG_TRACE("NULL bridge info passed in bridge info init");
        return;
    }
    bridge_info->max_learned_address = 0;
    bridge_info->learn_disable = false;
    bridge_info->ref_count = 0;
}

void sai_bridge_init_default_bridge_port_info(dn_sai_bridge_port_info_t *bridge_port_info)
{
    if(bridge_port_info == NULL) {
        SAI_BRIDGE_LOG_TRACE("NULL bridge port info passed in bridge port info init");
        return;
    }
    bridge_port_info->fdb_learn_mode = SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW;
    bridge_port_info->max_learned_address = 0;
    bridge_port_info->learn_limit_violation_action = SAI_PACKET_ACTION_DROP;
    bridge_port_info->admin_state = false;
    bridge_port_info->ingress_filtering = false;

}
sai_status_t sai_bridge_map_insert (sai_object_id_t bridge_id, sai_object_id_t bridge_port_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_BRIDGE_TO_BRIDGE_PORT_LIST;
    key.id1  = bridge_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = bridge_port_id;

    return sai_map_insert (&key, &value);
}

sai_status_t sai_bridge_map_remove (sai_object_id_t bridge_id, sai_object_id_t bridge_port_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;
    uint_t       count = 0;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_BRIDGE_TO_BRIDGE_PORT_LIST;
    key.id1  = bridge_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = bridge_port_id;

    sai_map_delete_elements (&key, &value, SAI_MAP_VAL_FILTER_VAL1);

    if (sai_map_get_val_count (&key, &count) == SAI_STATUS_SUCCESS) {
        if (count == 0) {
            sai_map_delete (&key);
        }
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_map_port_list_get (sai_object_id_t  bridge_id,
                                           uint_t          *count,
                                           sai_object_id_t *bridge_port_list)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_status_t   rc;
    uint_t         index;

    if((count == NULL) || (bridge_port_list == NULL)) {
        SAI_BRIDGE_LOG_TRACE("Error count is %p bridge_port_list is %p for bridge id 0x%"PRIx64""
                             " in bridge map port list get",count, bridge_port_list, bridge_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    sai_map_data_t data [*count];
    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data [0], 0, (*count) * sizeof (sai_map_data_t));

    key.type = SAI_MAP_TYPE_BRIDGE_TO_BRIDGE_PORT_LIST;
    key.id1  = bridge_id;

    value.count = *count;
    value.data  = data;

    rc = sai_map_get (&key, &value);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *count = 0;
        return SAI_STATUS_SUCCESS;
    }

    if (rc != SAI_STATUS_SUCCESS) {
        return rc;
    }

    for (index = 0; index < value.count; index++) {
        bridge_port_list [index] = data [index].val1;
    }
    *count = value.count;

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_map_get_port_count (sai_object_id_t  bridge_id,
                                            uint_t        *p_out_count)
{
    sai_map_key_t  key;
    sai_status_t   rc;

    if(p_out_count == NULL) {
        SAI_BRIDGE_LOG_TRACE("Error count is NULL for bridge id 0x%"PRIx64""
                             " in bridge map port count get", bridge_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    memset (&key, 0, sizeof (key));

    key.type = SAI_MAP_TYPE_BRIDGE_TO_BRIDGE_PORT_LIST;
    key.id1  = bridge_id;

    rc = sai_map_get_val_count (&key, p_out_count);

    return rc;
}

sai_status_t sai_bridge_port_vlan_to_bridge_port_map_insert (sai_object_id_t port_id,
                                                             sai_vlan_id_t vlan_id,
                                                             sai_object_id_t bridge_port_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_PORT_VLAN_TO_BRIDGE_PORT_LIST;
    key.id1  = port_id;
    key.id2  = vlan_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = bridge_port_id;

    return sai_map_insert (&key, &value);
}

static sai_status_t sai_bridge_get_bridge_port_id_from_port_vlan (sai_object_id_t port_id,
                                                                  sai_vlan_id_t vlan_id,
                                                                  sai_object_id_t *bridge_port_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;
    sai_status_t   rc = SAI_STATUS_FAILURE;

    if(bridge_port_id == NULL) {
        SAI_BRIDGE_LOG_TRACE("Error bridge_port_id is NULL for port id 0x%"PRIx64" vlan id %d"
                             " in bridge port get from port vlan",port_id, vlan_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_PORT_VLAN_TO_BRIDGE_PORT_LIST;
    key.id1  = port_id;
    key.id2  = vlan_id;

    value.count = 1;
    value.data  = &data;


    rc = sai_map_get (&key, &value);

    if (rc != SAI_STATUS_SUCCESS) {
        *bridge_port_id = SAI_NULL_OBJECT_ID;
        return rc;
    }

    *bridge_port_id = data.val1;

    return SAI_STATUS_SUCCESS;
}

bool sai_bridge_is_bridge_sub_port_duplicate(sai_object_id_t port_id, sai_vlan_id_t vlan_id)
{
    sai_object_id_t bridge_port_id = SAI_NULL_OBJECT_ID;
    sai_status_t    sai_rc = SAI_STATUS_FAILURE;

    sai_rc = sai_bridge_get_bridge_port_id_from_port_vlan(port_id, vlan_id, &bridge_port_id);

    if((sai_rc == SAI_STATUS_SUCCESS) && (bridge_port_id != SAI_NULL_OBJECT_ID)) {
        return true;
    }
    return false;
}

sai_status_t sai_bridge_port_vlan_to_bridge_port_map_remove (sai_object_id_t port_id,
                                                             sai_vlan_id_t vlan_id)
{
    sai_map_key_t  key;

    memset (&key, 0, sizeof (key));

    key.type = SAI_MAP_TYPE_PORT_VLAN_TO_BRIDGE_PORT_LIST;
    key.id1  = port_id;
    key.id2  = vlan_id;

    sai_map_delete (&key);

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_lag_to_bridge_port_map_insert (sai_object_id_t lag_id,
                                                sai_object_id_t bridge_port_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_LAG_TO_BRIDGE_PORT_LIST;
    key.id1  = lag_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = bridge_port_id;

    return sai_map_insert (&key, &value);
}

sai_status_t sai_lag_to_bridge_port_map_remove (sai_object_id_t lag_id,
                                                sai_object_id_t bridge_port_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;
    uint_t         count = 0;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_LAG_TO_BRIDGE_PORT_LIST;
    key.id1  = lag_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = bridge_port_id;

    sai_map_delete_elements (&key, &value, SAI_MAP_VAL_FILTER_VAL1);

    if (sai_map_get_val_count (&key, &count) == SAI_STATUS_SUCCESS) {
        if (count == 0) {
            sai_map_delete (&key);
        }
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_lag_bridge_map_port_list_get (sai_object_id_t  lag_id,
                                               uint_t          *count,
                                               sai_object_id_t *bridge_port_list)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_status_t   rc;
    uint_t       index;

    if((count == NULL) || (bridge_port_list == NULL)) {
        SAI_BRIDGE_LOG_TRACE("Error count is %p bridge_port_list is %p for lag id 0x%"PRIx64""
                             " in lag map bridge port list get",count, bridge_port_list, lag_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    sai_map_data_t data [*count];

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data [0], 0, (*count) * sizeof (sai_map_data_t));

    key.type = SAI_MAP_TYPE_LAG_TO_BRIDGE_PORT_LIST;
    key.id1  = lag_id;

    value.count = *count;
    value.data  = data;

    rc = sai_map_get (&key, &value);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *count = 0;
        return SAI_STATUS_SUCCESS;
    }
    if (rc != SAI_STATUS_SUCCESS) {
        return rc;
    }

    for (index = 0; index < value.count; index++) {
        bridge_port_list [index] = data [index].val1;
    }
    *count = value.count;

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_lag_map_get_bridge_port_count (sai_object_id_t  lag_id,
                                                uint_t          *p_out_count)
{
    sai_map_key_t  key;
    sai_status_t   rc;

    if(p_out_count == NULL) {
        SAI_BRIDGE_LOG_TRACE("Error count is NULL for lag id 0x%"PRIx64""
                             " in lag map bridge port list get", lag_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    memset (&key, 0, sizeof (key));

    key.type = SAI_MAP_TYPE_LAG_TO_BRIDGE_PORT_LIST;
    key.id1  = lag_id;

    rc = sai_map_get_val_count (&key, p_out_count);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *p_out_count = 0;
        return SAI_STATUS_SUCCESS;
    }
    return rc;
}

sai_status_t sai_bridge_port_to_vlan_member_map_insert (sai_object_id_t bridge_port_id,
                                                        sai_object_id_t vlan_member_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;
    sai_status_t   rc = SAI_STATUS_FAILURE;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_VLAN_MEMBER_LIST;
    key.id1  = bridge_port_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = vlan_member_id;

    rc = sai_map_insert (&key, &value);
    if(rc == SAI_STATUS_SUCCESS) {
        sai_bridge_port_increment_ref_count(bridge_port_id);
    }
    return rc;
}

sai_status_t sai_bridge_port_to_vlan_member_map_remove (sai_object_id_t bridge_port_id,
                                                        sai_object_id_t vlan_member_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;
    uint_t         count = 0;
    sai_status_t   rc = SAI_STATUS_FAILURE;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_VLAN_MEMBER_LIST;
    key.id1  = bridge_port_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = vlan_member_id;

    rc = sai_map_delete_elements (&key, &value, SAI_MAP_VAL_FILTER_VAL1);

    if (sai_map_get_val_count (&key, &count) == SAI_STATUS_SUCCESS) {
        if (count == 0) {
            sai_map_delete (&key);
        }
    }

    if(rc == SAI_STATUS_SUCCESS) {
        sai_bridge_port_decrement_ref_count(bridge_port_id);
    }
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_to_vlan_member_list_get (sai_object_id_t  bridge_port_id,
                                                      uint_t          *count,
                                                      sai_object_id_t *vlan_member_list)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_status_t   rc;
    uint_t       index;

    if((count == NULL) || (vlan_member_list == NULL)) {
        SAI_BRIDGE_LOG_TRACE("Error count is %p vlan_member_list is %p for bridge port id "
                             " 0x%"PRIx64" in bridge port vlan member list get",
                             count, vlan_member_list, bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    sai_map_data_t data [*count];

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data [0], 0, (*count) * sizeof (sai_map_data_t));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_VLAN_MEMBER_LIST;
    key.id1  = bridge_port_id;

    value.count = *count;
    value.data  = data;

    rc = sai_map_get (&key, &value);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *count = 0;
        return SAI_STATUS_SUCCESS;
    }
    if (rc != SAI_STATUS_SUCCESS) {
        return rc;
    }

    for (index = 0; index < value.count; index++) {
        vlan_member_list [index] = data [index].val1;
    }
    *count = value.count;

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_to_vlan_member_count_get(sai_object_id_t  bridge_port_id,
                                                      uint_t          *p_out_count)
{
    sai_map_key_t  key;
    sai_status_t   rc;

    if(p_out_count == NULL) {
        SAI_BRIDGE_LOG_TRACE("Error count is NULL for bridge port id 0x%"PRIx64""
                             " in bridge port vlan member count get", bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    memset (&key, 0, sizeof (key));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_VLAN_MEMBER_LIST;
    key.id1  = bridge_port_id;

    rc = sai_map_get_val_count (&key, p_out_count);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *p_out_count = 0;
        return SAI_STATUS_SUCCESS;
    }

    return rc;
}

sai_status_t sai_bridge_port_to_stp_port_map_insert (sai_object_id_t bridge_port_id,
                                                     sai_object_id_t stp_port_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;
    sai_status_t   rc = SAI_STATUS_FAILURE;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_STP_PORT_LIST;
    key.id1  = bridge_port_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = stp_port_id;

    rc = sai_map_insert (&key, &value);

    if(rc == SAI_STATUS_SUCCESS) {
        sai_bridge_port_increment_ref_count(bridge_port_id);
    }
    return rc;
}

sai_status_t sai_bridge_port_to_stp_port_map_remove (sai_object_id_t bridge_port_id,
                                                     sai_object_id_t stp_port_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;
    uint_t         count = 0;
    sai_status_t   rc = SAI_STATUS_FAILURE;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_STP_PORT_LIST;
    key.id1  = bridge_port_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = stp_port_id;

    rc = sai_map_delete_elements (&key, &value, SAI_MAP_VAL_FILTER_VAL1);

    if (sai_map_get_val_count (&key, &count) == SAI_STATUS_SUCCESS) {
        if (count == 0) {
            sai_map_delete (&key);
        }
    }

    if(rc == SAI_STATUS_SUCCESS) {
        sai_bridge_port_decrement_ref_count(bridge_port_id);
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_to_stp_port_list_get (sai_object_id_t  bridge_port_id,
                                                   uint_t          *count,
                                                   sai_object_id_t *stp_port_list)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_status_t   rc;
    uint_t       index;

    if((count == NULL) || (stp_port_list == NULL)) {
        SAI_BRIDGE_LOG_TRACE("Error count is %p stp_port_list is %p for bridge port id "
                             " 0x%"PRIx64" in bridge port stp port list get",
                             count, stp_port_list, bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    sai_map_data_t data [*count];

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data [0], 0, (*count) * sizeof (sai_map_data_t));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_STP_PORT_LIST;
    key.id1  = bridge_port_id;

    value.count = *count;
    value.data  = data;

    rc = sai_map_get (&key, &value);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *count = 0;
        return SAI_STATUS_SUCCESS;
    }
    if (rc != SAI_STATUS_SUCCESS) {
        return rc;
    }

    for (index = 0; index < value.count; index++) {
        stp_port_list [index] = data [index].val1;
    }
    *count = value.count;

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_to_stp_port_count_get(sai_object_id_t  bridge_port_id,
                                                   uint_t          *p_out_count)
{
    sai_map_key_t  key;
    sai_status_t   rc;

    memset (&key, 0, sizeof (key));

    if(p_out_count == NULL) {
        SAI_BRIDGE_LOG_TRACE("Error count is NULL for bridge port id 0x%"PRIx64""
                             " in bridge port stp port count get", bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_STP_PORT_LIST;
    key.id1  = bridge_port_id;

    rc = sai_map_get_val_count (&key, p_out_count);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *p_out_count = 0;
        return SAI_STATUS_SUCCESS;
    }
    return rc;
}

sai_status_t sai_tunnel_to_bridge_port_map_insert (sai_object_id_t tunnel_id,
                                                   sai_object_id_t bridge_port_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_TUNNEL_TO_BRIDGE_PORT_LIST;
    key.id1  = tunnel_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = bridge_port_id;

    return sai_map_insert (&key, &value);
}

sai_status_t sai_tunnel_to_bridge_port_map_remove (sai_object_id_t tunnel_id,
                                                   sai_object_id_t bridge_port_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;
    uint_t         count = 0;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_TUNNEL_TO_BRIDGE_PORT_LIST;
    key.id1  = tunnel_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = bridge_port_id;

    sai_map_delete_elements (&key, &value, SAI_MAP_VAL_FILTER_VAL1);

    if (sai_map_get_val_count (&key, &count) == SAI_STATUS_SUCCESS) {
        if (count == 0) {
            sai_map_delete (&key);
        }
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_tunnel_to_bridge_port_list_get (sai_object_id_t  tunnel_id,
                                                 uint_t          *count,
                                                 sai_object_id_t *bridge_port_list)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_status_t   rc = SAI_STATUS_FAILURE;
    uint_t         index = 0;
    sai_map_data_t data;
    uint_t         map_cnt = 0;

    if((count == NULL) || (bridge_port_list == NULL)) {
        SAI_BRIDGE_LOG_TRACE("Error count is %p bridge_port_list is %p for tunnel id "
                             " 0x%"PRIx64" in tunnel bridge port list get",
                             count, bridge_port_list, tunnel_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (sai_map_data_t));

    key.type = SAI_MAP_TYPE_TUNNEL_TO_BRIDGE_PORT_LIST;
    key.id1  = tunnel_id;

    rc = sai_map_get_val_count (&key, &map_cnt);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *count = 0;
        return SAI_STATUS_SUCCESS;
    }

    if(*count < map_cnt) {
        return SAI_STATUS_BUFFER_OVERFLOW;
    }

    value.data = &data;

    for (index = 0; index < map_cnt; index++) {
        rc = sai_map_get_element_at_index(&key, index, &value);

        if (rc != SAI_STATUS_SUCCESS) {
            return rc;
        }
        bridge_port_list [index] = data.val1;
    }

    *count = map_cnt;

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_tunnel_to_bridge_port_count_get(sai_object_id_t  tunnel_id,
                                                 uint_t          *p_out_count)
{
    sai_map_key_t  key;
    sai_status_t   rc;

    if(p_out_count == NULL) {
        SAI_BRIDGE_LOG_TRACE("Count is NULL for tunnel id 0x%"PRIx64""
                             " in tunnel bridge port count get", tunnel_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    memset (&key, 0, sizeof (key));

    key.type = SAI_MAP_TYPE_TUNNEL_TO_BRIDGE_PORT_LIST;
    key.id1  = tunnel_id;

    rc = sai_map_get_val_count (&key, p_out_count);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *p_out_count = 0;
        return SAI_STATUS_SUCCESS;
    }

    return rc;
}

sai_status_t sai_tunnel_to_bridge_port_get_at_index(sai_object_id_t tunnel_id,
                                                    uint_t index,
                                                    sai_object_id_t *bridge_port)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_status_t   rc = SAI_STATUS_FAILURE;
    sai_map_data_t data;

    if(bridge_port == NULL) {
        SAI_BRIDGE_LOG_TRACE("Bridge port is NULL for tunnel 0x%"PRIx64" in "
                             "tunnel to bridge port get at index", tunnel_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (sai_map_data_t));

    key.type = SAI_MAP_TYPE_TUNNEL_TO_BRIDGE_PORT_LIST;
    key.id1 = tunnel_id;
    value.data = &data;

    rc = sai_map_get_element_at_index(&key, index, &value);
    if (rc != SAI_STATUS_SUCCESS) {
        return rc;
    }

    *bridge_port = data.val1;

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_get_attr_value_from_bridge_info (const dn_sai_bridge_info_t *bridge_info,
                                                         uint_t attr_count,
                                                         sai_attribute_t *attr_list)
{
    uint_t       attr_idx = 0;
    sai_status_t sai_rc = SAI_STATUS_FAILURE;

    if((bridge_info == NULL) || (attr_list == NULL)) {
        SAI_BRIDGE_LOG_TRACE("Bridge info is %p attr_list is %p in get attr value from bridge info",
                             bridge_info, attr_list);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    for(attr_idx = 0; attr_idx < attr_count; attr_idx++) {

        switch(attr_list[attr_idx].id) {
            case SAI_BRIDGE_ATTR_TYPE:
                attr_list[attr_idx].value.s32 = bridge_info->bridge_type;
                break;

            case SAI_BRIDGE_ATTR_MAX_LEARNED_ADDRESSES:
                attr_list[attr_idx].value.u32 = bridge_info->max_learned_address;
                break;

            case SAI_BRIDGE_ATTR_LEARN_DISABLE:
                attr_list[attr_idx].value.booldata = bridge_info->learn_disable;
                break;

            case SAI_BRIDGE_ATTR_PORT_LIST:
                sai_rc = sai_bridge_map_port_list_get(bridge_info->bridge_id,
                                                      &attr_list[attr_idx].value.objlist.count,
                                                      attr_list[attr_idx].value.objlist.list);
                if(sai_rc != SAI_STATUS_SUCCESS) {
                    SAI_BRIDGE_LOG_ERR("Error %d in getting bridge port list for bridge id "
                                       "0x%"PRIx64"",sai_rc, bridge_info->bridge_id);
                    return sai_rc;
                }
                break;

            default:
                return (SAI_STATUS_UNKNOWN_ATTRIBUTE_0 + attr_idx);
        }
    }

    return SAI_STATUS_SUCCESS;

}

sai_status_t sai_bridge_update_attr_value_in_cache(dn_sai_bridge_info_t *bridge_info,
                                                   const sai_attribute_t *attr)
{
    if((bridge_info == NULL) || (attr == NULL)) {
        SAI_BRIDGE_LOG_TRACE("Bridge info is %p attr is %p in update attr value in bridge info",
                             bridge_info, attr);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    switch(attr->id) {

        case SAI_BRIDGE_ATTR_MAX_LEARNED_ADDRESSES:
            bridge_info->max_learned_address = attr->value.u32;
            break;

        case SAI_BRIDGE_ATTR_LEARN_DISABLE:
            bridge_info->learn_disable = attr->value.booldata;
            break;

        default:
            return SAI_STATUS_INVALID_ATTRIBUTE_0;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_increment_ref_count(sai_object_id_t bridge_id)
{
    sai_status_t          sai_rc = SAI_STATUS_FAILURE;
    dn_sai_bridge_info_t *p_bridge_info = NULL;

    sai_rc = sai_bridge_cache_read (bridge_id, &p_bridge_info);
    if ((sai_rc != SAI_STATUS_SUCCESS) || (p_bridge_info == NULL)) {
        SAI_BRIDGE_LOG_ERR("Invalid bridge object id 0x%"PRIx64" in set attribute", bridge_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    p_bridge_info->ref_count++;
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_decrement_ref_count(sai_object_id_t bridge_id)
{
    sai_status_t          sai_rc = SAI_STATUS_FAILURE;
    dn_sai_bridge_info_t *p_bridge_info = NULL;

    sai_rc = sai_bridge_cache_read (bridge_id, &p_bridge_info);
    if ((sai_rc != SAI_STATUS_SUCCESS) || (p_bridge_info == NULL)) {
        SAI_BRIDGE_LOG_ERR("Invalid bridge object id 0x%"PRIx64" in set attribute", bridge_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    p_bridge_info->ref_count--;
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_increment_ref_count(sai_object_id_t bridge_port_id)
{
    sai_status_t               sai_rc = SAI_STATUS_FAILURE;
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;

    sai_rc = sai_bridge_port_cache_read (bridge_port_id, &p_bridge_port_info);
    if ((sai_rc != SAI_STATUS_SUCCESS) || (p_bridge_port_info == NULL)) {
        SAI_BRIDGE_LOG_ERR("Invalid bridge_port object id 0x%"PRIx64" in set attribute",
                            bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    p_bridge_port_info->ref_count++;
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_decrement_ref_count(sai_object_id_t bridge_port_id)
{
    sai_status_t               sai_rc = SAI_STATUS_FAILURE;
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;

    sai_rc = sai_bridge_port_cache_read (bridge_port_id, &p_bridge_port_info);
    if ((sai_rc != SAI_STATUS_SUCCESS) || (p_bridge_port_info == NULL)) {
        SAI_BRIDGE_LOG_ERR("Invalid bridge_port object id 0x%"PRIx64" in set attribute",
                           bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    p_bridge_port_info->ref_count--;
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_get_attr_value_from_bridge_port_info (const dn_sai_bridge_port_info_t
                                                                   *bridge_port_info,
                                                                   uint_t attr_count,
                                                                   sai_attribute_t *attr_list)
{
    uint_t       attr_idx = 0;

    if((bridge_port_info == NULL) || (attr_list == NULL)) {
        SAI_BRIDGE_LOG_TRACE("Bridge port info is %p attr_list is %p in get attr value from "
                             "bridge port info", bridge_port_info, attr_list);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    for(attr_idx = 0; attr_idx < attr_count; attr_idx++) {

        switch(attr_list[attr_idx].id) {

            case SAI_BRIDGE_PORT_ATTR_TYPE:
                attr_list[attr_idx].value.s32 = bridge_port_info->bridge_port_type;
                break;

            case SAI_BRIDGE_PORT_ATTR_MAX_LEARNED_ADDRESSES:
                attr_list[attr_idx].value.u32 = bridge_port_info->max_learned_address;
                break;

            case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE:
                attr_list[attr_idx].value.s32 = bridge_port_info->fdb_learn_mode;
                break;

            case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_LIMIT_VIOLATION_PACKET_ACTION:
                attr_list[attr_idx].value.s32 = bridge_port_info->learn_limit_violation_action;
                break;

            case SAI_BRIDGE_PORT_ATTR_ADMIN_STATE:
                attr_list[attr_idx].value.booldata = bridge_port_info->admin_state;
                break;

            case SAI_BRIDGE_PORT_ATTR_INGRESS_FILTERING:
                attr_list[attr_idx].value.booldata = bridge_port_info->ingress_filtering;
                break;

            case SAI_BRIDGE_PORT_ATTR_BRIDGE_ID:
                attr_list[attr_idx].value.oid = bridge_port_info->bridge_id;
                break;

            case SAI_BRIDGE_PORT_ATTR_PORT_ID:
                attr_list[attr_idx].value.oid = sai_bridge_port_info_get_port_id(bridge_port_info);
                break;

            case SAI_BRIDGE_PORT_ATTR_VLAN_ID:
                attr_list[attr_idx].value.u16 = sai_bridge_port_info_get_vlan_id(bridge_port_info);
                break;

            case SAI_BRIDGE_PORT_ATTR_RIF_ID:
                attr_list[attr_idx].value.oid = sai_bridge_port_info_get_rif_id(bridge_port_info);
                break;

            case SAI_BRIDGE_PORT_ATTR_TUNNEL_ID:
                attr_list[attr_idx].value.oid = sai_bridge_port_info_get_tunnel_id(bridge_port_info);
                break;

            default:
                return (SAI_STATUS_UNKNOWN_ATTRIBUTE_0 + attr_idx);
        }
    }

    return SAI_STATUS_SUCCESS;

}

sai_status_t sai_bridge_port_update_attr_value_in_cache (dn_sai_bridge_port_info_t *bridge_port_info,
                                                         const sai_attribute_t *attr)
{
    if((bridge_port_info == NULL) || (attr == NULL)) {
        SAI_BRIDGE_LOG_TRACE("Bridge port info is %p attr is %p in set attr value in "
                             "bridge port info", bridge_port_info, attr);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    switch(attr->id) {

            case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE:
                bridge_port_info->fdb_learn_mode = attr->value.s32;
                break;

            case SAI_BRIDGE_PORT_ATTR_MAX_LEARNED_ADDRESSES:
                bridge_port_info->max_learned_address = attr->value.u32;
                break;

            case SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_LIMIT_VIOLATION_PACKET_ACTION:
                bridge_port_info->learn_limit_violation_action = attr->value.s32;
                break;

            case SAI_BRIDGE_PORT_ATTR_ADMIN_STATE:
                bridge_port_info->admin_state = attr->value.booldata;
                break;

            case SAI_BRIDGE_PORT_ATTR_INGRESS_FILTERING:
                bridge_port_info->ingress_filtering = attr->value.booldata;
                break;

            case SAI_BRIDGE_PORT_ATTR_BRIDGE_ID:
                bridge_port_info->bridge_id = attr->value.oid;
                break;

            default:
                return SAI_STATUS_INVALID_ATTRIBUTE_0;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_get_port_id(sai_object_id_t bridge_port_id,
                                         sai_object_id_t *sai_port_id)
{
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;
    sai_status_t               sai_rc = SAI_STATUS_FAILURE;

    if(sai_port_id == NULL) {
        SAI_BRIDGE_LOG_TRACE("sai_port_id is NULL for bridge port 0x%"PRIx64" in get port",
                             bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    sai_rc = sai_bridge_port_cache_read(bridge_port_id, &p_bridge_port_info);

    if(sai_rc != SAI_STATUS_SUCCESS) {
        SAI_BRIDGE_LOG_ERR("Error %d in reading bridge port cache for bridge port"
                           " 0x%"PRIx64"", sai_rc, bridge_port_id);
        return sai_rc;
    }
    *sai_port_id = sai_bridge_port_info_get_port_id(p_bridge_port_info);
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_get_vlan_id(sai_object_id_t  bridge_port_id,
                                         uint16_t        *vlan_id)
{
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;
    sai_status_t               sai_rc = SAI_STATUS_FAILURE;

    if(vlan_id == NULL) {
        SAI_BRIDGE_LOG_TRACE("vlan_id is NULL for bridge port 0x%"PRIx64" in get port",
                             bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    sai_rc = sai_bridge_port_cache_read(bridge_port_id, &p_bridge_port_info);

    if(sai_rc != SAI_STATUS_SUCCESS) {
        SAI_BRIDGE_LOG_ERR("Error %d in reading bridge port cache for bridge port"
                           " 0x%"PRIx64"", sai_rc, bridge_port_id);
        return sai_rc;
    }
    *vlan_id = sai_bridge_port_info_get_vlan_id(p_bridge_port_info);
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_get_rif_id(sai_object_id_t  bridge_port_id,
                                        sai_object_id_t *rif_id)
{
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;
    sai_status_t               sai_rc = SAI_STATUS_FAILURE;

    if(rif_id == NULL) {
        SAI_BRIDGE_LOG_TRACE("rif_id is NULL for bridge port 0x%"PRIx64" in get port",
                             bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    sai_rc = sai_bridge_port_cache_read(bridge_port_id, &p_bridge_port_info);

    if(sai_rc != SAI_STATUS_SUCCESS) {
        SAI_BRIDGE_LOG_ERR("Error %d in reading bridge port cache for bridge port"
                           " 0x%"PRIx64"", sai_rc, bridge_port_id);
        return sai_rc;
    }
    *rif_id = sai_bridge_port_info_get_rif_id(p_bridge_port_info);
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_get_tunnel_id(sai_object_id_t  bridge_port_id,
                                           sai_object_id_t *tunnel_id)
{
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;
    sai_status_t               sai_rc = SAI_STATUS_FAILURE;

    if(tunnel_id == NULL) {
        SAI_BRIDGE_LOG_TRACE("tunnel_id is NULL for bridge port 0x%"PRIx64" in get port",
                             bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    sai_rc = sai_bridge_port_cache_read(bridge_port_id, &p_bridge_port_info);

    if(sai_rc != SAI_STATUS_SUCCESS) {
        SAI_BRIDGE_LOG_ERR("Error %d in reading bridge port cache for bridge port"
                           " 0x%"PRIx64"", sai_rc, bridge_port_id);
        return sai_rc;
    }
    *tunnel_id = sai_bridge_port_info_get_tunnel_id(p_bridge_port_info);
    return SAI_STATUS_SUCCESS;
}

void *sai_bridge_port_info_get_bridge_hw_info(dn_sai_bridge_port_info_t  *bridge_port_info)
{
    dn_sai_bridge_info_t *p_bridge_info = NULL;
    sai_status_t         sai_rc = SAI_STATUS_FAILURE;

    if(bridge_port_info == NULL) {
        SAI_BRIDGE_LOG_TRACE("bridge_port_info is NULL in get hardware info");
        return NULL;
    }

    sai_rc = sai_bridge_cache_read(bridge_port_info->bridge_id, &p_bridge_info);

    if(sai_rc != SAI_STATUS_SUCCESS) {
        SAI_BRIDGE_LOG_ERR("Error %d in reading bridge cache for bridge"
                           " 0x%"PRIx64"", sai_rc, bridge_port_info->bridge_id);
       return NULL;
    }
    return p_bridge_info->hw_info;
}

bool sai_is_bridge_port_type_port(sai_object_id_t bridge_port_id)
{
    sai_status_t sai_rc = SAI_STATUS_FAILURE;;
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;

    sai_rc = sai_bridge_port_cache_read(bridge_port_id, &p_bridge_port_info);

    if((sai_rc != SAI_STATUS_SUCCESS) || (p_bridge_port_info == NULL)) {
        SAI_BRIDGE_LOG_ERR("Error in reading cache for bridge port id 0x%"PRIx64"",
                           bridge_port_id);
        return false;
    }
    return (p_bridge_port_info->bridge_port_type == SAI_BRIDGE_PORT_TYPE_PORT);
}

bool sai_bridge_is_bridge_connected_to_tunnel(sai_object_id_t bridge_id,
                                              sai_object_id_t tunnel_id)
{
    bool is_connected = false;
    uint_t bridge_port_idx = 0;
    uint_t bridge_port_count = 0;
    sai_object_id_t bridge_port_id = SAI_NULL_OBJECT_ID;
    sai_status_t sai_rc = SAI_STATUS_FAILURE;
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;

    sai_rc = sai_tunnel_to_bridge_port_count_get(tunnel_id, &bridge_port_count);

    if(sai_rc != SAI_STATUS_SUCCESS)
    {
        SAI_BRIDGE_LOG_ERR("Failed to get bridge port count in tunnel 0x%"
                             PRIx64"object",tunnel_id);
        return false;
    }

    if(bridge_port_count == 0) {
        return false;
    }

    for(bridge_port_idx = 0; bridge_port_idx < bridge_port_count; bridge_port_idx++)
    {
        sai_rc = sai_tunnel_to_bridge_port_get_at_index(tunnel_id, bridge_port_idx,
                                                        &bridge_port_id);
        if(sai_rc != SAI_STATUS_SUCCESS) {
            SAI_BRIDGE_LOG_ERR("Failed to get bridge port at index %u in tunnel "
                               "to bridge port list for tunnel 0x%"PRIx64"",
                               bridge_port_idx, tunnel_id);
            continue;
        }

        sai_rc = sai_bridge_port_cache_read(bridge_port_id,
                                            &p_bridge_port_info);

        if(sai_rc != SAI_STATUS_SUCCESS) {
            SAI_BRIDGE_LOG_ERR("Error %d in reading bridge port cache for bridge port"
                               " 0x%"PRIx64"", sai_rc, bridge_port_id);
            continue;
        }

        if(p_bridge_port_info->bridge_id == bridge_id) {
            is_connected = true;
            break;
        }
    }

    return is_connected;
}

sai_status_t sai_bridge_port_get_type(sai_object_id_t bridge_port_id,
                                      sai_bridge_port_type_t *bridge_port_type)
{
    sai_status_t sai_rc = SAI_STATUS_FAILURE;;
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;

    if(bridge_port_type == NULL) {
        SAI_BRIDGE_LOG_TRACE("Error bridge port type is null for bridge port 0x%"PRIx64""
                             "in bridge port type get", bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    sai_rc = sai_bridge_port_cache_read(bridge_port_id, &p_bridge_port_info);

    if((sai_rc != SAI_STATUS_SUCCESS) || (p_bridge_port_info == NULL)) {
        SAI_BRIDGE_LOG_ERR("Error in reading cache for bridge port id 0x%"PRIx64"",
                           bridge_port_id);
        return sai_rc;
    }
    *bridge_port_type = p_bridge_port_info->bridge_port_type;
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_to_l2mc_member_map_insert (sai_object_id_t bridge_port_id,
                                                        sai_object_id_t l2mc_member_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;
    sai_status_t   rc = SAI_STATUS_FAILURE;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_L2MC_MEMBER_LIST;
    key.id1  = bridge_port_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = l2mc_member_id;

    rc = sai_map_insert (&key, &value);
    if(rc == SAI_STATUS_SUCCESS) {
        sai_bridge_port_increment_ref_count(bridge_port_id);
    }
    return rc;
}

sai_status_t sai_bridge_port_to_l2mc_member_map_remove (sai_object_id_t bridge_port_id,
                                                        sai_object_id_t l2mc_member_id)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_map_data_t data;
    uint_t         count = 0;
    sai_status_t   rc = SAI_STATUS_FAILURE;

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data, 0, sizeof (data));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_L2MC_MEMBER_LIST;
    key.id1  = bridge_port_id;

    value.count = 1;
    value.data  = &data;

    data.val1 = l2mc_member_id;

    rc = sai_map_delete_elements (&key, &value, SAI_MAP_VAL_FILTER_VAL1);

    if (sai_map_get_val_count (&key, &count) == SAI_STATUS_SUCCESS) {
        if (count == 0) {
            sai_map_delete (&key);
        }
    }

    if(rc == SAI_STATUS_SUCCESS) {
        sai_bridge_port_decrement_ref_count(bridge_port_id);
    }
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_to_l2mc_member_list_get (sai_object_id_t  bridge_port_id,
                                                      uint_t          *count,
                                                      sai_object_id_t *l2mc_member_list)
{
    sai_map_key_t  key;
    sai_map_val_t  value;
    sai_status_t   rc;
    uint_t       index;

    if((count == NULL) || (l2mc_member_list == NULL)) {
        SAI_BRIDGE_LOG_TRACE("Error count is %p l2mc_member_list is %p for bridge port id "
                             " 0x%"PRIx64" in bridge port l2mc member list get",
                             count, l2mc_member_list, bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }
    sai_map_data_t data [*count];

    memset (&key, 0, sizeof (key));
    memset (&value, 0, sizeof (value));
    memset (&data [0], 0, (*count) * sizeof (sai_map_data_t));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_L2MC_MEMBER_LIST;
    key.id1  = bridge_port_id;

    value.count = *count;
    value.data  = data;

    rc = sai_map_get (&key, &value);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *count = 0;
        return SAI_STATUS_SUCCESS;
    }
    if (rc != SAI_STATUS_SUCCESS) {
        return rc;
    }

    for (index = 0; index < value.count; index++) {
        l2mc_member_list [index] = data [index].val1;
    }
    *count = value.count;

    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_bridge_port_to_l2mc_member_count_get(sai_object_id_t  bridge_port_id,
                                                      uint_t          *p_out_count)
{
    sai_map_key_t  key;
    sai_status_t   rc;

    if(p_out_count == NULL) {
        SAI_BRIDGE_LOG_TRACE("Error count is NULL for bridge port id 0x%"PRIx64""
                             " in bridge port l2mc member count get", bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    memset (&key, 0, sizeof (key));

    key.type = SAI_MAP_TYPE_BRIDGE_PORT_TO_L2MC_MEMBER_LIST;
    key.id1  = bridge_port_id;

    rc = sai_map_get_val_count (&key, p_out_count);

    if(rc == SAI_STATUS_ITEM_NOT_FOUND) {
        *p_out_count = 0;
        return SAI_STATUS_SUCCESS;
    }

    return rc;
}

bool sai_is_bridge_port_type_sub_port(sai_object_id_t bridge_port_id)
{
    sai_status_t sai_rc = SAI_STATUS_FAILURE;;
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;

    sai_rc = sai_bridge_port_cache_read(bridge_port_id, &p_bridge_port_info);

    if((sai_rc != SAI_STATUS_SUCCESS) || (p_bridge_port_info == NULL)) {
        SAI_BRIDGE_LOG_ERR("Error in reading cache for bridge port id 0x%"PRIx64"",
                           bridge_port_id);
        return false;
    }
    return (p_bridge_port_info->bridge_port_type == SAI_BRIDGE_PORT_TYPE_SUB_PORT);
}

bool sai_is_bridge_port_obj_lag(sai_object_id_t bridge_port_id)
{
    sai_status_t sai_rc = SAI_STATUS_FAILURE;;
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;
    sai_object_id_t port_id = SAI_NULL_OBJECT_ID;
    sai_rc = sai_bridge_port_cache_read(bridge_port_id, &p_bridge_port_info);

    if((sai_rc != SAI_STATUS_SUCCESS) || (p_bridge_port_info == NULL)) {
        SAI_BRIDGE_LOG_ERR("Error in reading cache for bridge port id 0x%"PRIx64"",
                           bridge_port_id);
        return false;
    }
    if (p_bridge_port_info->bridge_port_type == SAI_BRIDGE_PORT_TYPE_PORT) {
        port_id = sai_bridge_port_info_get_port_id(p_bridge_port_info);
        return sai_is_obj_id_lag(port_id);
    }
    return false;
}

sai_status_t sai_bridge_port_get_admin_state(sai_object_id_t bridge_port_id,
                                             bool *admin_state)
{
    sai_status_t               sai_rc = SAI_STATUS_FAILURE;;
    dn_sai_bridge_port_info_t *p_bridge_port_info = NULL;

    if(admin_state == NULL) {
        SAI_BRIDGE_LOG_TRACE("Error admin state is null for bridge port 0x%"PRIx64""
                             "in bridge port type get", bridge_port_id);
        return SAI_STATUS_INVALID_PARAMETER;
    }

    sai_rc = sai_bridge_port_cache_read(bridge_port_id, &p_bridge_port_info);

    if((sai_rc != SAI_STATUS_SUCCESS) || (p_bridge_port_info == NULL)) {
        SAI_BRIDGE_LOG_ERR("Error in reading cache for bridge port id 0x%"PRIx64"",
                           bridge_port_id);
        return sai_rc;
    }
    *admin_state = p_bridge_port_info->admin_state;
    return SAI_STATUS_SUCCESS;
}
