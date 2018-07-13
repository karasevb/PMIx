/*
 * Copyright (c) 2018      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef PMIX_GDS_DS_BASE_H_
#define PMIX_GDS_DS_BASE_H_

#include <pthread.h>
#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "src/include/pmix_globals.h"
#include "src/class/pmix_list.h"
#include "src/mca/gds/gds.h"
#include "src/mca/pshmem/pshmem.h"

typedef void* pmix_common_dstor_lock_ctx_t;

typedef pmix_common_dstor_lock_ctx_t (*pmix_common_dstor_lock_init_fn_t)(const char *base_path, uid_t uid, bool setuid);
typedef void (*pmix_common_dstor_lock_finalize_fn_t)(pmix_common_dstor_lock_ctx_t *ctx);
typedef pmix_status_t (*pmix_common_dstor_lock_rd_get_fn_t)(pmix_common_dstor_lock_ctx_t *ctx);
typedef pmix_status_t (*pmix_common_dstor_lock_rd_rel_fn_t)(pmix_common_dstor_lock_ctx_t *ctx);
typedef pmix_status_t (*pmix_common_dstor_lock_wr_get_fn_t)(pmix_common_dstor_lock_ctx_t *ctx);
typedef pmix_status_t (*pmix_common_dstor_lock_wr_rel_fn_t)(pmix_common_dstor_lock_ctx_t *ctx);

typedef struct {
    pmix_common_dstor_lock_init_fn_t init;
    pmix_common_dstor_lock_finalize_fn_t finalize;
    pmix_common_dstor_lock_rd_get_fn_t rd_lock;
    pmix_common_dstor_lock_rd_rel_fn_t rd_unlock;
    pmix_common_dstor_lock_wr_get_fn_t wr_lock;
    pmix_common_dstor_lock_wr_rel_fn_t wr_unlock;
} pmix_common_lock_callbacks_t;

typedef struct pmix_common_dstore_ctx_s pmix_common_dstore_ctx_t;

/* Goes to the internal header
struct pmix_dstore_ctx_s {
	pmix_dstor_lock_callbacks_t *lock_cbs;

	pmix_value_array_t *session_array;
	pmix_value_array_t *ns_map_array;

	gds_ds_base_hndl_init_fn_t init;
	gds_ds_base_hndl_finalize_fn_t finalize;
} pmix_dstore_ctx_t;

*/

/*
 * Goes to internal header
typedef struct {
    int in_use;
    ns_map_data_t data;
} ns_map_t;

typedef struct {
    char name[PMIX_MAX_NSLEN+1];
    size_t tbl_idx;
    int track_idx;
} ns_map_data_t;

typedef struct {
	int in_use;
	uid_t jobuid;
	char setjobuid;
	char *nspace_path;
	seg_desc_t *sm_seg_first;
	seg_desc_t *sm_seg_last;
} gds_ds_base_session_t;
*/

pmix_dstore_ctx_t *pmix_common_dstor_init(pmix_dstor_lock_callbacks_t *lock_cb,
                                   const char *paths, const char *ds_name);
pmix_status_t pmix_common_dstor_finalize(pmix_dstore_ctx_t *ctx);

/*
typedef pmix_status_t (*gds_ds_base_hndl_assign_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		pmix_info_t *info, size_t ninfo, int *priority);
typedef pmix_status_t (*gds_ds_base_hndl_job_info_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		struct pmix_nspace_t *ns, pmix_info_t info[], size_t ninfo);
typedef pmix_status_t (*gds_ds_base_hndl_register_job_info_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		struct pmix_peer_t *pr, pmix_buffer_t *reply);
typedef pmix_status_t (*gds_ds_base_hndl_store_job_info_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		const char *nspace, pmix_buffer_t *buf);
typedef pmix_status_t (*gds_ds_base_hndl_store_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		const pmix_proc_t *proc, pmix_scope_t scope, pmix_kval_t *kv);
typedef pmix_status_t (*gds_ds_base_hndl_store_modex_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		struct pmix_nspace_t *ns, pmix_list_t *cbs, pmix_byte_object_t *bo);
typedef pmix_status_t (*gds_ds_base_hndl_fetch_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		const pmix_proc_t *proc, pmix_scope_t scope, bool copy, const char *key,
		pmix_info_t info[], size_t ninfo, pmix_list_t *kvs);
typedef pmix_status_t (*gds_ds_base_hndl_setup_fork_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		const pmix_proc_t *proc, r ***env);
typedef pmix_status_t (*gds_ds_base_hndl_add_nspace_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		const char *nspace, pmix_info_t info[], size_t ninfo);
typedef pmix_status_t (*gds_ds_base_hndl_del_nspace_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		const char* nspace);
typedef pmix_status_t (*gds_ds_base_hndl_assemb_kvs_req_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		const pmix_proc_t *proc, pmix_list_t *kvs, pmix_buffer_t *buf, void *cbdata);
typedef pmix_status_t (*gds_ds_base_hndl_accept_kvs_resp_fn_t)(gds_ds_base_hndl_t *ds_hndl,
		pmix_buffer_t *buf);
*/

#endif
