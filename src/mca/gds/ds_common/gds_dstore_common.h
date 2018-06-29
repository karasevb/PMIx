#ifndef PMIX_DS_COMMON_H
#define PMIX_DS_COMMON_H

#include <src/include/pmix_config.h>
#include "src/mca/gds/gds.h"
#include "src/mca/pshmem/pshmem.h"
#include "dstore_seg.h"

BEGIN_C_DECLS

#include <src/include/pmix_config.h>
#include "src/class/pmix_value_array.h"

#define INITIAL_SEG_SIZE 4096
#define NS_META_SEG_SIZE (1<<22)
#define NS_DATA_SEG_SIZE (1<<22)

#define PMIX_DSTORE_ESH_BASE_PATH "PMIX_DSTORE_ESH_BASE_PATH"

#ifdef HAVE_PTHREAD_SHARED
#define ESH_PTHREAD_LOCK
#elif defined HAVE_FCNTL_FLOCK
#define ESH_FCNTL_LOCK
#else
#error No locking mechanism was found
#endif

typedef struct session_s session_t;

struct session_s {
    int in_use;
    uid_t jobuid;
    char setjobuid;
    char *nspace_path;
    char *lockfile;
#ifdef ESH_PTHREAD_LOCK
    pmix_pshmem_seg_t *rwlock_seg;
    pthread_rwlock_t *rwlock;
#endif
    int lockfd;
    seg_desc_t *sm_seg_first;
    seg_desc_t *sm_seg_last;
};

typedef struct {
    pmix_value_array_t super;
    ns_map_data_t ns_map;
    size_t num_meta_seg;
    size_t num_data_seg;
    seg_desc_t *meta_seg;
    seg_desc_t *data_seg;
    bool in_use;
} ns_track_elem_t;

pmix_status_t dstore_init(pmix_info_t info[], size_t ninfo);

void dstore_finalize(void);

pmix_status_t dstore_setup_fork(const pmix_proc_t *peer, char ***env);

pmix_status_t dstore_cache_job_info(struct pmix_nspace_t *ns,
                                pmix_info_t info[], size_t ninfo);

pmix_status_t dstore_register_job_info(struct pmix_peer_t *pr,
                                pmix_buffer_t *reply);

pmix_status_t dstore_store_job_info(const char *nspace,
                                pmix_buffer_t *job_data);

pmix_status_t _dstore_store(const char *nspace,
                                    pmix_rank_t rank,
                                    pmix_kval_t *kv);

pmix_status_t dstore_store(const pmix_proc_t *proc,
                                pmix_scope_t scope,
                                pmix_kval_t *kv);

pmix_status_t _dstore_fetch(const char *nspace,
                                pmix_rank_t rank,
                                const char *key, pmix_value_t **kvs);

pmix_status_t dstore_fetch(const pmix_proc_t *proc,
                                pmix_scope_t scope, bool copy,
                                const char *key,
                                pmix_info_t info[], size_t ninfo,
                                pmix_list_t *kvs);

pmix_status_t dstore_add_nspace(const char *nspace,
                                pmix_info_t info[],
                                size_t ninfo);

pmix_status_t dstore_del_nspace(const char* nspace);

pmix_status_t dstore_assign_module(pmix_info_t *info, size_t ninfo,
                                int *priority);

pmix_status_t dstore_store_modex(struct pmix_nspace_t *nspace,
                                pmix_list_t *cbs,
                                pmix_byte_object_t *bo);

#ifdef ESH_PTHREAD_LOCK
/* initialize the module */
typedef pmix_status_t (*pmix_ds_lock_init_fn_t)(size_t tbl_idx);

/* finalize the module */
typedef void (*pmix_ds_lock_fini_fn_t)(size_t tbl_idx);

/* lock */
typedef pmix_status_t (*pmix_ds_lock_wr_acq_fn_t)(size_t tbl_idx);

typedef pmix_status_t (*pmix_ds_lock_wr_rel_fn_t)(size_t tbl_idx);

typedef pmix_status_t (*pmix_ds_lock_rd_acq_fn_t)(size_t tbl_idx);

typedef pmix_status_t (*pmix_ds_lock_rd_rel_fn_t)(size_t tbl_idx);

typedef struct {
    const char *name;
    pmix_ds_lock_init_fn_t init;
    pmix_ds_lock_fini_fn_t fini;
    pmix_ds_lock_wr_acq_fn_t wr_lock;
    pmix_ds_lock_wr_rel_fn_t wr_unlock;
    pmix_ds_lock_rd_acq_fn_t rd_lock;
    pmix_ds_lock_rd_rel_fn_t rd_unlock;
} pmix_ds_lock_module_t;

pmix_status_t _esh_pthread_lock_init(size_t idx);
void _esh_pthread_lock_fini(size_t idx);
pmix_status_t _esh_pthread_lock_w(size_t idx);
pmix_status_t _esh_pthread_lock_r(size_t idx);
pmix_status_t _esh_pthread_unlock(size_t idx);

#endif


END_C_DECLS

#endif
