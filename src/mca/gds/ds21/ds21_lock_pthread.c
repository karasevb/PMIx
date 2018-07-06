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

#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "gds_dstore.h"
#include "ds21_lock_pthread.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/mca/pshmem/pshmem.h"
#include "src/include/pmix_globals.h"

#include "src/mca/gds/ds_common/dstore_lock.h"
#include "src/mca/gds/ds_common/dstore_seg.h"
#include "src/mca/gds/ds_common/dstore_session.h"

pmix_status_t pmix_ds21_lock_init(size_t session_idx)
{
    pmix_status_t rc = PMIX_SUCCESS;
    size_t size;
    pthread_mutexattr_t attr;
    uint32_t i;
    int page_size = _pmix_getpagesize();
    char *lockfile = _ESH_SESSION_lockfile(session_idx);
    char setjobuid = _ESH_SESSION_setjobuid(session_idx);
    uid_t jobuid = _ESH_SESSION_jobuid(session_idx);

    if ((NULL != _ESH_SESSION_pthread_seg(session_idx)) || (NULL != _ESH_SESSION_pthread_mutex(session_idx))) {
        rc = PMIX_ERR_INIT;
        return rc;
    }
    _ESH_SESSION_pthread_seg(session_idx) = (pmix_pshmem_seg_t *)malloc(sizeof(pmix_pshmem_seg_t));
    if (NULL == _ESH_SESSION_pthread_seg(session_idx)) {
        rc = PMIX_ERR_OUT_OF_RESOURCE;
        return rc;
    }

    size = (2 * _ESH_SESSION_numlocks(session_idx) * sizeof(pthread_mutex_t) / page_size + 1) * page_size;

    if (PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
        if (PMIX_SUCCESS != (rc = pmix_pshmem.segment_create(_ESH_SESSION_pthread_seg(session_idx), lockfile, size))) {
            return rc;
        }
        memset(_ESH_SESSION_pthread_seg(session_idx)->seg_base_addr, 0, size);
        if (setjobuid > 0) {
            if (0 > chown(lockfile, (uid_t) jobuid, (gid_t) -1)){
                rc = PMIX_ERROR;
                PMIX_ERROR_LOG(rc);
                return rc;
            }
            /* set the mode as required */
            if (0 > chmod(lockfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP )) {
                rc = PMIX_ERROR;
                PMIX_ERROR_LOG(rc);
                return rc;
            }
        }
        _ESH_SESSION_pthread_mutex(session_idx) = (pthread_mutex_t *)_ESH_SESSION_pthread_seg(session_idx)->seg_base_addr;

        if (0 != pthread_mutexattr_init(&attr)) {
            rc = PMIX_ERR_INIT;
            pmix_pshmem.segment_detach(_ESH_SESSION_pthread_seg(session_idx));
            return rc;
        }
        if (0 != pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED)) {
            rc = PMIX_ERR_INIT;
            pmix_pshmem.segment_detach(_ESH_SESSION_pthread_seg(session_idx));
            pthread_mutexattr_destroy(&attr);
            return rc;
        }

        for(i = 0; i < _ESH_SESSION_numlocks(session_idx) * 2; i++) {
            if (0 != pthread_mutex_init(&_ESH_SESSION_pthread_mutex(session_idx)[i], &attr)) {
                rc = PMIX_ERR_INIT;
                pmix_pshmem.segment_detach(_ESH_SESSION_pthread_seg(session_idx));
                pthread_mutexattr_destroy(&attr);
                return rc;
            }
        }
        if (0 != pthread_mutexattr_destroy(&attr)) {
            rc = PMIX_ERR_INIT;
            return rc;
        }

    }
    else {
        char *str;
        if( NULL != (str = getenv(ESH_ENV_LOCK_IDX)) ) {
            _ESH_SESSION_lockidx(session_idx) = strtoul(str, NULL, 10);
        }

        _ESH_SESSION_pthread_seg(session_idx)->seg_size = size;
        snprintf(_ESH_SESSION_pthread_seg(session_idx)->seg_name, PMIX_PATH_MAX, "%s", lockfile);
        if (PMIX_SUCCESS != (rc = pmix_pshmem.segment_attach(_ESH_SESSION_pthread_seg(session_idx), PMIX_PSHMEM_RW))) {
            return rc;
        }
        _ESH_SESSION_pthread_mutex(session_idx) = (pthread_mutex_t *)_ESH_SESSION_pthread_seg(session_idx)->seg_base_addr;
    }

    return rc;
}

void pmix_ds21_lock_finalize(size_t session_idx)
{
    pmix_status_t rc;
    uint32_t i;

    pmix_pshmem.segment_detach(_ESH_SESSION_pthread_seg(session_idx));

    if(PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
        for(i = 0; i < _ESH_SESSION_numlocks(session_idx) * 2; i++) {
            if (0 != pthread_mutex_destroy(&_ESH_SESSION_pthread_mutex(session_idx)[i])) {
                rc = PMIX_ERROR;
                PMIX_ERROR_LOG(rc);
                return;
            }
        }
        /* detach & unlink from current desc */
        if (_ESH_SESSION_pthread_seg(session_idx)->seg_cpid == getpid()) {
            pmix_pshmem.segment_unlink(_ESH_SESSION_pthread_seg(session_idx));
        }
    }

    free(_ESH_SESSION_pthread_seg(session_idx));
    _ESH_SESSION_pthread_seg(session_idx) = NULL;
    _ESH_SESSION_pthread_mutex(session_idx) = NULL;
}

pmix_status_t _esh_pthread_lock_w(size_t session_idx)
{
    pthread_mutex_t *locks = _ESH_SESSION_pthread_mutex(session_idx);
    uint32_t num_locks = _ESH_SESSION_numlocks(session_idx);
    uint32_t i;
    pmix_status_t rc = PMIX_SUCCESS;

    /* Lock the "signalling" lock first to let clients know that
     * server is going to get a write lock.
     * Clients do not hold this lock for a long time,
     * so this loop should be relatively dast.
     */
    for (i = 0; i < num_locks; i++) {
        pthread_mutex_lock(&locks[2*i]);
    }

    /* Now we can go and grab the main locks
     * New clients will be stopped at the previous
     * "barrier" locks.
     * We will wait here while all clients currently holding
     * locks will be done
     */
    for(i = 0; i < num_locks; i++) {
        pthread_mutex_lock(&locks[2*i + 1]);
    }

    /* TODO: consider rc from mutex functions */
    return rc;
}

pmix_status_t _esh_pthread_unlock_w(size_t session_idx)
{
    pthread_mutex_t *locks = _ESH_SESSION_pthread_mutex(session_idx);
    uint32_t num_locks = _ESH_SESSION_numlocks(session_idx);
    uint32_t i;
    pmix_status_t rc = PMIX_SUCCESS;

    /* Lock the second lock first to ensure that all procs will see
     * that we are trying to grab the main one */
    for(i=0; i<num_locks; i++) {
        pthread_mutex_unlock(&locks[2*i]);
        pthread_mutex_unlock(&locks[2*i + 1]);
    }
    /* TODO: consider rc from mutex functions */
    return rc;
}

pmix_status_t _esh_pthread_lock_r(size_t session_idx)
{
    pthread_mutex_t *locks = _ESH_SESSION_pthread_mutex(session_idx);
    uint32_t idx = _ESH_SESSION_lockidx(session_idx);

    /* This mutex is only used to acquire the next one,
     * this is a barrier that server is using to let clients
     * know that it is going to grab the write lock
     */
    pthread_mutex_lock(&locks[2 * idx]);

    /* Now grab the main lock */
    pthread_mutex_lock(&locks[2*idx + 1]);

    /* Once done - release signalling lock */
    pthread_mutex_unlock(&locks[2*idx]);

    /* TODO: consider rc from mutex functions */
    return 0;
}

pmix_status_t _esh_pthread_unlock_r(size_t session_idx)
{
    pthread_mutex_t *locks = _ESH_SESSION_pthread_mutex(session_idx);
    uint32_t idx = _ESH_SESSION_lockidx(session_idx);

    /* Release the main lock */
    pthread_mutex_unlock(&locks[2*idx + 1]);

    /* TODO: consider rc from mutex functions */
    return 0;
}
