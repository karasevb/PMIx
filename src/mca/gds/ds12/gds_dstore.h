/*
 * Copyright (c) 2015-2017 Intel, Inc. All rights reserved.
 * Copyright (c) 2017-2018 Mellanox Technologies, Inc.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef GDS_DSTORE_12_H
#define GDS_DSTORE_12_H

#include "src/mca/gds/gds.h"
#include "src/mca/common/pmix_common_dstore.h"

/* the component must be visible data for the linker to find it */
PMIX_EXPORT extern pmix_gds_base_component_t mca_gds_ds12_component;
extern pmix_gds_base_module_t pmix_ds12_module;

typedef struct pmix_ds12_ctx_s {
    pmix_value_array_t *session_array;
    pmix_value_array_t *ns_map_array;

    pmix_common_dstor_lock_ctx_t *lock_ctx;
    pmix_common_lock_callbacks_t *lock_cbs;
} pmix_ds12_ctx_t;

#endif // GDS_DSTORE_12_H
