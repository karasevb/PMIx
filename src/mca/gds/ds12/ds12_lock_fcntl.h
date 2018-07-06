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

#ifndef DS12_LOCK_FCNTL_H
#define DS12_LOCK_FCNTL_H

#include <fcntl.h>

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "src/mca/gds/ds_common/dstore_lock.h"
#include "src/mca/gds/ds_common/dstore_session.h"

#define _ESH_12_FCNTL_LOCK(lockfd, operation)               \
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

#define _DS_INIT(idx) pmix_ds12_lock_init(idx)
#define _DS_FINI(idx) pmix_ds12_lock_finalize(idx)
#define _DS_WR_LOCK(idx) _ESH_12_FCNTL_LOCK(_ESH_SESSION_lock(idx), F_WRLCK)
#define _DS_RD_LOCK(idx) _ESH_12_FCNTL_LOCK(_ESH_SESSION_lock(idx), F_RDLCK)
#define _DS_WR_UNLOCK(idx) _ESH_12_FCNTL_LOCK(_ESH_SESSION_lock(idx), F_UNLCK)
#define _DS_RD_UNLOCK(idx) _ESH_12_FCNTL_LOCK(_ESH_SESSION_lock(idx), F_UNLCK)

pmix_status_t pmix_ds12_lock_init(size_t session_idx);
void pmix_ds12_lock_finalize(size_t session_idx);

#endif // DSTORE_LOCK_FCNTL_H
