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

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "src/util/error.h"
#include "src/util/output.h"
#include "src/include/pmix_globals.h"
#include "ds12_lock_fcntl.h"

pmix_status_t pmix_ds12_lock_init(size_t session_idx)
{
    char setjobuid = _ESH_SESSION_setjobuid(session_idx);
    uid_t jobuid = _ESH_SESSION_jobuid(session_idx);
    const char *lockfile = _ESH_SESSION_lockfile(session_idx);
    int lockfd;

    pmix_status_t rc = PMIX_SUCCESS;

    if (PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
        lockfd = open(lockfile, O_CREAT | O_RDWR | O_EXCL, 0600);

        /* if previous launch was crashed, the lockfile might not be deleted and unlocked,
         * so we delete it and create a new one. */
        if (lockfd < 0) {
            unlink(lockfile);
            lockfd = open(lockfile, O_CREAT | O_RDWR, 0600);
            if (lockfd < 0) {
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
        lockfd = open(lockfile, O_RDONLY);
        if (-1 == lockfd) {
            rc = PMIX_ERROR;
            PMIX_ERROR_LOG(rc);
            return rc;
        }
    }
    _ESH_SESSION_lockfd(session_idx) = lockfd;
    return rc;
}

void pmix_ds12_lock_finalize(size_t session_idx)
{
    close(_ESH_SESSION_lockfd(session_idx));

    if (PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
        unlink(_ESH_SESSION_lockfile(session_idx));
    }
}
