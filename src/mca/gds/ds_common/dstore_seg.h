#ifndef DSTORE_SEG_H
#define DSTORE_SEG_H

#include <src/include/pmix_config.h>
#include "src/mca/pshmem/pshmem.h"
#include "dstore_nspace.h"

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

/* initial segment format:
 * size_t num_elems;
 * size_t full; //indicate to client that it needs to attach to the next segment
 * ns_seg_info_t ns_seg_info[max_ns_num];
 */

typedef struct {
    ns_map_data_t ns_map;
    size_t num_meta_seg;/* read by clients to attach to this number of segments. */
    size_t num_data_seg;
} ns_seg_info_t;

/* meta segment format:
 * size_t num_elems;
 * rank_meta_info meta_info[max_meta_elems];
 */

typedef struct {
    size_t rank;
    size_t offset;
    size_t count;
} rank_meta_info;

/*
extern size_t _initial_segment_size;
extern size_t _max_ns_num;
extern size_t _meta_segment_size;
extern size_t _max_meta_elems;
extern size_t _data_segment_size;
extern size_t _lock_segment_size;
extern int _direct_mode;
*/

int _pmix_getpagesize(void);
void _set_constants_from_env(void);
seg_desc_t *_create_new_segment(segment_type type, const char *name,
                                const char *path, char setjobuid, uint32_t id);
seg_desc_t *_attach_new_segment(segment_type type, const char *name,
                                const char *path, uint32_t id);
int _put_ns_info_to_initial_segment(seg_desc_t *seg_desc_last, const char *name,
                                    const char *path, size_t tbl_idx, char setjobuid);
void _update_initial_segment_info(seg_desc_t *seg_desc_first, const char *name,
                                  const char *path);
ns_seg_info_t *_get_ns_info_from_initial_segment(seg_desc_t *seg_desc_first,
                                                 const char *name);
seg_desc_t *extend_segment(seg_desc_t *segdesc, const char *name,
                           const char *path, char setjobuid);

#endif // DSTORE_SEG_H
