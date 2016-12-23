/************************************************************************
* LEGALESE:   "Copyright (c) 2016, Dell Inc. All rights reserved."
*
* This source code is confidential, proprietary, and contains trade
* secrets that are the sole property of Dell Inc.
* Copy and/or distribution of this source code or disassembly or reverse
* engineering of the resultant object code are strictly forbidden without
* the written consent of Dell Inc.
*
************************************************************************/
/**
* @file sai_tunnel.h
*
* @brief This file contains the datastructure definitions for SAI Tunnel.
*
*************************************************************************/
#ifndef __SAI_TUNNEL_H__
#define __SAI_TUNNEL_H__

#include "saitunnel.h"
#include "saitypes.h"
#include "saistatus.h"
#include "std_type_defs.h"
#include "std_llist.h"
#include "std_rbtree.h"

/**
 * @brief SAI Tunnel data structure for the global parameters
 */
typedef struct _dn_sai_tunnel_global_t {

    /** Nodes of type dn_sai_tunnel_t */
    rbtree_handle    tunnel_db;

    /** Nodes of type dn_sai_tunnel_term_entry_t */
    rbtree_handle    tunnel_term_table_db;

    /** Nodes of type dn_sai_tunnel_map_t */
    rbtree_handle    tunnel_mapper_db;

    /** Bitmap for tunnel object index */
    uint8_t          *tunnel_obj_id_bitmap;

    /** Bitmap for tunnel map object index */
    uint8_t          *tunnel_map_id_bitmap;

    /** Bitmap for tunnel map object index */
    uint8_t          *tunnel_term_id_bitmap;

    /** flag to indicate if global params are initialized */
    bool             is_init_complete;
} dn_sai_tunnel_global_t;

/**
 * @brief SAI Tunnel attributes structure.
 *
 */
typedef struct _dn_sai_tunnel_params_t {

    /** Tunnel TTL mode attribute. */
    sai_tunnel_ttl_mode_t  ttl_mode;
    /** Tunnel DSCP mode attribute. */
    sai_tunnel_dscp_mode_t dscp_mode;
    /** TTL value for user defined tunnel ttl mode. */
    sai_uint8_t            ttl;
    /** DSCP value for user defined tunnel dscp mode. */
    sai_uint8_t            dscp;

} dn_sai_tunnel_params_t;

/**
 * @brief SAI Tunnel object data structure.
 * Contains the encap and decap attributes.
 *
 */
typedef struct _dn_sai_tunnel_t {

    /** Tunnel Id. Key parameter for the tunnel db */
    sai_object_id_t         tunnel_id;

    sai_tunnel_type_t       tunnel_type;
    sai_object_id_t         underlay_rif;
    sai_object_id_t         overlay_rif;
    sai_object_id_t         underlay_vrf;
    sai_object_id_t         overlay_vrf;

    /** Encap attributes */
    sai_ip_address_t        src_ip;
    dn_sai_tunnel_params_t  encap;

    /** Decap attributes */
    dn_sai_tunnel_params_t  decap;

    /** List of Tunnel Encap Next Hops in the tunnel. */
    std_dll_head            tunnel_encap_nh_list;

    /** List of Tunnel Termination entries in the tunnel. */
    std_dll_head            tunnel_term_entry_list;

    /** List of Tunnel Encap Mappers in the tunnel. */
    std_dll_head            tunnel_encap_mapper_list;

    /** List of Tunnel Decap Mappers in the tunnel. */
    std_dll_head            tunnel_decap_mapper_list;

    /** Place holder for NPU-specific data */
    void                   *hw_info;
} dn_sai_tunnel_t;

/**
 * @brief SAI Tunnel Termination table entry data structure.
 *
 */
typedef struct _dn_sai_tunnel_term_entry_t {

    /** Tunnel termination entry Id. */
    sai_object_id_t                term_entry_id;

    /** Tunnel termination entry VR Id. */
    sai_object_id_t                vr_id;

    /** Tunnel termination entry type. */
    sai_tunnel_term_table_entry_type_t type;

    /** Tunnel termination entry keys. */
    sai_ip_address_t               src_ip;
    sai_ip_address_t               dst_ip;

    sai_tunnel_type_t              tunnel_type;

    /** Tunnel object id. */
    sai_object_id_t                tunnel_id;

    /** Tunnel node list pointers */
    std_dll                        tunnel_link;

} dn_sai_tunnel_term_entry_t;


/**
 * @brief SAI Tunnel Mapper data structure.
 *
 */
typedef struct _dn_sai_tunnel_map_t {

    /** Tunnel Mapper object Id. */
    sai_object_id_t                mapper_id;

    sai_tunnel_map_type_t          type;

    sai_tunnel_map_list_t          list;

    /** Tunnel node list pointers */
    std_dll                        tunnel_link;

    uint_t                         ref_count;

    /** Place holder for NPU-specific data */
    void                          *hw_info;
} dn_sai_tunnel_map_t;




#endif /* __SAI_TUNNEL_H__ */
