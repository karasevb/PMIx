/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015-2017 Intel, Inc. All rights reserved.
 * Copyright (c) 2016-2019 Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2018      IBM Corporation.  All rights reserved.
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

#include <pmix_common.h>
#include "src/include/pmix_globals.h"

#include "src/class/pmix_list.h"
#include "src/util/argv.h"
#include "src/util/error.h"

#include "src/mca/gds/base/base.h"
#include "src/server/pmix_server_ops.h"


char* pmix_gds_base_get_available_modules(void)
{
    if (!pmix_gds_globals.initialized) {
        return NULL;
    }

    return strdup(pmix_gds_globals.all_mods);
}

/* Select a gds module per the given directives */
pmix_gds_base_module_t* pmix_gds_base_assign_module(pmix_info_t *info, size_t ninfo)
{
    pmix_gds_base_active_module_t *active;
    pmix_gds_base_module_t *mod = NULL;
    int pri, priority = -1;

    if (!pmix_gds_globals.initialized) {
        return NULL;
    }

    PMIX_LIST_FOREACH(active, &pmix_gds_globals.actives, pmix_gds_base_active_module_t) {
        if (NULL == active->module->assign_module) {
            continue;
        }
        if (PMIX_SUCCESS == active->module->assign_module(info, ninfo, &pri)) {
            if (pri < 0) {
                /* use the default priority from the component */
                pri = active->pri;
            }
            if (priority < pri) {
                mod = active->module;
                priority = pri;
            }
        }
    }

    return mod;
}

pmix_status_t pmix_gds_base_setup_fork(const pmix_proc_t *proc,
                                       char ***env)
{
    pmix_gds_base_active_module_t *active;
    pmix_status_t rc;

    if (!pmix_gds_globals.initialized) {
        return PMIX_ERR_INIT;
    }

    PMIX_LIST_FOREACH(active, &pmix_gds_globals.actives, pmix_gds_base_active_module_t) {
        if (NULL == active->module->setup_fork) {
            continue;
        }
        if (PMIX_SUCCESS != (rc = active->module->setup_fork(proc, env))) {
            return rc;
        }
    }

    return PMIX_SUCCESS;
}

static int proc_compare(pmix_list_item_t **_a,
                        pmix_list_item_t **_b)
{
    pmix_proc_t *a = ((pmix_proc_caddy_t*)*_a)->proc;
    pmix_proc_t *b = ((pmix_proc_caddy_t*)*_b)->proc;
    int ret;

    if (0 == (ret = strcmp(a->nspace, b->nspace))) {
        assert(a->rank != b->rank);
        return a->rank - b->rank;
    }
    return ret;
}

pmix_status_t pmix_gds_base_store_modex(struct pmix_namespace_t *nspace,
                                        pmix_buffer_t * buff,
                                        pmix_gds_base_ctx_t ctx,
                                        pmix_gds_base_store_modex_cb_fn_t cb_fn,
                                        void *cbdata)
{
    pmix_status_t rc = PMIX_SUCCESS;
    pmix_namespace_t * ns = (pmix_namespace_t *)nspace;
    pmix_buffer_t bkt;
    pmix_byte_object_t bo, bo2;
    int32_t cnt = 1;
    char byte;
    pmix_collect_t ctype;
    bool have_ctype = false;
    pmix_server_trkr_t *trk = (pmix_server_trkr_t*)cbdata;
    pmix_proc_t proc;
    pmix_buffer_t pbkt;
    pmix_rank_t rel_rank = 0;
    char *nspace_tmp = NULL;
    pmix_proc_caddy_t *p;
    pmix_list_t plist_sort;
    pmix_rank_t nprocs = 0;
    size_t i;

    /* Loop over the enclosed byte object envelopes and
     * store them in our GDS module */
    cnt = 1;
    PMIX_BFROPS_UNPACK(rc, pmix_globals.mypeer,
            buff, &bo, &cnt, PMIX_BYTE_OBJECT);
    while (PMIX_SUCCESS == rc) {
        PMIX_CONSTRUCT(&bkt, pmix_buffer_t);
        PMIX_LOAD_BUFFER(pmix_globals.mypeer, &bkt, bo.bytes, bo.size);
        /* unpack the data collection flag */
        cnt = 1;
        PMIX_BFROPS_UNPACK(rc, pmix_globals.mypeer,
                &bkt, &byte, &cnt, PMIX_BYTE);
        if (PMIX_ERR_UNPACK_READ_PAST_END_OF_BUFFER == rc) {
            /* no data was returned, so we are done with this blob */
            PMIX_DESTRUCT(&bkt);
            break;
        }
        if (PMIX_SUCCESS != rc) {
            /* we have an error */
            PMIX_DESTRUCT(&bkt);
            goto error;
        }

        // Check that this blob was accumulated with the same data collection setting
        if (have_ctype) {
            if (ctype != (pmix_collect_t)byte) {
                rc = PMIX_ERR_INVALID_ARG;
                pbkt.base_ptr = NULL;
                goto error;
            }
        }
        else {
            ctype = (pmix_collect_t)byte;
            have_ctype = true;
        }
        /* create the sorted tracker proc list */
        PMIX_CONSTRUCT(&plist_sort, pmix_list_t);
        for(i = 0; i < trk->npcs; i++) {
            p = PMIX_NEW(pmix_proc_caddy_t);
            p->proc = &trk->pcs[i];
            pmix_list_append(&plist_sort, &p->super);
        }
        pmix_list_sort(&plist_sort, proc_compare);

        /* unpack the enclosed blobs from the various peers */
        cnt = 1;
        PMIX_BFROPS_UNPACK(rc, pmix_globals.mypeer,
                &bkt, &bo2, &cnt, PMIX_BYTE_OBJECT);
        while (PMIX_SUCCESS == rc) {
            /* unpack all the kval's from this peer and store them in
             * our GDS. Note that PMIx by design holds all data at
             * the server level until requested. If our GDS is a
             * shared memory region, then the data may be available
             * right away - but the client still has to be notified
             * of its presence. */

            /* setup the byte object for unpacking */
            PMIX_CONSTRUCT(&pbkt, pmix_buffer_t);
            PMIX_LOAD_BUFFER(pmix_globals.mypeer, &pbkt, bo2.bytes, bo2.size);
            /* unload the proc that provided this data */
            cnt = 1;
            PMIX_BFROPS_UNPACK(rc, pmix_globals.mypeer, &pbkt, &rel_rank, &cnt,
                               PMIX_PROC_RANK);
            if (PMIX_SUCCESS != rc) {
                PMIX_ERROR_LOG(rc);
                pbkt.base_ptr = NULL;
                PMIX_DESTRUCT(&pbkt);
                PMIX_DESTRUCT(&bkt);
                goto error;
            }

            /* calculate proc form the relative rank */
            proc.rank = PMIX_RANK_UNDEF;
            nspace_tmp = NULL;
            PMIX_LIST_FOREACH(p, &plist_sort, pmix_proc_caddy_t) {
                if (nspace_tmp && (0 == strcmp(nspace_tmp, p->proc->nspace))) {
                    continue;
                }
                nprocs = 0;
                nspace_tmp = p->proc->nspace;
                PMIX_LIST_FOREACH(ns, &pmix_server_globals.nspaces, pmix_namespace_t) {
                    if (0 == strcmp(ns->nspace, p->proc->nspace)) {
                        nprocs = ns->nprocs;
                        break;
                    }
                }
                if (rel_rank < nprocs) {
                    PMIX_PROC_LOAD(&proc, p->proc->nspace, rel_rank);
                    break;
                }
                rel_rank -= nprocs;
            }
            if (PMIX_RANK_UNDEF == proc.rank) {
                PMIX_ERROR_LOG(rc);
                pbkt.base_ptr = NULL;
                PMIX_DESTRUCT(&pbkt);
                PMIX_DESTRUCT(&bkt);
                goto error;
            }

            rc = cb_fn(ctx, &proc, &pbkt);
            if (PMIX_SUCCESS != rc) {
                pbkt.base_ptr = NULL;
                PMIX_DESTRUCT(&pbkt);
                PMIX_DESTRUCT(&bkt);
                goto error;
            }
            pbkt.base_ptr = NULL;
            PMIX_DESTRUCT(&pbkt);
            PMIX_BYTE_OBJECT_DESTRUCT(&bo2);
            /* get the next blob */
            cnt = 1;
            PMIX_BFROPS_UNPACK(rc, pmix_globals.mypeer,
                    &bkt, &bo2, &cnt, PMIX_BYTE_OBJECT);
        }
        PMIX_DESTRUCT(&bkt);
        if (PMIX_ERR_UNPACK_READ_PAST_END_OF_BUFFER == rc) {
            rc = PMIX_SUCCESS;
        } else if (PMIX_SUCCESS != rc) {
            goto error;
        }
        /* unpack and process the next blob */
        cnt = 1;
        PMIX_BFROPS_UNPACK(rc, pmix_globals.mypeer,
                buff, &bo, &cnt, PMIX_BYTE_OBJECT);
    }
    if (PMIX_ERR_UNPACK_READ_PAST_END_OF_BUFFER == rc) {
        rc = PMIX_SUCCESS;
    }

error:
    if (PMIX_SUCCESS != rc) {
        PMIX_ERROR_LOG(rc);
    }

    return rc;
}
