/*
 * Copyright (c) 2015-2017 Intel, Inc. All rights reserved.
 * Copyright (c) 2017-2018 Mellanox Technologies, Inc.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef PMIX_DSTORE_H
#define PMIX_DSTORE_H

#if (defined(GDS_DS_VER)) && (GDS_DS_VER == 21)
#define ESH_PTHREAD_LOCK_21
#define GDS_DS_NAME_STR "ds21"
#elif GDS_DS_VER == 12
#define ESH_PTHREAD_LOCK_12
#define GDS_DS_NAME_STR "ds12"
#endif

#include <src/include/pmix_config.h>
#include "src/mca/gds/ds_common/dstore_seg.h"

#include "src/mca/gds/gds.h"
#include "src/mca/pshmem/pshmem.h"

BEGIN_C_DECLS

#include <src/include/pmix_config.h>
#include "src/class/pmix_value_array.h"

#define INITIAL_SEG_SIZE 4096
#define NS_META_SEG_SIZE (1<<22)
#define NS_DATA_SEG_SIZE (1<<22)

#define ESH_ENV_INITIAL_SEG_SIZE    "INITIAL_SEG_SIZE"
#define ESH_ENV_NS_META_SEG_SIZE    "NS_META_SEG_SIZE"
#define ESH_ENV_NS_DATA_SEG_SIZE    "NS_DATA_SEG_SIZE"
#define ESH_ENV_LINEAR              "SM_USE_LINEAR_SEARCH"

#define PMIX_DSTORE_ESH_BASE_PATH "PMIX_DSTORE_ESH_BASE_PATH"

typedef struct {
    pmix_value_array_t super;
    ns_map_data_t ns_map;
    size_t num_meta_seg;
    size_t num_data_seg;
    seg_desc_t *meta_seg;
    seg_desc_t *data_seg;
    bool in_use;
} ns_track_elem_t;

void dstore_finalize(void);

pmix_status_t dstore_setup_fork(const pmix_proc_t *peer, char ***env);

pmix_status_t dstore_cache_job_info(struct pmix_nspace_t *ns,
                                pmix_info_t info[], size_t ninfo);

pmix_status_t dstore_register_job_info(struct pmix_peer_t *pr,
                                pmix_buffer_t *reply);

pmix_status_t dstore_store_job_info(const char *nspace,
                                pmix_buffer_t *job_data);

pmix_status_t dstore_store(const pmix_proc_t *proc,
                                pmix_scope_t scope,
                                pmix_kval_t *kv);


pmix_status_t dstore_fetch(const pmix_proc_t *proc,
                                pmix_scope_t scope, bool copy,
                                const char *key,
                                pmix_info_t info[], size_t ninfo,
                                pmix_list_t *kvs);

pmix_status_t dstore_del_nspace(const char* nspace);

pmix_status_t dstore_assign_module(pmix_info_t *info, size_t ninfo,
                                int *priority);

pmix_status_t dstore_store_modex(struct pmix_nspace_t *nspace,
                                pmix_list_t *cbs,
                                pmix_byte_object_t *bo);

extern char *_base_path ;
extern uid_t _jobuid;
extern char _setjobuid;
extern ns_map_data_t * (*_esh_session_map_search)(const char *nspace);

int _esh_tbls_init(void);
ns_map_data_t * _esh_session_map_search_server(const char *nspace);
ns_map_data_t * _esh_session_map_search_client(const char *nspace);

END_C_DECLS

#endif
