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

#include <src/include/pmix_config.h>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <pmix_common.h>
 
#include "src/include/pmix_globals.h"
#include "src/class/pmix_list.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/mca/gds/base/base.h"

#include "src/mca/gds/ds_common/dstore_lock.h"
#include "src/mca/gds/ds_common/dstore_seg.h"
#include "src/mca/gds/ds_common/dstore_session.h"
#include "src/mca/gds/ds_common/dstore_nspace.h"

#define ESH_INIT_SESSION_TBL_SIZE 2

pmix_value_array_t *_session_array = NULL;

#ifdef ESH_PTHREAD_LOCK_12
pmix_status_t _esh_session_init(size_t idx, ns_map_data_t *m, const char *base_path, size_t jobuid, int setjobuid)
#else
pmix_status_t _esh_session_init(size_t idx, ns_map_data_t *m, const char *base_path, size_t jobuid,
                                int setjobuid, uint32_t local_size)
#endif
{
    seg_desc_t *seg = NULL;
    session_t *s = &(PMIX_VALUE_ARRAY_GET_ITEM(_session_array, session_t, idx));
    pmix_status_t rc = PMIX_SUCCESS;

    s->setjobuid = setjobuid;
    s->jobuid = jobuid;
    s->nspace_path = strdup(base_path);
#ifndef ESH_PTHREAD_LOCK_12
    s->num_locks = local_size;
    s->num_forked = 0;
    s->num_procs = local_size;
#endif

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
        seg = _create_new_segment(INITIAL_SEGMENT, m->name, s->nspace_path, setjobuid, 0);
        if( NULL == seg ){
            rc = PMIX_ERR_OUT_OF_RESOURCE;
            PMIX_ERROR_LOG(rc);
            return rc;
        }
    }
    else {
        seg = _attach_new_segment(INITIAL_SEGMENT, m->name, s->nspace_path, 0);
        if( NULL == seg ){
            rc = PMIX_ERR_OUT_OF_RESOURCE;
            PMIX_ERROR_LOG(rc);
            return rc;
        }
    }
    if (PMIX_SUCCESS != (rc = _ESH_LOCK_INIT(idx))) {
        PMIX_ERROR_LOG(rc);
        return rc;
    }

    s->sm_seg_first = seg;
    s->sm_seg_last = s->sm_seg_first;
    return PMIX_SUCCESS;
}

void _esh_session_release(size_t tbl_idx)
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
    _ESH_LOCK_FINI(tbl_idx);
    memset(pmix_value_array_get_item(_session_array, tbl_idx), 0, sizeof(session_t));
}

int _esh_session_tbl_add(size_t *tbl_idx)
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

ns_map_data_t * _esh_session_map(const char *nspace, size_t tbl_idx)
{
    size_t map_idx;
    size_t size = pmix_value_array_get_size(_ns_map_array);;
    ns_map_t *ns_map = PMIX_VALUE_ARRAY_GET_BASE(_ns_map_array, ns_map_t);;
    ns_map_t *new_map = NULL;

    if (NULL == nspace) {
        PMIX_ERROR_LOG(PMIX_ERR_BAD_PARAM);
        return NULL;
    }

    for(map_idx = 0; map_idx < size; map_idx++) {
        if (!ns_map[map_idx].in_use) {
            ns_map[map_idx].in_use = true;
            strncpy(ns_map[map_idx].data.name, nspace, sizeof(ns_map[map_idx].data.name)-1);
            ns_map[map_idx].data.tbl_idx = tbl_idx;
            return  &ns_map[map_idx].data;
        }
    }

    if (NULL == (new_map = pmix_value_array_get_item(_ns_map_array, map_idx))) {
        PMIX_ERROR_LOG(PMIX_ERR_OUT_OF_RESOURCE);
        return NULL;
    }

    _esh_session_map_clean(new_map);
    new_map->in_use = true;
    new_map->data.tbl_idx = tbl_idx;
    strncpy(new_map->data.name, nspace, sizeof(new_map->data.name)-1);

    return  &new_map->data;
}

void _esh_session_map_clean(ns_map_t *m) {
    memset(m, 0, sizeof(*m));
    m->data.track_idx = -1;
}

int _esh_jobuid_tbl_search(uid_t jobuid, size_t *tbl_idx)
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

int _esh_dir_del(const char *path)
{
    DIR *dir;
    struct dirent *d_ptr;
    struct stat st;
    pmix_status_t rc = PMIX_SUCCESS;

    char name[PMIX_PATH_MAX];

    dir = opendir(path);
    if (NULL == dir) {
        rc = PMIX_ERR_BAD_PARAM;
        return rc;
    }

    while (NULL != (d_ptr = readdir(dir))) {
        snprintf(name, PMIX_PATH_MAX, "%s/%s", path, d_ptr->d_name);
        if ( 0 > lstat(name, &st) ){
            /* No fatal error here - just log this event
             * we will hit the error later at rmdir. Keep trying ...
             */
            PMIX_ERROR_LOG(PMIX_ERR_NOT_FOUND);
            continue;
        }

        if(S_ISDIR(st.st_mode)) {
            if(strcmp(d_ptr->d_name, ".") && strcmp(d_ptr->d_name, "..")) {
                rc = _esh_dir_del(name);
                if( PMIX_SUCCESS != rc ){
                    /* No fatal error here - just log this event
                     * we will hit the error later at rmdir. Keep trying ...
                     */
                    PMIX_ERROR_LOG(rc);
                }
            }
        }
        else {
            if( 0 > unlink(name) ){
                /* No fatal error here - just log this event
                 * we will hit the error later at rmdir. Keep trying ...
                 */
                PMIX_ERROR_LOG(PMIX_ERR_NO_PERMISSIONS);
            }
        }
    }
    closedir(dir);

    /* remove the top dir */
    if( 0 > rmdir(path) ){
        rc = PMIX_ERR_NO_PERMISSIONS;
        PMIX_ERROR_LOG(rc);
    }
    return rc;
}
