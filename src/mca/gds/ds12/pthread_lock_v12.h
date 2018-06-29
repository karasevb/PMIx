#ifndef PTHREAD_LOCK_V12_H
#define PTHREAD_LOCK_V12_H

#include <pmix_common.h>

/* initialize the module */
typedef pmix_status_t (*pmix_ds_lock_init_fn_t)(size_t tbl_idx);

/* finalize the module */
typedef void (*pmix_ds_lock_fini_fn_t)(void);

/* lock */
typedef pmix_status_t (*pmix_ds_lock_rw_acq_fn_t)(size_t tbl_idx);

typedef pmix_status_t (*pmix_ds_lock_rw_rel_fn_t)(size_t tbl_idx);

typedef pmix_status_t (*pmix_ds_lock_rd_acq_fn_t)(size_t tbl_idx);

typedef pmix_status_t (*pmix_ds_lock_rd_rel_fn_t)(size_t tbl_idx);

typedef struct {
    const char *name;
    pmix_ds_lock_init_fn_t init;
    pmix_ds_lock_fini_fn_t fini;
    pmix_ds_lock_rw_acq_fn_t rw_lock;
    pmix_ds_lock_rw_rel_fn_t rw_unlock;
    pmix_ds_lock_rd_acq_fn_t rd_lock;
    pmix_ds_lock_rd_rel_fn_t rd_unlock;
} pmix_ds_lock_module;


#endif // PTHREAD_LOCK_V12_H
