/*
 * Copyright (c) 2015-2018 Intel, Inc. All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * Copyright (c) 2016-2017 Mellanox Technologies, Inc.
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
#include <sys/stat.h>
#include <sys/file.h>
#include <dirent.h>
#include <errno.h>
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
#include <time.h>

#include <pmix_common.h>

#include "src/include/pmix_globals.h"
#include "src/class/pmix_list.h"
#include "src/client/pmix_client_ops.h"
#include "src/server/pmix_server_ops.h"
#include "src/util/argv.h"
#include "src/util/compress.h"
#include "src/util/error.h"
#include "src/util/output.h"
#include "src/util/pmix_environ.h"
#include "src/util/hash.h"
#include "src/mca/preg/preg.h"

#include "src/mca/gds/base/base.h"
#include "gds_dstore_v12.h"
#include "src/mca/pshmem/base/base.h"

#include "src/mca/gds/ds_common/gds_dstore_common.h"

pmix_gds_base_module_t pmix_ds12_module = {
    .name = "ds12",
    .init = dstore_init,
    .finalize = dstore_finalize,
    .assign_module = dstore_assign_module,
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
