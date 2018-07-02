/*
 * Copyright (c) 2015-2017 Intel, Inc. All rights reserved.
 * Copyright (c) 2017      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef PMIX_DS12_H
#define PMIX_DS12_H

#include <src/include/pmix_config.h>
#include "dstore_seg.h"


#include "src/mca/gds/gds.h"
#include "src/mca/pshmem/pshmem.h"

BEGIN_C_DECLS

#include <src/include/pmix_config.h>
#include "src/class/pmix_value_array.h"

#define INITIAL_SEG_SIZE 4096
#define NS_META_SEG_SIZE (1<<22)
#define NS_DATA_SEG_SIZE (1<<22)

#define PMIX_DSTORE_ESH_BASE_PATH "PMIX_DSTORE_ESH_BASE_PATH"

#ifdef HAVE_PTHREAD_SHARED
#define ESH_PTHREAD_LOCK
#elif defined HAVE_FCNTL_FLOCK
#define ESH_FCNTL_LOCK
#else
#error No locking mechanism was found
#endif

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

typedef struct {
    pmix_value_array_t super;
    ns_map_data_t ns_map;
    size_t num_meta_seg;
    size_t num_data_seg;
    seg_desc_t *meta_seg;
    seg_desc_t *data_seg;
    bool in_use;
} ns_track_elem_t;

/* the component must be visible data for the linker to find it */
PMIX_EXPORT extern pmix_gds_base_component_t mca_gds_ds12_component;
extern pmix_gds_base_module_t pmix_ds12_module;

END_C_DECLS

#endif
