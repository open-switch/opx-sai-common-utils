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

/*
 * sai_map_utl.h
 */

#ifndef _SAI_MAP_UTIL_H_
#define _SAI_MAP_UTIL_H_

#include "saistatus.h"
#include "saitypes.h"

typedef enum {
    /*
     * Key
     * ---
     * sai_map_key_t.id1   : nhGroup Oid,
     *
     * Value
     * -----
     * sai_map_data_t.val1 : nhGroupMember Oid.
     *
     * nhGroupOid --> list <nhGroupMemberOids>
     */
    SAI_MAP_TYPE_NH_GRP_2_MEMBER_LIST,

    /*
     * Key
     * ---
     * sai_map_key_t.id1   : nhGroupMember Oid,
     *
     * Value
     * -----
     * sai_map_data_t.val1 : nhGroup Oid,
     * sai_map_data_t.val2 : nhId Oid,
     *
     * nhGroupMemberOid --> {nhGroupOid, nhIdOid}
     *
     * NOTE:
     * 'sai_map_val_t' will NOT be a list, but a single element.
     */
    SAI_MAP_TYPE_NH_MEMBER_2_GRP_INFO,

    /*
     * Key
     * ---
     * sai_map_key_t.id1   : tc map Oid,
     * sai_map_key_t.id1   : color map Oid,
     *
     * Value
     * -----
     * sai_map_data_t.val1 : port oid.
     * sai_map_data_t.val2 : tc and color map id.
     *
     * {tc_map_id, color_map_id} --> {port_id, tc_and_clor_id} list
     */
    SAI_MAP_TYPE_PORT_TC_AND_COLOR_MAP_LIST,

    /*
     * Key
     * ---
     * sai_map_key_t.id1   : lag oid,
     *
     * Value
     * -----
     * sai_map_data_t.val1 : rif oid.
     *
     * {lag_id} --> {rif_id} list
     */
    SAI_MAP_TYPE_LAG_RIF_INFO,
} sai_map_type_t;

typedef enum {
    /*
     * Bitmap values specifying which fields in 'sai_map_val_t'
     * should be matched.
     */
    SAI_MAP_VAL_FILTER_NONE = 0x00000001,
    SAI_MAP_VAL_FILTER_VAL1 = 0x00000002,
    SAI_MAP_VAL_FILTER_VAL2 = 0x00000004,
} sai_map_val_filter_t;

typedef struct _sai_map_key_t {
    /* Used to determine which of the subsequent fields are valid. */
    sai_map_type_t  type;
    sai_object_id_t id1;
    sai_object_id_t id2;
} sai_map_key_t;

typedef struct _sai_map_data_t {
    sai_object_id_t val1;
    sai_object_id_t val2;
} sai_map_data_t;

typedef struct _sai_map_val_t {
    /**
     * - In get operation, the calling function provides the buffer for the
     *   'data' field. The 'count' specifies the size of the list. If there
     *   are more elements to be filled, than the buffer could accomodate,
     *   then the function returns 'SAI_STATUS_BUFFER_OVERFLOW' and fills the
     *   'count' field with the total number of elements. The calling
     *   function must then invoke the function again, with the sufficient
     *   buffer to accomodate the elements.
     *
     * - In NON-Get operation, the calling function must set the 'data'
     *   field to the actual number of valid elements in the 'data' field.
     *
     * NOTE:
     *   The memory pointed to by 'data' must be freed by the calling function.
     */
    uint32_t        count;
    sai_map_data_t *data;
} sai_map_val_t;

#ifdef __cplusplus
extern "C"{
#endif

sai_status_t sai_map_insert (sai_map_key_t *key, sai_map_val_t *value);

sai_status_t sai_map_delete (sai_map_key_t *key);

sai_status_t sai_map_delete_elements (sai_map_key_t        *key,
                                      sai_map_val_t        *value,
                                      sai_map_val_filter_t  filter);

sai_status_t sai_map_get (sai_map_key_t *key, sai_map_val_t *val);

sai_status_t sai_map_get_elements (sai_map_key_t        *key,
                                   sai_map_val_t        *value,
                                   sai_map_val_filter_t  filter);

sai_status_t sai_map_get_val_count (sai_map_key_t *key, uint32_t *count);

#ifdef __cplusplus
}
#endif

#endif  /* _SAI_MAP_UTIL_H_ */
