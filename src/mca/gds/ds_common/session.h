#ifndef SESSION_H
#define SESSION_H

#include <src/include/pmix_config.h>
#include "src/mca/gds/gds.h"
#include "src/mca/pshmem/pshmem.h"

BEGIN_C_DECLS

#include <src/include/pmix_config.h>
#include "src/class/pmix_value_array.h"

typedef struct session_s session_t;

struct session_s {
    int in_use;
    uid_t jobuid;
    char setjobuid;
    char *nspace_path;
    char *lockfile;
#ifdef ESH_PTHREAD_LOCK
    pmix_pshmem_seg_t *rwlock_seg;
    pthread_rwlock_t *rwlock;
#endif
    int lockfd;
    seg_desc_t *sm_seg_first;
    seg_desc_t *sm_seg_last;
};

#endif // SESSION_H
