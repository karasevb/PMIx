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

#ifndef GDS_DSTORE_21_H
#define GDS_DSTORE_21_H

#include "src/mca/gds/gds.h"

#define ESH_ENV_LOCK_DISTR          "ESH_LOCK_DISTR"
#define ESH_ENV_NUMLOCKS            "ESH_NUM_LOCKS"
#define ESH_ENV_LOCK_IDX            "ESH_LOCK_IDX"

/* the component must be visible data for the linker to find it */
PMIX_EXPORT extern pmix_gds_base_component_t mca_gds_ds21_component;
extern pmix_gds_base_module_t pmix_ds21_module;

#endif // GDS_DSTORE_21_H
