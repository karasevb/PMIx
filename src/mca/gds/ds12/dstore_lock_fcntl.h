#ifndef DSTORE_LOCK_FCNTL_H
#define DSTORE_LOCK_FCNTL_H

#include <fcntl.h>

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "dstore_lock.h"

int pmix_ds_lock_init(const char *lockfile, int *lockfd, char setjobuid, uid_t jobuid);
pmix_status_t pmix_ds_lock_wr_acq(int lockfd);
pmix_status_t pmix_ds_lock_rd_acq(int lockfd);
pmix_status_t pmix_ds_lock_rw_rel(int lockfd);

/* initialize the module */
typedef pmix_status_t (*pmix_ds_lock_init_fn_t)(const char *lockfile, int *lockfd, char setjobuid, uid_t jobuid);

typedef pmix_status_t (*pmix_ds_lock_wr_acq_fn_t)(int lockfd);

typedef pmix_status_t (*pmix_ds_lock_wr_rel_fn_t)(int lockfd);

typedef pmix_status_t (*pmix_ds_lock_rd_acq_fn_t)(int lockfd);

typedef pmix_status_t (*pmix_ds_lock_rd_rel_fn_t)(int lockfd);

typedef struct {
    const char *name;
    pmix_ds_lock_init_fn_t init;
    pmix_ds_lock_fini_fn_t fini;
    pmix_ds_lock_wr_acq_fn_t wr_lock;
    pmix_ds_lock_wr_rel_fn_t wr_unlock;
    pmix_ds_lock_rd_acq_fn_t rd_lock;
    pmix_ds_lock_rd_rel_fn_t rd_unlock;
} pmix_ds_lock_module_t;

extern pmix_ds_lock_module_t ds_lock;

#endif // DSTORE_LOCK_FCNTL_H
