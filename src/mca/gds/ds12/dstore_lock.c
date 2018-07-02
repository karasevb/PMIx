#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "src/include/pmix_globals.h"
#include "dstore_lock.h"

#ifdef ESH_PTHREAD_LOCK
#include "dstore_lock_pthread.h"
#endif
#ifdef ESH_FCNTL_LOCK
#include "dstore_lock_fcntl.h"
#endif

pmix_status_t dstore_lock_init(size_t idx)
{
#ifdef ESH_PTHREAD_LOCK
    ds_lock.init();
#endif
#ifdef ESH_FCNTL_LOCK
#endif

}
