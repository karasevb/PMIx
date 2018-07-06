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

#define _DS_INIT(idx) pmix_ds21_lock_init(idx)
#define _DS_FINI(idx) pmix_ds21_lock_finalize(idx)
#define _DS_WR_LOCK(idx) _esh_pthread_lock_w(idx)
#define _DS_RD_LOCK(idx) _esh_pthread_lock_r(idx)
#define _DS_WR_UNLOCK(idx) _esh_pthread_unlock_w(idx)
#define _DS_RD_UNLOCK(idx) _esh_pthread_unlock_r(idx)

pmix_status_t pmix_ds21_lock_init(size_t session_idx);
void pmix_ds21_lock_finalize(size_t session_idx);
pmix_status_t _esh_pthread_lock_w(size_t session_idx);
pmix_status_t _esh_pthread_unlock_w(size_t session_idx);
pmix_status_t _esh_pthread_lock_r(size_t session_idx);
pmix_status_t _esh_pthread_unlock_r(size_t session_idx);

#endif // DSTORE_LOCK_PTHREAD_H
