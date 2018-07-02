#ifndef DSTORE_LOCK_H
#define DSTORE_LOCK_H

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "src/mca/pshmem/pshmem.h"

/* initialize the module */
typedef pmix_status_t (*pmix_ds_lock_init_fn_t)(pmix_pshmem_seg_t **rwlock_seg, pthread_rwlock_t **rwlock,
                                                char *lockfile, uid_t jobuid);
/* finalize the module */
typedef void (*pmix_ds_lock_fini_fn_t)(pmix_pshmem_seg_t **rwlock_seg, pthread_rwlock_t **rwlock);

typedef pmix_status_t (*pmix_ds_lock_wr_acq_fn_t)(pthread_rwlock_t *rwlock);

typedef pmix_status_t (*pmix_ds_lock_wr_rel_fn_t)(pthread_rwlock_t *rwlock);

typedef pmix_status_t (*pmix_ds_lock_rd_acq_fn_t)(pthread_rwlock_t *rwlock);

typedef pmix_status_t (*pmix_ds_lock_rd_rel_fn_t)(pthread_rwlock_t *rwlock);

typedef struct {
    const char *name;
    pmix_ds_lock_init_fn_t init;
    pmix_ds_lock_fini_fn_t fini;
    pmix_ds_lock_wr_acq_fn_t wr_lock;
    pmix_ds_lock_wr_rel_fn_t wr_unlock;
    pmix_ds_lock_rd_acq_fn_t rd_lock;
    pmix_ds_lock_rd_rel_fn_t rd_unlock;
} pmix_ds_lock_module_t;

int pmix_ds_lock_init(pmix_pshmem_seg_t **rwlock_seg, pthread_rwlock_t **rwlock, char *lockfile, uid_t jobuid);
void pmix_ds_lock_fini(pmix_pshmem_seg_t **rwlock_seg, pthread_rwlock_t **rwlock);
pmix_status_t pmix_ds_lock_wr_acq(pthread_rwlock_t *rwlock);
pmix_status_t pmix_ds_lock_rd_acq(pthread_rwlock_t *rwlock);
pmix_status_t pmix_ds_lock_rw_rel(pthread_rwlock_t *rwlock);

extern pmix_ds_lock_module_t ds_lock;

#endif // DSTORE_LOCK_H
