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
#include "dstore_lock_pthread.h"
#endif
#ifdef ESH_FCNTL_LOCK
#include "dstore_lock_fcntl.h"
#endif
#include "dstore_session.h"

#define _ESH_LOCK_INIT(idx) pmix_ds12_lock_init(idx)
#define _ESH_LOCK_FINI(idx) pmix_ds12_lock_finalize(idx)
#define _ESH_WR_LOCK(idx)   _DS_WRLOCK(_ESH_SESSION_lock(idx))
#define _ESH_RD_LOCK(idx)   _DS_RDLOCK(_ESH_SESSION_lock(idx))
#define _ESH_WR_UNLOCK(idx) _DS_UNLOCK(_ESH_SESSION_lock(idx))
#define _ESH_RD_UNLOCK(idx) _DS_UNLOCK(_ESH_SESSION_lock(idx))

#endif // DSTORE_LOCK_H
