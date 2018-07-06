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

#ifndef DSTORE_LOCK_H
#define DSTORE_LOCK_H

#include <src/include/pmix_config.h>
#include <pmix_common.h>

#ifdef HAVE_PTHREAD_SHARED
#define ESH_PTHREAD_LOCK
#elif defined HAVE_FCNTL_FLOCK
#define ESH_FCNTL_LOCK
#else
#error No locking mechanism was found
#endif

#ifdef ESH_PTHREAD_LOCK
#ifdef ESH_PTHREAD_LOCK_21
#include "ds21_lock_pthread.h"
#else
#include "ds12_lock_pthread.h"
#endif
#endif
#ifdef ESH_FCNTL_LOCK
#include "ds12_lock_fcntl.h"
#endif

#define _ESH_LOCK_INIT(idx) _DS_INIT(idx)
#define _ESH_LOCK_FINI(idx) _DS_INIT(idx)
#define _ESH_WR_LOCK(idx)   _DS_WR_LOCK(idx)
#define _ESH_RD_LOCK(idx)   _DS_RD_LOCK(idx)
#define _ESH_WR_UNLOCK(idx) _DS_WR_UNLOCK(idx)
#define _ESH_RD_UNLOCK(idx) _DS_RD_UNLOCK(idx)

#endif // DSTORE_LOCK_H
