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

#ifndef DSTORE_SESSION_H
#define DSTORE_SESSION_H
 
#include <pmix_common.h>
#include <src/include/pmix_config.h>

#include "src/mca/gds/ds_common/dstore_lock.h"
#include "src/mca/gds/ds_common/dstore_seg.h"

typedef struct session_s session_t;

struct session_s {
    int in_use;
    uid_t jobuid;
    char setjobuid;
    char *nspace_path;
    char *lockfile;
#ifdef ESH_PTHREAD_LOCK
    pmix_pshmem_seg_t *lock_seg;
    pthread_rwlock_t *lock;
#endif
#ifdef xESH_PTHREAD_LOCK_21
    pmix_pshmem_seg_t *lock_seg;
    pthread_mutex_t *lock;
    uint32_t num_locks, num_forked, num_procs;
    uint32_t lock_idx;
#endif
    int lockfd;
    seg_desc_t *sm_seg_first;
    seg_desc_t *sm_seg_last;
};


#define _ESH_SESSION_path(tbl_idx)         (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].nspace_path)
#define _ESH_SESSION_lockfile(tbl_idx)     (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].lockfile)
#define _ESH_SESSION_setjobuid(tbl_idx)    (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].setjobuid)
#define _ESH_SESSION_jobuid(tbl_idx)       (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].jobuid)
#define _ESH_SESSION_sm_seg_first(tbl_idx) (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].sm_seg_first)
#define _ESH_SESSION_sm_seg_last(tbl_idx)  (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].sm_seg_last)
#define _ESH_SESSION_ns_info(tbl_idx)      (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].ns_info)
#define _ESH_SESSION_in_use(tbl_idx)       (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].in_use)
#define _ESH_SESSION_lockfd(tbl_idx)       (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].lockfd)

#ifdef ESH_PTHREAD_LOCK
#define _ESH_SESSION_pthread_rwlock(tbl_idx) (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].lock)
#define _ESH_SESSION_pthread_seg(tbl_idx)   (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].lock_seg)
#define _ESH_SESSION_lock(tbl_idx)          _ESH_SESSION_pthread_rwlock(tbl_idx)
#endif
#ifdef xESH_PTHREAD_LOCK_21
#define _ESH_SESSION_numlocks(tbl_idx)     (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].num_locks)
#define _ESH_SESSION_numprocs(tbl_idx)     (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].num_procs)
#define _ESH_SESSION_numforked(tbl_idx)    (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].num_fork)
#define _ESH_SESSION_pthread_mutex(tbl_idx) (PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t)[tbl_idx].lock)
#endif

#ifdef ESH_FCNTL_LOCK
#define _ESH_SESSION_lock(tbl_idx)         _ESH_SESSION_lockfd(tbl_idx)
#endif

extern pmix_value_array_t *_session_array;

#endif // DSTORE_SESSION_H
