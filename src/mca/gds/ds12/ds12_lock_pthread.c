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
#include "ds12_lock_pthread.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/mca/pshmem/pshmem.h"
#include "src/include/pmix_globals.h"

#include "src/mca/gds/ds_common/dstore_lock.h"
#include "src/mca/gds/ds_common/dstore_seg.h"
#include "src/mca/gds/ds_common/dstore_session.h"

pmix_status_t pmix_ds12_lock_init(size_t session_idx)
{
    pmix_status_t rc = PMIX_SUCCESS;
    size_t size = _lock_segment_size;
    pthread_rwlockattr_t attr;
    char *lockfile = _ESH_SESSION_lockfile(session_idx);
    char setjobuid = _ESH_SESSION_setjobuid(session_idx);
    uid_t jobuid = _ESH_SESSION_jobuid(session_idx);

    if ((NULL != _ESH_SESSION_pthread_rwlock(session_idx)) || (NULL != _ESH_SESSION_pthread_seg(session_idx))) {
        rc = PMIX_ERR_INIT;
        return rc;
    }
    _ESH_SESSION_pthread_seg(session_idx) = (pmix_pshmem_seg_t *)malloc(sizeof(pmix_pshmem_seg_t));
    if (NULL == _ESH_SESSION_pthread_seg(session_idx)) {
        rc = PMIX_ERR_OUT_OF_RESOURCE;
        return rc;
    }
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
        _ESH_SESSION_pthread_rwlock(session_idx) = (pthread_rwlock_t *)_ESH_SESSION_pthread_seg(session_idx)->seg_base_addr;

        if (0 != pthread_rwlockattr_init(&attr)) {
            rc = PMIX_ERR_INIT;
            pmix_pshmem.segment_detach(_ESH_SESSION_pthread_seg(session_idx));
            return rc;
        }
        if (0 != pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED)) {
            rc = PMIX_ERR_INIT;
            pmix_pshmem.segment_detach(_ESH_SESSION_pthread_seg(session_idx));
            pthread_rwlockattr_destroy(&attr);
            return rc;
        }
#ifdef HAVE_PTHREAD_SETKIND
        if (0 != pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP)) {
            rc = PMIX_ERR_INIT;
            pmix_pshmem.segment_detach(_ESH_SESSION_pthread_seg(session_idx));
            pthread_rwlockattr_destroy(&attr);
            return rc;
        }
#endif
        if (0 != pthread_rwlock_init(_ESH_SESSION_pthread_rwlock(session_idx), &attr)) {
            rc = PMIX_ERR_INIT;
            pmix_pshmem.segment_detach(_ESH_SESSION_pthread_seg(session_idx));
            pthread_rwlockattr_destroy(&attr);
            return rc;
        }
        if (0 != pthread_rwlockattr_destroy(&attr)) {
            rc = PMIX_ERR_INIT;
            return rc;
        }

    }
    else {
        _ESH_SESSION_pthread_seg(session_idx)->seg_size = size;
        snprintf(_ESH_SESSION_pthread_seg(session_idx)->seg_name, PMIX_PATH_MAX, "%s", lockfile);
        if (PMIX_SUCCESS != (rc = pmix_pshmem.segment_attach(_ESH_SESSION_pthread_seg(session_idx), PMIX_PSHMEM_RW))) {
            return rc;
        }
        _ESH_SESSION_pthread_rwlock(session_idx) = (pthread_rwlock_t *)_ESH_SESSION_pthread_seg(session_idx)->seg_base_addr;
    }

    return rc;
}

void pmix_ds12_lock_finalize(size_t session_idx)
{
    pmix_status_t rc;

    if (0 != pthread_rwlock_destroy(_ESH_SESSION_pthread_rwlock(session_idx))) {
        rc = PMIX_ERROR;
        PMIX_ERROR_LOG(rc);
        return;
    }

    /* detach & unlink from current desc */
    if (_ESH_SESSION_pthread_seg(session_idx)->seg_cpid == getpid()) {
        pmix_pshmem.segment_unlink(_ESH_SESSION_pthread_seg(session_idx));
    }
    pmix_pshmem.segment_detach(_ESH_SESSION_pthread_seg(session_idx));

    free(_ESH_SESSION_pthread_seg(session_idx));
    _ESH_SESSION_pthread_seg(session_idx) = NULL;
    _ESH_SESSION_pthread_rwlock(session_idx) = NULL;
}
