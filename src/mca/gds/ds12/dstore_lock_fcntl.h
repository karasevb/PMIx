#ifndef DSTORE_LOCK_FCNTL_H
#define DSTORE_LOCK_FCNTL_H

#include <fcntl.h>

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "dstore_lock.h"


#define _ESH_12_FCNTL_LOCK(lockfd, operation)                        \
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

#define _DS_WRLOCK(lock) _ESH_12_FCNTL_LOCK(lock, F_WRLCK)
#define _DS_RDLOCK(lock) _ESH_12_FCNTL_LOCK(lock, F_RDLCK)
#define _DS_UNLOCK(lock) _ESH_12_FCNTL_LOCK(lock, F_UNLCK)

pmix_status_t pmix_ds12_lock_init(size_t session_idx);
void pmix_ds12_lock_finalize(size_t session_idx);

#endif // DSTORE_LOCK_FCNTL_H
