#include <src/include/pmix_config.h>
#include <pmix_common.h>

#include "src/include/pmix_globals.h"
#include "src/class/pmix_list.h"
#include "src/util/error.h"

#include "session.h"

static pmix_value_array_t *_session_array = NULL;
static pmix_value_array_t *_ns_map_array = NULL;
static pmix_value_array_t *_ns_track_array = NULL;

static inline int _esh_tbls_init(void)
{
    pmix_status_t rc = PMIX_SUCCESS;
    size_t idx;

    /* initial settings */
    _ns_track_array = NULL;
    _session_array = NULL;
    _ns_map_array = NULL;

    /* Setup namespace tracking array */
    if (NULL == (_ns_track_array = PMIX_NEW(pmix_value_array_t))) {
        rc = PMIX_ERR_OUT_OF_RESOURCE;
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }
    if (PMIX_SUCCESS != (rc = pmix_value_array_init(_ns_track_array, sizeof(ns_track_elem_t)))){
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }

    /* Setup sessions table */
    if (NULL == (_session_array = PMIX_NEW(pmix_value_array_t))){
        rc = PMIX_ERR_OUT_OF_RESOURCE;
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }
    if (PMIX_SUCCESS != (rc = pmix_value_array_init(_session_array, sizeof(session_t)))) {
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }
    if (PMIX_SUCCESS != (rc = pmix_value_array_set_size(_session_array, ESH_INIT_SESSION_TBL_SIZE))) {
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }
    for (idx = 0; idx < ESH_INIT_SESSION_TBL_SIZE; idx++) {
        memset(pmix_value_array_get_item(_session_array, idx), 0, sizeof(session_t));
    }

    /* Setup namespace map array */
    if (NULL == (_ns_map_array = PMIX_NEW(pmix_value_array_t))) {
        rc = PMIX_ERR_OUT_OF_RESOURCE;
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }
    if (PMIX_SUCCESS != (rc = pmix_value_array_init(_ns_map_array, sizeof(ns_map_t)))) {
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }
    if (PMIX_SUCCESS != (rc = pmix_value_array_set_size(_ns_map_array, ESH_INIT_NS_MAP_TBL_SIZE))) {
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }
    for (idx = 0; idx < ESH_INIT_NS_MAP_TBL_SIZE; idx++) {
        _esh_session_map_clean(pmix_value_array_get_item(_ns_map_array, idx));
    }

    return PMIX_SUCCESS;
err_exit:
    if (NULL != _ns_track_array) {
        PMIX_RELEASE(_ns_track_array);
    }
    if (NULL != _session_array) {
        PMIX_RELEASE(_session_array);
    }
    if (NULL != _ns_map_array) {
        PMIX_RELEASE(_ns_map_array);
    }
    return rc;
}

static inline void _esh_sessions_cleanup(void)
{
    size_t idx;
    size_t size;
    session_t *s_tbl;

    if (NULL == _session_array) {
        return;
    }

    size = pmix_value_array_get_size(_session_array);
    s_tbl = PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t);

    for (idx = 0; idx < size; idx++) {
        if(s_tbl[idx].in_use)
            _esh_session_release(idx);
    }

    PMIX_RELEASE(_session_array);
    _session_array = NULL;
}

static inline void _esh_ns_map_cleanup(void)
{
    size_t idx;
    size_t size;
    ns_map_t *ns_map;

    if (NULL == _ns_map_array) {
        return;
    }

    size = pmix_value_array_get_size(_ns_map_array);
    ns_map = PMIX_VALUE_ARRAY_GET_BASE(_ns_map_array, ns_map_t);

    for (idx = 0; idx < size; idx++) {
        if(ns_map[idx].in_use)
            _esh_session_map_clean(&ns_map[idx]);
    }

    PMIX_RELEASE(_ns_map_array);
    _ns_map_array = NULL;
}

static inline void _esh_ns_track_cleanup(void)
{
    int size;
    ns_track_elem_t *ns_trk;

    if (NULL == _ns_track_array) {
        return;
    }

    size = pmix_value_array_get_size(_ns_track_array);
    ns_trk = PMIX_VALUE_ARRAY_GET_BASE(_ns_track_array, ns_track_elem_t);

    for (int i = 0; i < size; i++) {
        ns_track_elem_t *trk = ns_trk + i;
        if (trk->in_use) {
            PMIX_DESTRUCT(trk);
        }
    }

    PMIX_RELEASE(_ns_track_array);
    _ns_track_array = NULL;
}

static inline int _esh_jobuid_tbl_search(uid_t jobuid, size_t *tbl_idx)
{
    size_t idx, size;
    session_t *session_tbl = NULL;

    size = pmix_value_array_get_size(_session_array);
    session_tbl = PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t);

    for(idx = 0; idx < size; idx++) {
        if (session_tbl[idx].in_use && session_tbl[idx].jobuid == jobuid) {
            *tbl_idx = idx;
            return PMIX_SUCCESS;
        }
    }

    return PMIX_ERR_NOT_FOUND;
}

static inline int _esh_session_tbl_add(size_t *tbl_idx)
{
    size_t idx;
    size_t size = pmix_value_array_get_size(_session_array);
    session_t *s_tbl = PMIX_VALUE_ARRAY_GET_BASE(_session_array, session_t);
    session_t *new_sesion;
    pmix_status_t rc = PMIX_SUCCESS;

    for(idx = 0; idx < size; idx ++) {
        if (0 == s_tbl[idx].in_use) {
            s_tbl[idx].in_use = 1;
            *tbl_idx = idx;
            return PMIX_SUCCESS;
        }
    }

    if (NULL == (new_sesion = pmix_value_array_get_item(_session_array, idx))) {
        rc = PMIX_ERR_OUT_OF_RESOURCE;
        PMIX_ERROR_LOG(rc);
        return rc;
    }
    s_tbl[idx].in_use = 1;
    *tbl_idx = idx;

    return PMIX_SUCCESS;
}

static inline ns_map_data_t * _esh_session_map_search_server(const char *nspace)
{
    size_t idx, size = pmix_value_array_get_size(_ns_map_array);
    ns_map_t *ns_map = PMIX_VALUE_ARRAY_GET_BASE(_ns_map_array, ns_map_t);
    if (NULL == nspace) {
        return NULL;
    }

    for (idx = 0; idx < size; idx++) {
        if (ns_map[idx].in_use &&
            (0 == strcmp(ns_map[idx].data.name, nspace))) {
                return &ns_map[idx].data;
        }
    }
    return NULL;
}

static inline ns_map_data_t * _esh_session_map_search_client(const char *nspace)
{
    size_t idx, size = pmix_value_array_get_size(_ns_map_array);
    ns_map_t *ns_map = PMIX_VALUE_ARRAY_GET_BASE(_ns_map_array, ns_map_t);

    if (NULL == nspace) {
        return NULL;
    }

    for (idx = 0; idx < size; idx++) {
        if (ns_map[idx].in_use &&
            (0 == strcmp(ns_map[idx].data.name, nspace))) {
                return &ns_map[idx].data;
        }
    }
    return _esh_session_map(nspace, 0);
}

static inline int _esh_session_init(size_t idx, ns_map_data_t *m, size_t jobuid, int setjobuid)
{
    seg_desc_t *seg = NULL;
    session_t *s = &(PMIX_VALUE_ARRAY_GET_ITEM(_session_array, session_t, idx));
    pmix_status_t rc = PMIX_SUCCESS;

    s->setjobuid = setjobuid;
    s->jobuid = jobuid;
    s->nspace_path = strdup(_base_path);

    /* create a lock file to prevent clients from reading while server is writing to the shared memory.
    * This situation is quite often, especially in case of direct modex when clients might ask for data
    * simultaneously.*/
    if(0 > asprintf(&s->lockfile, "%s/dstore_sm.lock", s->nspace_path)) {
        rc = PMIX_ERR_OUT_OF_RESOURCE;
        PMIX_ERROR_LOG(rc);
        return rc;
    }
    PMIX_OUTPUT_VERBOSE((10, pmix_gds_base_framework.framework_output,
        "%s:%d:%s _lockfile_name: %s", __FILE__, __LINE__, __func__, s->lockfile));

    if (PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
        if (0 != mkdir(s->nspace_path, 0770)) {
            if (EEXIST != errno) {
                pmix_output(0, "session init: can not create session directory \"%s\": %s",
                    s->nspace_path, strerror(errno));
                rc = PMIX_ERROR;
                PMIX_ERROR_LOG(rc);
                return rc;
            }
        }
        if (s->setjobuid > 0){
            if (0 > chown(s->nspace_path, (uid_t) s->jobuid, (gid_t) -1)){
                rc = PMIX_ERROR;
                PMIX_ERROR_LOG(rc);
                return rc;
            }
        }
        seg = _create_new_segment(INITIAL_SEGMENT, m, 0);
        if( NULL == seg ){
            rc = PMIX_ERR_OUT_OF_RESOURCE;
            PMIX_ERROR_LOG(rc);
            return rc;
        }
    }
    else {
        seg = _attach_new_segment(INITIAL_SEGMENT, m, 0);
        if( NULL == seg ){
            rc = PMIX_ERR_OUT_OF_RESOURCE;
            PMIX_ERROR_LOG(rc);
            return rc;
        }
    }

    if (NULL == _esh_lock_init) {
        rc = PMIX_ERR_INIT;
        PMIX_ERROR_LOG(rc);
        return rc;
    }
    if ( PMIX_SUCCESS != (rc = ds_lock.init(m->tbl_idx))) {
        PMIX_ERROR_LOG(rc);
        return rc;
    }

    s->sm_seg_first = seg;
    s->sm_seg_last = s->sm_seg_first;
    return PMIX_SUCCESS;
}

static inline void _esh_session_release(size_t tbl_idx)
{
    if (!_ESH_SESSION_in_use(tbl_idx)) {
        return;
    }

    _delete_sm_desc(_ESH_SESSION_sm_seg_first(tbl_idx));
    /* if the lock fd was somehow set, then we
     * need to close it */
    if (0 != _ESH_SESSION_lockfd(tbl_idx)) {
        close(_ESH_SESSION_lockfd(tbl_idx));
    }

    if (NULL != _ESH_SESSION_lockfile(tbl_idx)) {
        if(PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
            unlink(_ESH_SESSION_lockfile(tbl_idx));
        }
        free(_ESH_SESSION_lockfile(tbl_idx));
    }
    if (NULL != _ESH_SESSION_path(tbl_idx)) {
        if(PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
            _esh_dir_del(_ESH_SESSION_path(tbl_idx));
        }
        free(_ESH_SESSION_path(tbl_idx));
    }
#ifdef ESH_PTHREAD_LOCK
    ds_lock.fini(tbl_idx);
#endif
    memset(pmix_value_array_get_item(_session_array, tbl_idx), 0, sizeof(session_t));
}

