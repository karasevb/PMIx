/*
 * Copyright (c) 2018      Mellanox Technologies, Inc.
 *                         All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "src/mca/common/pmix_common_dstore.h"
#include "src/mca/pshmem/pshmem.h"

#include "src/util/error.h"
#include "src/util/output.h"

typedef struct {
    char *lockfile;
    pmix_pshmem_seg_t *segment;
    pthread_mutex_t *mutex;
    uint32_t num_locks;
    uint32_t num_forked;
    uint32_t num_procs;
    uint32_t lock_idx;
} ds21_lock_pthread_ctx_t;

pmix_common_dstor_lock_ctx_t *pmix_gds_ds21_lock_init(const char *base_path, uid_t uid, bool setuid)
{
    ds21_lock_pthread_ctx_t *lock_ctx = NULL;
    pmix_status_t rc = PMIX_SUCCESS;
    pthread_mutexattr_t attr;
    size_t size;
    uint32_t i;
    int page_size = _pmix_getpagesize();

    lock_ctx = (ds21_lock_pthread_ctx_t*)malloc(sizeof(ds21_lock_pthread_ctx_t));
    if (NULL == lock_ctx) {
        PMIX_ERROR_LOG(PMIX_ERR_INIT);
        goto error;
    }
    memset(lock_ctx, 0, sizeof(ds21_lock_pthread_ctx_t));

    lock_ctx->segment = (pmix_pshmem_seg_t *)malloc(sizeof(pmix_pshmem_seg_t));
    if (NULL == lock_ctx->segment ) {
        rc = PMIX_ERR_OUT_OF_RESOURCE;
        goto error;
    }

    /* create a lock file to prevent clients from reading while server is writing
     * to the shared memory. This situation is quite often, especially in case of
     * direct modex when clients might ask for data simultaneously. */
    if(0 > asprintf(&lock_ctx->lockfile, "%s/dstore_sm.lock", base_path)) {
        PMIX_ERROR_LOG(PMIX_ERR_OUT_OF_RESOURCE);
        goto error;
    }

    size = (2 * lock_ctx->num_locks * sizeof(pthread_mutex_t) / page_size + 1) * page_size;

    if (PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
        if (PMIX_SUCCESS != (rc = pmix_pshmem.segment_create(lock_ctx->segment),
                             lock_ctx->lockfile, size)) {
            PMIX_ERROR_LOG(rc);
            goto error;
        }
        memset(lock_ctx->segment->seg_base_addr, 0, size);
        if (0 != setuid) {
            if (0 > chown(lock_ctx->lockfile, (uid_t) uid, (gid_t) -1)){
                PMIX_ERROR_LOG(PMIX_ERROR);
                goto error;
            }
            /* set the mode as required */
            if (0 > chmod(lock_ctx->lockfile, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP )) {
                PMIX_ERROR_LOG(PMIX_ERROR);
                goto error;
            }
        }

        if (0 != pthread_mutexattr_init(&attr)) {
            PMIX_ERROR_LOG(PMIX_ERR_INIT);
            goto error;
        }
        if (0 != pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED)) {
            rc = PMIX_ERR_INIT;
            pthread_mutexattr_destroy(&attr);
            PMIX_ERROR_LOG(PMIX_ERR_INIT);
            goto error;
        }

        lock_ctx->mutex = (pthread_mutex_t *)lock_ctx->segment->seg_base_addr;
        for(i = 0; i < lock_ctx->num_locks * 2; i++) {
            if (0 != pthread_mutex_init(&lock_ctx->mutex[i], &attr)) {
                pthread_mutexattr_destroy(&attr);
                PMIX_ERROR_LOG(PMIX_ERR_INIT);
                goto error;
            }
        }
        if (0 != pthread_mutexattr_destroy(&attr)) {
            PMIX_ERROR_LOG(PMIX_ERR_INIT);
            goto error;
        }
    }
    else {
        char *str;
        if( NULL != (str = getenv(ESH_ENV_LOCK_IDX)) ) {
            lock_ctx->lock_idx = strtoul(str, NULL, 10);
        }

        lock_ctx->segment->seg_size = size;
        snprintf(lock_ctx->segment->seg_name, PMIX_PATH_MAX, "%s", lock_ctx->lockfile);
        if (PMIX_SUCCESS != (rc = pmix_pshmem.segment_attach(lock_ctx->segment, PMIX_PSHMEM_RW))) {
            PMIX_ERROR_LOG(rc);
            goto error;
        }
        lock_ctx->mutex = (pthread_mutex_t *)lock_ctx->segment->seg_base_addr;
    }

    return (pmix_common_dstor_lock_ctx_t)lock_ctx;

error:
    if (NULL != lock_ctx) {
        if (PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
            if (NULL != lock_ctx->lockfile) {
                free(lock_ctx->lockfile);
            }
            if (NULL != lock_ctx->segment) {
                pmix_pshmem.segment_detach(lock_ctx->segment);
                /* detach & unlink from current desc */
                if (lock_ctx->segment->seg_cpid == getpid()) {
                    pmix_pshmem.segment_unlink(lock_ctx->segment);
                }
                if (lock_ctx->mutex) {
                    for(i = 0; i < lock_ctx->num_locks * 2; i++) {
                        pthread_mutex_destroy(&lock_ctx->mutex[i]);
                    }
                }
                free(lock_ctx->segment);
            }
            free(lock_ctx);
            lock_ctx = NULL;
        } else {
            if (lock_ctx->lockfile) {
                free(lock_ctx->lockfile);
            }
            if (NULL != lock_ctx->segment) {
                pmix_pshmem.segment_detach(_ESH_SESSION_pthread_seg(session_idx));
                free(lock_ctx->segment);
                free(lock_ctx);
            }
        }
    }

    return NULL;
}

void pmix_ds21_lock_finalize(pmix_common_dstor_lock_ctx_t lock_ctx)
{
    pmix_status_t rc;
    uint32_t i;

    ds21_lock_pthread_ctx_t *pthread_lock =
            (ds21_lock_pthread_ctx_t*)lock_ctx;

    if (NULL == pthread_lock) {
        PMIX_ERROR_LOG(PMIX_ERR_NOT_FOUND);
        return;
    }

    if (NULL == pthread_lock->segment) {
        PMIX_ERROR_LOG(PMIX_ERROR);
        return;
    }

    pmix_pshmem.segment_detach(pthread_lock->segment);

    if(PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
        for(i = 0; i < pthread_lock->num_locks * 2; i++) {
            if (0 != pthread_mutex_destroy(&pthread_lock->mutex[i])) {
                PMIX_ERROR_LOG(PMIX_ERROR);
                return;
            }
        }
        /* detach & unlink from current desc */
        if (pthread_lock->segment->seg_cpid == getpid()) {
            pmix_pshmem.segment_unlink(pthread_lock->segment);
        }
    }
    free(pthread_lock->segment);
}

pmix_status_t pmix_ds12_lock_wr_get(pmix_common_dstor_lock_ctx_t lock_ctx)
{
    pthread_mutex_t *locks;
    uint32_t num_locks;
    uint32_t i;

    ds21_lock_pthread_ctx_t *pthread_lock = (ds21_lock_pthread_ctx_t*)lock_ctx;
    pmix_status_t rc;

    if (NULL == pthread_lock) {
        rc = PMIX_ERR_NOT_FOUND;
        PMIX_ERROR_LOG(rc);
        return rc;
    }

    locks = pthread_lock->mutex;
    num_locks = pthread_lock->num_locks;

    /* Lock the "signalling" lock first to let clients know that
     * server is going to get a write lock.
     * Clients do not hold this lock for a long time,
     * so this loop should be relatively dast.
     */
    for (i = 0; i < num_locks; i++) {
        if (0 != pthread_mutex_lock(&locks[2*i])) {
            return PMIX_ERROR;
        }
    }

    /* Now we can go and grab the main locks
     * New clients will be stopped at the previous
     * "barrier" locks.
     * We will wait here while all clients currently holding
     * locks will be done
     */
    for(i = 0; i < num_locks; i++) {
        if (0 != pthread_mutex_lock(&locks[2*i + 1])) {
            return PMIX_ERROR;
        }
    }

    return PMIX_SUCCESS;
}

pmix_status_t pmix_ds12_lock_wr_rel(pmix_common_dstor_lock_ctx_t lock_ctx)
{
    pthread_mutex_t *locks;
    uint32_t num_locks;
    uint32_t i;

    ds21_lock_pthread_ctx_t *pthread_lock = (ds21_lock_pthread_ctx_t*)lock_ctx;
    pmix_status_t rc;

    if (NULL == pthread_lock) {
        rc = PMIX_ERR_NOT_FOUND;
        PMIX_ERROR_LOG(rc);
        return rc;
    }

    locks = pthread_lock->mutex;
    num_locks = pthread_lock->num_locks;


    /* Lock the second lock first to ensure that all procs will see
     * that we are trying to grab the main one */
    for(i=0; i<num_locks; i++) {
        if (0 != pthread_mutex_unlock(&locks[2*i])) {
            return PMIX_ERROR;
        }
        if (0 != pthread_mutex_unlock(&locks[2*i + 1])) {
            return PMIX_ERROR;
        }
    }

    return PMIX_SUCCESS;
}

pmix_status_t _esh_pthread_lock_r(size_t session_idx)
{
    pthread_mutex_t *locks = _ESH_SESSION_pthread_mutex(session_idx);
    uint32_t idx = _ESH_SESSION_lockidx(session_idx);

    /* This mutex is only used to acquire the next one,
     * this is a barrier that server is using to let clients
     * know that it is going to grab the write lock
     */
    if (0 != pthread_mutex_lock(&locks[2 * idx])) {
        return PMIX_ERROR;
    }

    /* Now grab the main lock */
    if (0 != pthread_mutex_lock(&locks[2*idx + 1])) {
        return PMIX_ERROR;
    }

    /* Once done - release signalling lock */
    if (0 != pthread_mutex_unlock(&locks[2*idx])) {
        return PMIX_ERROR;
    }

    return PMIX_SUCCESS;
}

pmix_status_t _esh_pthread_unlock_r(size_t session_idx)
{
    pthread_mutex_t *locks = _ESH_SESSION_pthread_mutex(session_idx);
    uint32_t idx = _ESH_SESSION_lockidx(session_idx);

    /* Release the main lock */
    if (0 != pthread_mutex_unlock(&locks[2*idx + 1])) {
        return PMIX_SUCCESS;
    }

    return PMIX_SUCCESS;
}
