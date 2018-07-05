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
#include <pmix_common.h>
#include "src/include/pmix_globals.h"
#include "src/class/pmix_list.h"
#include "src/client/pmix_client_ops.h"
#include "src/server/pmix_server_ops.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/util/argv.h"

#include "gds_dstore.h"
#include "src/mca/gds/ds_common/gds_dstore.h"

static pmix_status_t ds21_assign_module(pmix_info_t *info, size_t ninfo,
                                        int *priority)
{
    size_t n, m;
    char **options;

    *priority = 30;
    if (NULL != info) {
        for (n=0; n < ninfo; n++) {
            if (0 == strncmp(info[n].key, PMIX_GDS_MODULE, PMIX_MAX_KEYLEN)) {
                options = pmix_argv_split(info[n].value.data.string, ',');
                for (m=0; NULL != options[m]; m++) {
                    if (0 == strcmp(options[m], "ds21")) {
                        /* they specifically asked for us */
                        *priority = 110;
                        break;
                    }
                    if (0 == strcmp(options[m], "dstore")) {
                        /* they are asking for any dstore module - we
                         * take an intermediate priority in case another
                         * dstore is more modern than us */
                        *priority = 55;
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

pmix_gds_base_module_t pmix_ds21_module = {
    .name = "ds21",
    .init = dstore_init,
    .finalize = dstore_finalize,
    .assign_module = ds21_assign_module,
    .cache_job_info = dstore_cache_job_info,
    .register_job_info = dstore_register_job_info,
    .store_job_info = dstore_store_job_info,
    .store = dstore_store,
    .store_modex = dstore_store_modex,
    .fetch = dstore_fetch,
    .setup_fork = dstore_setup_fork,
    .add_nspace = dstore_add_nspace,
    .del_nspace = dstore_del_nspace,
};

