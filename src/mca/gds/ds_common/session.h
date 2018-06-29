#ifndef SESSION_H
#define SESSION_H

#include <src/include/pmix_config.h>
#include "src/mca/gds/gds.h"
#include "src/mca/pshmem/pshmem.h"

BEGIN_C_DECLS

#include <src/include/pmix_config.h>
#include "src/class/pmix_value_array.h"

/* this structs are used to store information about
 * shared segments addresses locally at each process,
 * so they are common for different types of segments
 * and don't have a specific content (namespace's info,
 * rank's meta info, ranks's data). */

typedef enum {
    INITIAL_SEGMENT,
    NS_META_SEGMENT,
    NS_DATA_SEGMENT
} segment_type;

typedef struct seg_desc_t seg_desc_t;
struct seg_desc_t {
    segment_type type;
    pmix_pshmem_seg_t seg_info;
    uint32_t id;
    seg_desc_t *next;
};

typedef struct session_s session_t;
typedef struct ns_map_data_s ns_map_data_t;
typedef struct ns_map_s ns_map_t;

struct ns_map_data_s {
    char name[PMIX_MAX_NSLEN+1];
    size_t tbl_idx;
    int track_idx;
};

struct ns_map_s {
    int in_use;
    ns_map_data_t data;
};

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

#endif // SESSION_H
