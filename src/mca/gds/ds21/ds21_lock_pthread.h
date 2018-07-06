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


#ifndef DS21_LOCK_PTHREAD_H
#define DS21_LOCK_PTHREAD_H

#include <pthread.h>

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "src/mca/pshmem/pshmem.h"

#define _ESH_21_PTHREAD_LOCK(rwlock, func)                  \
__pmix_attribute_extension__ ({                             \
    pmix_status_t ret = PMIX_SUCCESS;                       \
    int rc;                                                 \
    rc = pthread_rwlock_##func(rwlock);                     \
    if (0 != rc) {                                          \
        switch (errno) {                                    \
            case EINVAL:                                    \
                ret = PMIX_ERR_INIT;                        \
                break;                                      \
            case EPERM:                                     \
                ret = PMIX_ERR_NO_PERMISSIONS;              \
                break;                                      \
        }                                                   \
    }                                                       \
    if (ret) {                                              \
        pmix_output(0, "%s %d:%s lock failed: %s",          \
            __FILE__, __LINE__, __func__, strerror(errno)); \
    }                                                       \
    ret;                                                    \
})

#define _DS_INIT(idx) pmix_ds21_lock_init(idx)
#define _DS_FINI(idx) pmix_ds21_lock_finalize(idx)
#define _DS_WR_LOCK(idx) _ESH_21_PTHREAD_LOCK(_ESH_SESSION_lock(idx), wrlock)
#define _DS_RD_LOCK(idx) _ESH_21_PTHREAD_LOCK(_ESH_SESSION_lock(idx), rdlock)
#define _DS_WR_UNLOCK(idx) _ESH_21_PTHREAD_LOCK(_ESH_SESSION_lock(idx), unlock)
#define _DS_RD_UNLOCK(idx) _ESH_21_PTHREAD_LOCK(_ESH_SESSION_lock(idx), unlock)

pmix_status_t pmix_ds21_lock_init(size_t session_idx);
void pmix_ds21_lock_finalize(size_t session_idx);

#endif // DSTORE_LOCK_PTHREAD_H
