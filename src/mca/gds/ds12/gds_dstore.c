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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <pmix_common.h>
#include "src/include/pmix_globals.h"
#include "src/class/pmix_list.h"
#include "src/client/pmix_client_ops.h"
#include "src/server/pmix_server_ops.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/util/argv.h"

#include "src/mca/gds/base/base.h"
#include "src/mca/pshmem/base/base.h"

#include "gds_dstore.h"
//#include "src/mca/gds/ds_common/dstore_lock.h"
#include "src/mca/gds/ds_common/gds_dstore.h"
#include "src/mca/gds/ds_common/dstore_seg.h"
#include "src/mca/gds/ds_common/dstore_session.h"

//#include "src/mca/common/pmix_common_dstore.h"
//#include "ds12_lock.h"

//static pmix_common_lock_callbacks_t *ds12_lock_cb = &pmix_ds12_lock_module;
static pmix_common_dstor_lock_ctx_t ds12_lock_ctx;
static pmix_ds12_ctx_t *ds12_ctx;

void _set_constants_from_env(void)
{
    char *str;
    int page_size = _pmix_getpagesize();

    if( NULL != (str = getenv(ESH_ENV_INITIAL_SEG_SIZE)) ) {
        _initial_segment_size = strtoul(str, NULL, 10);
        if ((size_t)page_size > _initial_segment_size) {
            _initial_segment_size = (size_t)page_size;
        }
    }
    if (0 == _initial_segment_size) {
        _initial_segment_size = INITIAL_SEG_SIZE;
    }
    if( NULL != (str = getenv(ESH_ENV_NS_META_SEG_SIZE)) ) {
        _meta_segment_size = strtoul(str, NULL, 10);
        if ((size_t)page_size > _meta_segment_size) {
            _meta_segment_size = (size_t)page_size;
        }
    }
    if (0 == _meta_segment_size) {
        _meta_segment_size = NS_META_SEG_SIZE;
    }
    if( NULL != (str = getenv(ESH_ENV_NS_DATA_SEG_SIZE)) ) {
        _data_segment_size = strtoul(str, NULL, 10);
        if ((size_t)page_size > _data_segment_size) {
            _data_segment_size = (size_t)page_size;
        }
    }
    if (0 == _data_segment_size) {
        _data_segment_size = NS_DATA_SEG_SIZE;
    }
    if (NULL != (str = getenv(ESH_ENV_LINEAR))) {
        if (1 == strtoul(str, NULL, 10)) {
            _direct_mode = 1;
        }
    }

    _lock_segment_size = page_size;
    _max_ns_num = (_initial_segment_size - sizeof(size_t) * 2) / sizeof(ns_seg_info_t);
    _max_meta_elems = (_meta_segment_size - sizeof(size_t)) / sizeof(rank_meta_info);
}

static pmix_status_t ds12_init(pmix_info_t info[], size_t ninfo)
{
    pmix_status_t rc;
    size_t n;
    char *dstor_tmpdir = NULL;
    size_t tbl_idx=0;
    ns_map_data_t *ns_map = NULL;

    pmix_output_verbose(2, pmix_gds_base_framework.framework_output,
                        "pmix:gds:dstore init");

    //ds12_lock_ctx = pmix_ds12_lock_module.init();

    //ds12_ctx = pmix_common_dstor_init(&pmix_ds12_lock_module,
    //                                  path, "ds12", info, ninfo);

    /* open the pshmem and select the active plugins */
    if( PMIX_SUCCESS != (rc = pmix_mca_base_framework_open(&pmix_pshmem_base_framework, 0)) ) {
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }
    if( PMIX_SUCCESS != (rc = pmix_pshmem_base_select()) ) {
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }

    _jobuid = getuid();
    _setjobuid = 0;

    if (PMIX_SUCCESS != (rc = _esh_tbls_init())) {
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }

    rc = pmix_pshmem.init();
    if (PMIX_SUCCESS != rc) {
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }

    _set_constants_from_env();

    if (NULL != _base_path) {
        free(_base_path);
        _base_path = NULL;
    }

    /* find the temp dir */
    if (PMIX_PROC_IS_SERVER(pmix_globals.mypeer)) {
        _esh_session_map_search = _esh_session_map_search_server;

        /* scan incoming info for directives */
        if (NULL != info) {
            for (n=0; n < ninfo; n++) {
                if (0 == strcmp(PMIX_USERID, info[n].key)) {
                    _jobuid = info[n].value.data.uint32;
                    _setjobuid = 1;
                    continue;
                }
                if (0 == strcmp(PMIX_DSTPATH, info[n].key)) {
                    /* PMIX_DSTPATH is the way for RM to customize the
                     * place where shared memory files are placed.
                     * We need this for the following reasons:
                     * - disk usage: files can be relatively large and the system may
                     *   have a small common temp directory.
                     * - performance: system may have a fast IO device (i.e. burst buffer)
                     *   for the local usage.
                     *
                     * PMIX_DSTPATH has higher priority than PMIX_SERVER_TMPDIR
                     */
                    if( PMIX_STRING != info[n].value.type ){
                        rc = PMIX_ERR_BAD_PARAM;
                        PMIX_ERROR_LOG(rc);
                        goto err_exit;
                    }
                    dstor_tmpdir = (char*)info[n].value.data.string;
                    continue;
                }
                if (0 == strcmp(PMIX_SERVER_TMPDIR, info[n].key)) {
                    if( PMIX_STRING != info[n].value.type ){
                        rc = PMIX_ERR_BAD_PARAM;
                        PMIX_ERROR_LOG(rc);
                        goto err_exit;
                    }
                    if (NULL == dstor_tmpdir) {
                        dstor_tmpdir = (char*)info[n].value.data.string;
                    }
                    continue;
                }
            }
        }

        if (NULL == dstor_tmpdir) {
            if (NULL == (dstor_tmpdir = getenv("TMPDIR"))) {
                if (NULL == (dstor_tmpdir = getenv("TEMP"))) {
                    if (NULL == (dstor_tmpdir = getenv("TMP"))) {
                        dstor_tmpdir = "/tmp";
                    }
                }
            }
        }

        pmix_common_dstor_init("ds12");

        rc = asprintf(&_base_path, "%s/pmix_dstor_%s_%d", dstor_tmpdir, GDS_DS_NAME_STR, getpid());
        if ((0 > rc) || (NULL == _base_path)) {
            rc = PMIX_ERR_OUT_OF_RESOURCE;
            PMIX_ERROR_LOG(rc);
            goto err_exit;
        }

        if (0 != mkdir(_base_path, 0770)) {
            if (EEXIST != errno) {
                rc = PMIX_ERROR;
                PMIX_ERROR_LOG(rc);
                goto err_exit;
            }
        }
        if (_setjobuid > 0) {
            if (chown(_base_path, (uid_t) _jobuid, (gid_t) -1) < 0){
                rc = PMIX_ERR_NO_PERMISSIONS;
                PMIX_ERROR_LOG(rc);
                goto err_exit;
            }
        }
        _esh_session_map_search = _esh_session_map_search_server;
        return PMIX_SUCCESS;
    }
    /* for clients */
    else {
        if (NULL == (dstor_tmpdir = getenv(PMIX_DSTORE_ESH_BASE_PATH))){
            return PMIX_ERR_NOT_AVAILABLE; // simply disqualify ourselves
        }
        if (NULL == (_base_path = strdup(dstor_tmpdir))) {
            rc = PMIX_ERR_OUT_OF_RESOURCE;
            PMIX_ERROR_LOG(rc);
            goto err_exit;
        }
        _esh_session_map_search = _esh_session_map_search_client;
    }

    rc = _esh_session_tbl_add(&tbl_idx);
    if (PMIX_SUCCESS != rc) {
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }

    ns_map = _esh_session_map(pmix_globals.myid.nspace, tbl_idx);
    if (NULL == ns_map) {
        rc = PMIX_ERR_OUT_OF_RESOURCE;
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }

    if (PMIX_SUCCESS != (rc =_esh_session_init(tbl_idx, ns_map, _base_path, _jobuid, _setjobuid))) {
        PMIX_ERROR_LOG(rc);
        goto err_exit;
    }

    return PMIX_SUCCESS;
err_exit:
    return rc;
}

static pmix_status_t ds12_assign_module(pmix_info_t *info, size_t ninfo,
                                        int *priority)
{
    size_t n, m;
    char **options;

    *priority = 20;
    if (NULL != info) {
        for (n=0; n < ninfo; n++) {
            if (0 == strncmp(info[n].key, PMIX_GDS_MODULE, PMIX_MAX_KEYLEN)) {
                options = pmix_argv_split(info[n].value.data.string, ',');
                for (m=0; NULL != options[m]; m++) {
                    if (0 == strcmp(options[m], "ds12")) {
                        /* they specifically asked for us */
                        *priority = 100;
                        break;
                    }
                    if (0 == strcmp(options[m], "dstore")) {
                        /* they are asking for any dstore module - we
                         * take an intermediate priority in case another
                         * dstore is more modern than us */
                        *priority = 50;
                        break;
                    }
                }
                pmix_argv_free(options);
                break;
            }
        }
    }

    return PMIX_SUCCESS;
}

static pmix_status_t ds12_setup_fork(const pmix_proc_t *peer, char ***env)
{
    pmix_status_t rc = PMIX_SUCCESS;
    ns_map_data_t *ns_map = NULL;

    pmix_output_verbose(2, pmix_gds_base_framework.framework_output,
                        "gds: dstore setup fork");

    if (NULL == _esh_session_map_search) {
        rc = PMIX_ERR_NOT_AVAILABLE;
        PMIX_ERROR_LOG(rc);
        return rc;
    }

    if (NULL == (ns_map = _esh_session_map_search(peer->nspace))) {
        rc = PMIX_ERR_NOT_AVAILABLE;
        PMIX_ERROR_LOG(rc);
        return rc;
    }

    if ((NULL == _base_path) || (strlen(_base_path) == 0)){
        rc = PMIX_ERR_NOT_AVAILABLE;
        PMIX_ERROR_LOG(rc);
        return rc;
    }

    if(PMIX_SUCCESS != (rc = pmix_setenv(PMIX_DSTORE_ESH_BASE_PATH,
                                        _ESH_SESSION_path(ns_map->tbl_idx), true, env))){
        PMIX_ERROR_LOG(rc);
    }
    return rc;
}

static pmix_status_t ds12_add_nspace(const char *nspace,
                                pmix_info_t info[],
                                size_t ninfo)
{
    pmix_status_t rc;
    size_t tbl_idx=0;
    uid_t jobuid = _jobuid;
    char setjobuid = _setjobuid;
    size_t n;
    ns_map_data_t *ns_map = NULL;

    pmix_output_verbose(2, pmix_gds_base_framework.framework_output,
                        "gds: dstore add nspace");

    if (NULL != info) {
        for (n=0; n < ninfo; n++) {
            if (0 == strcmp(PMIX_USERID, info[n].key)) {
                jobuid = info[n].value.data.uint32;
                setjobuid = 1;
                continue;
            }
        }
    }

    if (PMIX_SUCCESS != _esh_jobuid_tbl_search(jobuid, &tbl_idx)) {

        rc = _esh_session_tbl_add(&tbl_idx);
        if (PMIX_SUCCESS != rc) {
            PMIX_ERROR_LOG(rc);
            return rc;
        }
        ns_map = _esh_session_map(nspace, tbl_idx);
        if (NULL == ns_map) {
            rc = PMIX_ERROR;
            PMIX_ERROR_LOG(rc);
            return rc;
        }

        if (PMIX_SUCCESS != (rc =_esh_session_init(tbl_idx, ns_map, _base_path, jobuid, setjobuid))) {
            rc = PMIX_ERROR;
            PMIX_ERROR_LOG(rc);
            return rc;
        }
    }
    else {
        ns_map = _esh_session_map(nspace, tbl_idx);
        if (NULL == ns_map) {
            rc = PMIX_ERROR;
            PMIX_ERROR_LOG(rc);
            return rc;
        }
    }

    return PMIX_SUCCESS;
}

static pdstor_ctx *ctx;

pmix_gds_base_module_t pmix_ds12_module = {
    .name = "ds12",
    .init = ds12_init,
    .finalize = dstore_finalize,
    .assign_module = ds12_assign_module,
    .cache_job_info = dstore_cache_job_info,
    .register_job_info = dstore_register_job_info,
    .store_job_info = dstore_store_job_info,
    .store = dstore_store,
    .store_modex = dstore_store_modex,
    .fetch = dstore_fetch,
    .setup_fork = ds12_setup_fork,
    .add_nspace = ds12_add_nspace,
    .del_nspace = dstore_del_nspace,
};

