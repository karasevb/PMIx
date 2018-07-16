/*
 * Copyright (c) 2015-2018 Intel, Inc. All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * Copyright (c) 2016-2018 Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2018      Research Organization for Information Science
 *                         and Technology (RIST).  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <stdio.h>
#include <sys/stat.h>

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "src/include/pmix_globals.h"
#include "src/mca/gds/base/base.h"
#include "src/mca/pshmem/base/base.h"
#include "src/util/error.h"
#include "src/util/output.h"

#include "src/mca/gds/ds_common/dstore_seg.h"
#include "src/mca/gds/ds_common/dstore_nspace.h"

#define INITIAL_SEG_SIZE 4096
#define NS_META_SEG_SIZE (1<<22)
#define NS_DATA_SEG_SIZE (1<<22)

#define ESH_REGION_EXTENSION        "EXTENSION_SLOT"
#define ESH_REGION_INVALIDATED      "INVALIDATED"
#define ESH_ENV_INITIAL_SEG_SIZE    "INITIAL_SEG_SIZE"
#define ESH_ENV_NS_META_SEG_SIZE    "NS_META_SEG_SIZE"
#define ESH_ENV_NS_DATA_SEG_SIZE    "NS_DATA_SEG_SIZE"
#define ESH_ENV_LINEAR              "SM_USE_LINEAR_SEARCH"

size_t _initial_segment_size = 0;
size_t _max_ns_num;
size_t _meta_segment_size = 0;
size_t _max_meta_elems;
size_t _data_segment_size = 0;
size_t _lock_segment_size = 0;

/* If _direct_mode is set, it means that we use linear search
 * along the array of rank meta info objects inside a meta segment
 * to find the requested rank. Otherwise,  we do a fast lookup
 * based on rank and directly compute offset.
 * This mode is called direct because it's effectively used in
 * sparse communication patterns when direct modex is usually used.
 */
int _direct_mode = 0;

int _pmix_getpagesize(void)
{
#if defined(_SC_PAGESIZE )
    return sysconf(_SC_PAGESIZE);
#elif defined(_SC_PAGE_SIZE)
    return sysconf(_SC_PAGE_SIZE);
#else
    return 65536; /* safer to overestimate than under */
#endif
}

seg_desc_t *_create_new_segment(segment_type type, const char *name,
                                const char *path, char setjobuid, uint32_t id)
{
    pmix_status_t rc;
    char file_name[PMIX_PATH_MAX];
    size_t size;
    seg_desc_t *new_seg = NULL;

    PMIX_OUTPUT_VERBOSE((10, pmix_gds_base_framework.framework_output,
                         "%s:%d:%s: segment type %d, nspace %s, id %u",
                         __FILE__, __LINE__, __func__, type, name, id));

    switch (type) {
        case INITIAL_SEGMENT:
            size = _initial_segment_size;
            snprintf(file_name, PMIX_PATH_MAX, "%s/initial-pmix_shared-segment-%u",
                path, id);
            break;
        case NS_META_SEGMENT:
            size = _meta_segment_size;
            snprintf(file_name, PMIX_PATH_MAX, "%s/smseg-%s-%u",
                path, name, id);
            break;
        case NS_DATA_SEGMENT:
            size = _data_segment_size;
            snprintf(file_name, PMIX_PATH_MAX, "%s/smdataseg-%s-%d",
                path, name, id);
            break;
        default:
            PMIX_ERROR_LOG(PMIX_ERROR);
            return NULL;
    }
    new_seg = (seg_desc_t*)malloc(sizeof(seg_desc_t));
    if (new_seg) {
        new_seg->id = id;
        new_seg->next = NULL;
        new_seg->type = type;
        rc = pmix_pshmem.segment_create(&new_seg->seg_info, file_name, size);
        if (PMIX_SUCCESS != rc) {
            PMIX_ERROR_LOG(rc);
            goto err_exit;
        }
        memset(new_seg->seg_info.seg_base_addr, 0, size);


        if (setjobuid > 0){
            rc = PMIX_ERR_PERM;
            if (0 > chown(file_name, (uid_t) setjobuid, (gid_t) -1)){
                PMIX_ERROR_LOG(rc);
                goto err_exit;
            }
            /* set the mode as required */
            if (0 > chmod(file_name, S_IRUSR | S_IRGRP | S_IWGRP )) {
                PMIX_ERROR_LOG(rc);
                goto err_exit;
            }
        }
    }
    return new_seg;

err_exit:
    if( NULL != new_seg ){
        free(new_seg);
    }
    return NULL;
}

seg_desc_t *_attach_new_segment(segment_type type, const char *name,
                                const char *path, uint32_t id)
{
    pmix_status_t rc;
    seg_desc_t *new_seg = NULL;
    new_seg = (seg_desc_t*)malloc(sizeof(seg_desc_t));
    new_seg->id = id;
    new_seg->next = NULL;
    new_seg->type = type;

    PMIX_OUTPUT_VERBOSE((10, pmix_gds_base_framework.framework_output,
                         "%s:%d:%s: segment type %d, nspace %s, id %u",
                         __FILE__, __LINE__, __func__, type, name, id));

    switch (type) {
        case INITIAL_SEGMENT:
            new_seg->seg_info.seg_size = _initial_segment_size;
            snprintf(new_seg->seg_info.seg_name, PMIX_PATH_MAX, "%s/initial-pmix_shared-segment-%u",
                path, id);
            break;
        case NS_META_SEGMENT:
            new_seg->seg_info.seg_size = _meta_segment_size;
            snprintf(new_seg->seg_info.seg_name, PMIX_PATH_MAX, "%s/smseg-%s-%u",
                path, name, id);
            break;
        case NS_DATA_SEGMENT:
            new_seg->seg_info.seg_size = _data_segment_size;
            snprintf(new_seg->seg_info.seg_name, PMIX_PATH_MAX, "%s/smdataseg-%s-%d",
                path, name, id);
            break;
        default:
            free(new_seg);
            PMIX_ERROR_LOG(PMIX_ERROR);
            return NULL;
    }
    rc = pmix_pshmem.segment_attach(&new_seg->seg_info, PMIX_PSHMEM_RONLY);
    if (PMIX_SUCCESS != rc) {
        free(new_seg);
        new_seg = NULL;
        PMIX_ERROR_LOG(rc);
    }
    return new_seg;
}

int _put_ns_info_to_initial_segment(seg_desc_t *seg_desc_last, const char *name,
                                    const char *path, size_t tbl_idx, char setjobuid)
{
    ns_seg_info_t elem;
    size_t num_elems;
    num_elems = *((size_t*)(seg_desc_last->seg_info.seg_base_addr));
    seg_desc_t *last_seg = seg_desc_last;
    pmix_status_t rc;

    PMIX_OUTPUT_VERBOSE((10, pmix_gds_base_framework.framework_output,
                         "%s:%d:%s", __FILE__, __LINE__, __func__));

    if (_max_ns_num == num_elems) {
        num_elems = 0;
        if (NULL == (last_seg = extend_segment(last_seg, name, path, setjobuid))) {
            rc = PMIX_ERROR;
            PMIX_ERROR_LOG(rc);
            return rc;
        }
        /* mark previous segment as full */
        size_t full = 1;
        memcpy((uint8_t*)(seg_desc_last->seg_info.seg_base_addr + sizeof(size_t)), &full, sizeof(size_t));
        seg_desc_last = last_seg;
        memset(seg_desc_last->seg_info.seg_base_addr, 0, _initial_segment_size);
    }
    memset(&elem.ns_map, 0, sizeof(elem.ns_map));
    strncpy(elem.ns_map.name, name, sizeof(elem.ns_map.name)-1);
    elem.ns_map.tbl_idx = tbl_idx;
    elem.num_meta_seg = 1;
    elem.num_data_seg = 1;
    memcpy((uint8_t*)(seg_desc_last->seg_info.seg_base_addr) + sizeof(size_t) * 2 + num_elems * sizeof(ns_seg_info_t),
            &elem, sizeof(ns_seg_info_t));
    num_elems++;
    memcpy((uint8_t*)(seg_desc_last->seg_info.seg_base_addr), &num_elems, sizeof(size_t));
    return PMIX_SUCCESS;
}

/* clients should sync local info with information from initial segment regularly */
void _update_initial_segment_info(seg_desc_t *seg_desc_first, const char *name,
                                  const char *path)
{
    seg_desc_t *tmp;
    tmp = seg_desc_first;

    PMIX_OUTPUT_VERBOSE((2, pmix_gds_base_framework.framework_output,
                         "%s:%d:%s", __FILE__, __LINE__, __func__));

    /* go through all global segments */
    do {
        /* check if current segment was marked as full but no more next segment is in the chain */
        if (NULL == tmp->next && 1 == *((size_t*)((uint8_t*)(tmp->seg_info.seg_base_addr) + sizeof(size_t)))) {
            tmp->next = _attach_new_segment(INITIAL_SEGMENT, name, path, tmp->id+1);
        }
        tmp = tmp->next;
    }
    while (NULL != tmp);
}

/* this function will be used by clients to get ns data from the initial segment and add them to the tracker list */
ns_seg_info_t *_get_ns_info_from_initial_segment(seg_desc_t *seg_desc_first, const char *name)
{
    pmix_status_t rc;
    size_t i;
    seg_desc_t *tmp;
    ns_seg_info_t *elem, *cur_elem;
    elem = NULL;
    size_t num_elems;

    PMIX_OUTPUT_VERBOSE((2, pmix_gds_base_framework.framework_output,
                         "%s:%d:%s", __FILE__, __LINE__, __func__));

    tmp = seg_desc_first;

    rc = 1;
    /* go through all global segments */
    do {
        num_elems = *((size_t*)(tmp->seg_info.seg_base_addr));
        for (i = 0; i < num_elems; i++) {
            cur_elem = (ns_seg_info_t*)((uint8_t*)(tmp->seg_info.seg_base_addr) + sizeof(size_t) * 2 + i * sizeof(ns_seg_info_t));
            if (0 == (rc = strncmp(cur_elem->ns_map.name, name, strlen(name)+1))) {
                break;
            }
        }
        if (0 == rc) {
            elem = cur_elem;
            break;
        }
        tmp = tmp->next;
    }
    while (NULL != tmp);
    return elem;
}

seg_desc_t *extend_segment(seg_desc_t *segdesc, const char *name,
                           const char *path, char setjobuid)
{
    seg_desc_t *tmp, *seg;

    PMIX_OUTPUT_VERBOSE((2, pmix_gds_base_framework.framework_output,
                         "%s:%d:%s",
                         __FILE__, __LINE__, __func__));
    /* find last segment */
    tmp = segdesc;
    while (NULL != tmp->next) {
        tmp = tmp->next;
    }
    /* create another segment, the old one is full. */
    seg = _create_new_segment(segdesc->type, name, path, setjobuid, tmp->id + 1);
    tmp->next = seg;

    return seg;
}

void _delete_sm_desc(seg_desc_t *desc)
{
    seg_desc_t *tmp;

    /* free all global segments */
    while (NULL != desc) {
        tmp = desc->next;
        /* detach & unlink from current desc */
        if (desc->seg_info.seg_cpid == getpid()) {
            pmix_pshmem.segment_unlink(&desc->seg_info);
        }
        pmix_pshmem.segment_detach(&desc->seg_info);
        free(desc);
        desc = tmp;
    }
}
