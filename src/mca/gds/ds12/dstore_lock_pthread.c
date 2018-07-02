#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "dstore_seg.h"
#include "dstore_lock_pthread.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/mca/pshmem/pshmem.h"
#include "src/include/pmix_globals.h"

#define _ESH_LOCK(rwlock, func)                             \
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

#define _ESH_WRLOCK(rwlock) _ESH_LOCK(rwlock, wrlock)
#define _ESH_RDLOCK(rwlock) _ESH_LOCK(rwlock, rdlock)
#define _ESH_UNLOCK(rwlock) _ESH_LOCK(rwlock, unlock)

int pmix_ds_lock_init(pmix_pshmem_seg_t **rwlock_seg, pthread_rwlock_t **rwlock, char *lockfile,
                      char setjobuid, uid_t jobuid) {
    pmix_status_t rc = PMIX_SUCCESS;
    size_t size = _lock_segment_size;
    pthread_rwlockattr_t attr;

    if ((NULL != *rwlock_seg) || (NULL != *rwlock)) {
        rc = PMIX_ERR_INIT;
        return rc;
    }
    *rwlock_seg = (pmix_pshmem_seg_t *)malloc(sizeof(pmix_pshmem_seg_t));
    if (NULL == *rwlock_seg) {
        rc = PMIX_ERR_OUT_OF_RESOURCE;
        return rc;
    }
    if (PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
        if (PMIX_SUCCESS != (rc = pmix_pshmem.segment_create(*rwlock_seg, lockfile, size))) {
            return rc;
        }
        memset((*rwlock_seg)->seg_base_addr, 0, size);
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
        *rwlock = (pthread_rwlock_t *)(*rwlock_seg)->seg_base_addr;

        if (0 != pthread_rwlockattr_init(&attr)) {
            rc = PMIX_ERR_INIT;
            pmix_pshmem.segment_detach(*rwlock_seg);
            return rc;
        }
        if (0 != pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED)) {
            rc = PMIX_ERR_INIT;
            pmix_pshmem.segment_detach(*rwlock_seg);
            pthread_rwlockattr_destroy(&attr);
            return rc;
        }
#ifdef HAVE_PTHREAD_SETKIND
        if (0 != pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP)) {
            rc = PMIX_ERR_INIT;
            pmix_pshmem.segment_detach(*rwlock_seg);
            pthread_rwlockattr_destroy(&attr);
            return rc;
        }
#endif
        if (0 != pthread_rwlock_init(*rwlock, &attr)) {
            rc = PMIX_ERR_INIT;
            pmix_pshmem.segment_detach(*rwlock_seg);
            pthread_rwlockattr_destroy(&attr);
            return rc;
        }
        if (0 != pthread_rwlockattr_destroy(&attr)) {
            rc = PMIX_ERR_INIT;
            return rc;
        }

    }
    else {
        (*rwlock_seg)->seg_size = size;
        snprintf((*rwlock_seg)->seg_name, PMIX_PATH_MAX, "%s", lockfile);
        if (PMIX_SUCCESS != (rc = pmix_pshmem.segment_attach(*rwlock_seg, PMIX_PSHMEM_RW))) {
            return rc;
        }
        *rwlock = (pthread_rwlock_t *)(*rwlock_seg)->seg_base_addr;
    }

    return rc;
}

void pmix_ds_lock_fini(pmix_pshmem_seg_t **rwlock_seg, pthread_rwlock_t **rwlock) {
    pmix_status_t rc;


    if (0 != pthread_rwlock_destroy(*rwlock)) {
        rc = PMIX_ERROR;
        PMIX_ERROR_LOG(rc);
        return;
    }

    /* detach & unlink from current desc */
    if ((*rwlock_seg)->seg_cpid == getpid()) {
        pmix_pshmem.segment_unlink(*rwlock_seg);
    }
    pmix_pshmem.segment_detach(*rwlock_seg);

    free(*rwlock_seg);
    *rwlock_seg = NULL;
    *rwlock = NULL;
}

pmix_status_t pmix_ds_lock_wr_acq(pthread_rwlock_t *rwlock) {
    return _ESH_LOCK(rwlock, wrlock);
}

pmix_status_t pmix_ds_lock_rd_acq(pthread_rwlock_t *rwlock) {
    return _ESH_LOCK(rwlock, rdlock);
}

pmix_status_t pmix_ds_lock_rw_rel(pthread_rwlock_t *rwlock) {
    return _ESH_LOCK(rwlock, unlock);
}

pmix_ds_lock_module_t ds_lock = {
    .name = "pthread",
    .init = pmix_ds_lock_init,
    .fini = pmix_ds_lock_fini,
    .wr_lock = pmix_ds_lock_wr_acq,
    .wr_unlock = pmix_ds_lock_rw_rel,
    .rd_lock = pmix_ds_lock_rd_acq,
    .rd_unlock = pmix_ds_lock_rw_rel
};
