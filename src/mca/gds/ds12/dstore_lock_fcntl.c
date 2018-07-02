#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "src/util/error.h"
#include "src/util/output.h"
#include "src/include/pmix_globals.h"

#define _ESH_LOCK(lockfd, operation)                        \
__pmix_attribute_extension__ ({                             \
    pmix_status_t ret = PMIX_SUCCESS;                       \
    int i;                                                  \
    struct flock fl = {0};                                  \
    fl.l_type = operation;                                  \
    fl.l_whence = SEEK_SET;                                 \
    for(i = 0; i < 10; i++) {                               \
        if( 0 > fcntl(lockfd, F_SETLKW, &fl) ) {            \
            switch( errno ){                                \
                case EINTR:                                 \
                    continue;                               \
                case ENOENT:                                \
                case EINVAL:                                \
                    ret = PMIX_ERR_NOT_FOUND;               \
                    break;                                  \
                case EBADF:                                 \
                    ret = PMIX_ERR_BAD_PARAM;               \
                    break;                                  \
                case EDEADLK:                               \
                case EFAULT:                                \
                case ENOLCK:                                \
                    ret = PMIX_ERR_RESOURCE_BUSY;           \
                    break;                                  \
                default:                                    \
                    ret = PMIX_ERROR;                       \
                    break;                                  \
            }                                               \
        }                                                   \
        break;                                              \
    }                                                       \
    if (ret) {                                              \
        pmix_output(0, "%s %d:%s lock failed: %s",          \
            __FILE__, __LINE__, __func__, strerror(errno)); \
    }                                                       \
    ret;                                                    \
})

#define _ESH_WRLOCK(lock) _ESH_LOCK(lock, F_WRLCK)
#define _ESH_RDLOCK(lock) _ESH_LOCK(lock, F_RDLCK)
#define _ESH_UNLOCK(lock) _ESH_LOCK(lock, F_UNLCK)

int pmix_ds_lock_init(const char *lockfile, int *lockfd, char setjobuid, uid_t jobuid) {
    pmix_status_t rc = PMIX_SUCCESS;

    if (PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
        *lockfd = open(lockfile, O_CREAT | O_RDWR | O_EXCL, 0600);

        /* if previous launch was crashed, the lockfile might not be deleted and unlocked,
         * so we delete it and create a new one. */
        if (*lockfd < 0) {
            unlink(lockfile);
            *lock = open(lockfile, O_CREAT | O_RDWR, 0600);
            if (*lock < 0) {
                rc = PMIX_ERROR;
                PMIX_ERROR_LOG(rc);
                return rc;
            }
        }
        if (setjobuid > 0) {
            if (0 > chown(lockfile, jobuid, (gid_t) -1)) {
                rc = PMIX_ERROR;
                PMIX_ERROR_LOG(rc);
                return rc;
            }
            if (0 > chmod(lockfile, S_IRUSR | S_IWGRP | S_IRGRP)) {
                rc = PMIX_ERROR;
                PMIX_ERROR_LOG(rc);
                return rc;
            }
        }
    }
    else {
        *lockfd= open(lockfile, O_RDONLY);
        if (-1 == *lockfd) {
            rc = PMIX_ERROR;
            PMIX_ERROR_LOG(rc);
            return rc;
        }
    }
    return rc;
}

pmix_status_t pmix_ds_lock_wr_acq(int lockfd)
{
    return _ESH_WRLOCK(lockfd);
}

pmix_status_t pmix_ds_lock_rd_acq(int lockfd)
{
    return _ESH_RDLOCK(lockfd);

}

pmix_status_t pmix_ds_lock_rw_rel(int lockfd)
{
    return _ESH_UNLOCK(lockfd);
}

pmix_ds_lock_module_t ds_lock = {
    .name = "fcntl",
    .init = pmix_ds_lock_init,
    .fini = NULL,
    .wr_lock = pmix_ds_lock_wr_acq,
    .wr_unlock = pmix_ds_lock_rw_rel,
    .rd_lock = pmix_ds_lock_rd_acq,
    .rd_unlock = pmix_ds_lock_rw_rel
};

