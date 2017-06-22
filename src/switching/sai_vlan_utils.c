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
 * @file sai_vlan_utils.c
 *
 * @brief This file contains utility APIs for SAI VLAN module
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "saivlan.h"
#include "saistatus.h"
#include "saitypes.h"
#include "std_llist.h"
#include "sai_vlan_api.h"
#include "sai_vlan_common.h"
#include "std_mutex_lock.h"
#include "std_assert.h"
#include "sai_switch_utils.h"
#include "sai_port_utils.h"
#include "sai_oid_utils.h"
#include "sai_gen_utils.h"

static sai_vlan_global_cache_node_t *global_vlan_list[SAI_MAX_VLAN_TAG_ID+1];
static std_mutex_lock_create_static_init_fast(vlan_lock);
static sai_vlan_id_t sai_internal_vlan_id = VLAN_UNDEF;
static std_dll_head global_port_vlan_count_list;
static rbtree_handle global_vlan_member_tree;

rbtree_handle sai_vlan_global_member_tree_get(void)
{
    return global_vlan_member_tree;
}

void sai_vlan_lock(void)
{
    std_mutex_lock(&vlan_lock);
}

void sai_vlan_unlock(void)
{
    std_mutex_unlock(&vlan_lock);
}

sai_vlan_global_cache_node_t* sai_vlan_portlist_cache_read(sai_vlan_id_t vlan_id)
{
    return global_vlan_list[vlan_id];
}

void sai_init_internal_vlan_id(sai_vlan_id_t vlan_id)
{
    sai_internal_vlan_id = vlan_id;
}

bool sai_is_internal_vlan_id_initialized (void)
{
    return (sai_internal_vlan_id != VLAN_UNDEF);
}

bool sai_is_internal_vlan_id(sai_vlan_id_t vlan_id)
{
    return (vlan_id == sai_internal_vlan_id) ? true : false;
}

sai_vlan_id_t sai_internal_vlan_id_get(void)
{
    return sai_internal_vlan_id;
}

sai_vlan_id_t sai_vlan_obj_id_to_vlan_id(sai_object_id_t vlan_obj_id)
{
    return ((sai_vlan_id_t)sai_uoid_npu_obj_id_get(vlan_obj_id));
}

sai_object_id_t sai_vlan_id_to_vlan_obj_id(sai_vlan_id_t vlan_id)
{
    return (sai_uoid_create(SAI_OBJECT_TYPE_VLAN, vlan_id));
}

sai_status_t sai_vlan_cache_init(void)
{
    sai_vlan_id_t vlan_id = 0;

    SAI_VLAN_LOG_TRACE("Performing VLAN Module Init");
    for(vlan_id = 0; vlan_id <= SAI_MAX_VLAN_TAG_ID; vlan_id++)
    {
        global_vlan_list[vlan_id] = NULL;
    }
    /*Allocating for maximum possible port range
      so that it can work for fanout too
     */
    std_dll_init_sort(&global_port_vlan_count_list,sai_port_node_compare,
                 SAI_PORTV_VLAN_COUNTER_OFFSET, SAI_PORTV_VLAN_COUNTER_SIZE);

    global_vlan_member_tree = std_rbtree_create_simple("SAI VLAN member tree",
            STD_STR_OFFSET_OF(sai_vlan_member_node_t, vlan_member_id),
            STD_STR_SIZE_OF(sai_vlan_member_node_t, vlan_member_id));

    return SAI_STATUS_SUCCESS;
}

static sai_status_t sai_add_port_vlan_counter_node(sai_object_id_t port_id)
{
    sai_port_vlan_counter_t *port_vlan_counter = NULL;

    port_vlan_counter = (sai_port_vlan_counter_t *)
                         calloc(1, sizeof(sai_port_vlan_counter_t));
    if(port_vlan_counter == NULL) {
        SAI_VLAN_LOG_CRIT("Unable to add port 0x%"PRIx64" memory %d unavailable",
                         port_id, sizeof(sai_port_vlan_counter_t));
        return SAI_STATUS_NO_MEMORY;

    }
    port_vlan_counter->port_id = port_id;
    std_dll_insert(&global_port_vlan_count_list,&(port_vlan_counter->node));
    return SAI_STATUS_SUCCESS;
}

static sai_port_vlan_counter_t* sai_find_port_vlan_counter(sai_object_id_t port_id)
{
    sai_port_vlan_counter_t *port_vlan_counter = NULL;
    std_dll *node = NULL;

    for(node = std_dll_getfirst(&global_port_vlan_count_list);
        node != NULL;
        node = std_dll_getnext(&global_port_vlan_count_list,node)) {
        port_vlan_counter = (sai_port_vlan_counter_t *)node;
        if(port_vlan_counter->port_id == port_id) {
            return port_vlan_counter;
        }else if (port_vlan_counter->port_id > port_id){
            break;
        }
    }
    return NULL;
}

static sai_status_t sai_remove_port_vlan_counter(sai_object_id_t port_id)
{
    sai_port_vlan_counter_t *port_vlan_counter = NULL;

    port_vlan_counter = sai_find_port_vlan_counter(port_id);
    if(port_vlan_counter != NULL) {
        std_dll_remove(&global_port_vlan_count_list,&(port_vlan_counter->node));
        free(port_vlan_counter);
        return SAI_STATUS_SUCCESS;
    }
    return SAI_STATUS_ITEM_NOT_FOUND;
}

bool sai_is_port_vlan_configured(sai_object_id_t port_id)
{
    sai_port_vlan_counter_t *port_vlan_counter = NULL;

    port_vlan_counter = sai_find_port_vlan_counter(port_id);
    if(port_vlan_counter == NULL) {
        return false;
    }
    return true;
}

static sai_status_t sai_increment_port_vlan_counter(sai_object_id_t port_id)
{
    sai_port_vlan_counter_t *port_vlan_counter = NULL;
    sai_status_t ret_val = SAI_STATUS_FAILURE;

    port_vlan_counter = sai_find_port_vlan_counter(port_id);
    if(port_vlan_counter == NULL) {
         ret_val = sai_add_port_vlan_counter_node(port_id);
         if(ret_val != SAI_STATUS_SUCCESS) {
             SAI_VLAN_LOG_WARN("Unable to create port vlan counter for port 0x%"PRIx64"",
                               port_id);
             return ret_val;
         }
         port_vlan_counter = sai_find_port_vlan_counter(port_id);
    }
    port_vlan_counter->vlan_count++;
    return SAI_STATUS_SUCCESS;
}

static sai_status_t sai_decrement_port_vlan_counter(sai_object_id_t port_id)
{
    sai_port_vlan_counter_t *port_vlan_counter = NULL;

    port_vlan_counter = sai_find_port_vlan_counter(port_id);
    if(port_vlan_counter == NULL) {
         SAI_VLAN_LOG_WARN("Unable to find port vlan counter for port 0x%"PRIx64"",
                            port_id);
         return SAI_STATUS_ITEM_NOT_FOUND;
    }
    port_vlan_counter->vlan_count--;
    if(port_vlan_counter->vlan_count == 0) {
        sai_remove_port_vlan_counter(port_id);
    }
    return SAI_STATUS_SUCCESS;
}

bool sai_is_vlan_created(sai_vlan_id_t vlan_id)
{
    if((vlan_id > SAI_MAX_VLAN_TAG_ID) ||
        (vlan_id < SAI_MIN_VLAN_TAG_ID)) {
        return false;
    }
    return ((global_vlan_list[vlan_id] != NULL) ? true:false);
}

sai_status_t sai_insert_vlan_in_list(sai_vlan_id_t vlan_id)
{
    sai_vlan_global_cache_node_t *vplist = NULL;

    if(sai_is_vlan_created(vlan_id)) {
        SAI_VLAN_LOG_INFO("Vlan Id %d already created",
                vlan_id);
        return SAI_STATUS_ITEM_ALREADY_EXISTS;
    }

    vplist=(sai_vlan_global_cache_node_t *)calloc(1, sizeof(sai_vlan_global_cache_node_t));
    if(vplist == NULL) {
        SAI_VLAN_LOG_CRIT("No memory  to create Vlan Id %d memory:%d",
                vlan_id, sizeof(sai_vlan_global_cache_node_t));
        return SAI_STATUS_NO_MEMORY;
    }

    vplist->vlan_id=vlan_id;
    std_dll_init(&(vplist->member_list));
    global_vlan_list[vlan_id] = vplist;
    SAI_VLAN_LOG_TRACE("Vlan Id %d Inserted in cache",
            vlan_id);
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_remove_vlan_from_list(sai_vlan_id_t vlan_id)
{
    if(!sai_is_vlan_created(vlan_id)) {
        SAI_VLAN_LOG_WARN("Vlan Id %d not found", vlan_id);
        return SAI_STATUS_ITEM_NOT_FOUND;
    }

    if(global_vlan_list[vlan_id]->port_count > 0) {
        return SAI_STATUS_OBJECT_IN_USE;
    }

    free(global_vlan_list[vlan_id]);
    global_vlan_list[vlan_id] = NULL;
    SAI_VLAN_LOG_TRACE("Deleted Vlan Id %d", vlan_id);
    return SAI_STATUS_SUCCESS;
}

sai_status_t sai_add_vlan_member_node(sai_vlan_member_node_t vlan_member_info)
{
    sai_vlan_member_node_t *vlan_member_node = NULL;
    sai_vlan_member_dll_node_t *vlan_member_dll_node = NULL;
    sai_port_fwd_mode_t fwd_mode;
    sai_status_t ret_val = SAI_STATUS_SUCCESS;
    sai_vlan_id_t vlan_id = VLAN_UNDEF;

    vlan_id = sai_vlan_obj_id_to_vlan_id(vlan_member_info.vlan_id);

    do {
        if((vlan_member_node = (sai_vlan_member_node_t *)
                    calloc(1, sizeof(sai_vlan_member_node_t))) == NULL) {
            SAI_VLAN_LOG_CRIT("Unable to add  port 0x"PRIx64" Vlan Id %d \
                    memory %d unavailable",
                    vlan_member_info.port_id,
                    vlan_id,
                    sizeof(sai_vlan_member_node_t));
            ret_val = SAI_STATUS_NO_MEMORY;
            break;
        }

        if((vlan_member_dll_node = (sai_vlan_member_dll_node_t *)
                    calloc(1, sizeof(sai_vlan_member_dll_node_t))) == NULL) {
            SAI_VLAN_LOG_CRIT("Unable to add  port 0x"PRIx64" Vlan Id %d \
                    memory %d unavailable",
                    vlan_member_info.port_id,
                    vlan_member_info.vlan_id,
                    sizeof(sai_vlan_member_dll_node_t));
            ret_val = SAI_STATUS_NO_MEMORY;
            break;
        }

        *vlan_member_node = vlan_member_info;
        vlan_member_dll_node->vlan_member_info = vlan_member_node;

        if(std_rbtree_insert(global_vlan_member_tree, vlan_member_node)
                != STD_ERR_OK) {
            ret_val = SAI_STATUS_FAILURE;
            break;
        }

        std_dll_insertatback(&(global_vlan_list[vlan_id]->member_list),
                &(vlan_member_dll_node->node));
        global_vlan_list[vlan_id]->port_count++;

        if(!sai_is_port_vlan_configured(vlan_member_info.port_id)) {
            fwd_mode = SAI_PORT_FWD_MODE_SWITCHING;
            sai_port_forward_mode_info (vlan_member_info.port_id, &fwd_mode, true);
        }
        sai_increment_port_vlan_counter(vlan_member_info.port_id);

        SAI_VLAN_LOG_TRACE("Added  port 0x:%"PRIx64" Vlan Id %d",
                vlan_member_info.port_id, vlan_id);
    } while(0);

    if(SAI_STATUS_SUCCESS != ret_val) {
        free(vlan_member_node);
        free(vlan_member_dll_node);
    }

    return ret_val;
}

sai_vlan_member_node_t* sai_find_vlan_member_node(
        sai_object_id_t vlan_member_id)
{
    sai_vlan_member_node_t vlan_member_info, *vlan_member_node = NULL;

    if(sai_is_obj_id_vlan_member(vlan_member_id)) {
        memset(&vlan_member_info, 0, sizeof(vlan_member_info));
        vlan_member_info.vlan_member_id = vlan_member_id;
        vlan_member_node = (sai_vlan_member_node_t *)
            std_rbtree_getexact(global_vlan_member_tree, &vlan_member_info);
    }

    return vlan_member_node;
}

sai_vlan_member_dll_node_t* sai_find_vlan_member_node_from_port(sai_vlan_id_t vlan_id,
        sai_object_id_t port_id)
{
    sai_vlan_member_dll_node_t *vlan_member_dll_node = NULL;
    std_dll *node = NULL;

    if(global_vlan_list[vlan_id]) {
        for(node = std_dll_getfirst(&(global_vlan_list[vlan_id]->member_list));
                node != NULL;
                node = std_dll_getnext(&(global_vlan_list[vlan_id]->member_list),node)) {
            vlan_member_dll_node = (sai_vlan_member_dll_node_t *)node;
            if(vlan_member_dll_node->vlan_member_info->port_id == port_id)
                return vlan_member_dll_node;
        }
        SAI_VLAN_LOG_TRACE("port 0x%"PRIx64" not found on Vlan Id %d",
                port_id, vlan_id);
    }

    return NULL;
}

bool sai_is_vlan_obj_in_use(sai_vlan_id_t vlan_id)
{
    if((global_vlan_list[vlan_id]) &&
            (global_vlan_list[vlan_id]->port_count > 0)) {
        return true;
    } else {
        return false;
    }
}

bool sai_is_port_vlan_member(sai_vlan_id_t vlan_id,
        sai_object_id_t port_id)
{
    bool ret_val = false;

    if(sai_find_vlan_member_node_from_port(vlan_id, port_id) != NULL) {
        ret_val = true;
    }
    return ret_val;
}

sai_status_t sai_remove_vlan_member_node(sai_vlan_member_node_t vlan_member_info)
{
    sai_vlan_member_dll_node_t *vlan_member_dll_node = NULL;
    sai_vlan_member_node_t *vlan_member_node = NULL;
    sai_port_fwd_mode_t fwd_mode;
    sai_vlan_id_t vlan_id = VLAN_UNDEF;

    vlan_id = sai_vlan_obj_id_to_vlan_id(vlan_member_info.vlan_id);

    vlan_member_dll_node = sai_find_vlan_member_node_from_port(
            vlan_id, vlan_member_info.port_id);
    if(vlan_member_dll_node != NULL) {
        std_dll_remove(&(global_vlan_list[vlan_id]->member_list),
                &(vlan_member_dll_node->node));
        global_vlan_list[vlan_id]->port_count--;

        sai_decrement_port_vlan_counter(vlan_member_info.port_id);
        vlan_member_node = vlan_member_dll_node->vlan_member_info;
        vlan_member_node = std_rbtree_remove(global_vlan_member_tree,
                vlan_member_node);

        if(!sai_is_port_vlan_configured(vlan_member_info.port_id)) {
            fwd_mode = SAI_PORT_FWD_MODE_UNKNOWN;
            sai_port_forward_mode_info (vlan_member_info.port_id, &fwd_mode, true);
        }

        SAI_VLAN_LOG_TRACE("port 0x%"PRIx64" removed from Vlan Id %d",
                vlan_member_info.port_id, vlan_id);
        free(vlan_member_dll_node);
        free(vlan_member_node);
        return SAI_STATUS_SUCCESS;
    }
    return SAI_STATUS_INVALID_PORT_MEMBER;
}

sai_status_t sai_vlan_port_list_get(sai_vlan_id_t vlan_id,
        sai_object_list_t *vlan_port_list)
{
    sai_vlan_member_dll_node_t *vlan_member_dll_node = NULL;
    std_dll *node = NULL;
    unsigned int port_idx = 0;

    STD_ASSERT(vlan_port_list != NULL);
    if(vlan_port_list->count < global_vlan_list[vlan_id]->port_count) {
        vlan_port_list->count = global_vlan_list[vlan_id]->port_count;
        return SAI_STATUS_BUFFER_OVERFLOW;
    }
    for(node = std_dll_getfirst(&(global_vlan_list[vlan_id]->member_list));
            node != NULL;
            node = std_dll_getnext(&(global_vlan_list[vlan_id]->member_list),node)) {
        vlan_member_dll_node = (sai_vlan_member_dll_node_t *)node;
        vlan_port_list->list[port_idx] =
            vlan_member_dll_node->vlan_member_info->vlan_member_id;
        port_idx++;
    }

    vlan_port_list->count = port_idx;
    return SAI_STATUS_SUCCESS;
}

bool sai_is_valid_vlan_tagging_mode(sai_vlan_tagging_mode_t tagging_mode)
{
    switch(tagging_mode) {
        case SAI_VLAN_TAGGING_MODE_UNTAGGED:
        case SAI_VLAN_TAGGING_MODE_TAGGED:
        case SAI_VLAN_TAGGING_MODE_PRIORITY_TAGGED:
            return true;
    }
    return false;
}

void sai_vlan_learn_disable_cache_write(sai_vlan_id_t vlan_id, bool disable)
{
    global_vlan_list[vlan_id]->learn_disable = disable;
}

bool sai_vlan_learn_disable_cache_read(sai_vlan_id_t vlan_id)
{
    return global_vlan_list[vlan_id]->learn_disable;
}

void sai_vlan_max_learn_adddress_cache_write(sai_vlan_id_t vlan_id, unsigned int val)
{
    global_vlan_list[vlan_id]->max_learned_address = val;
}

unsigned int sai_vlan_max_learn_adddress_cache_read(sai_vlan_id_t vlan_id)
{
    return global_vlan_list[vlan_id]->max_learned_address;
}

void sai_vlan_meta_data_cache_write(sai_vlan_id_t vlan_id, unsigned int val)
{
    global_vlan_list[vlan_id]->meta_data = val;
}

unsigned int sai_vlan_meta_data_cache_read(sai_vlan_id_t vlan_id)
{
    return global_vlan_list[vlan_id]->meta_data;
}
