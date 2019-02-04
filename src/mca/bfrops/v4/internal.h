/*
 * Copyright (c) 2019      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/pmix_config.h>

#include "src/mca/bfrops/base/base.h"

#define PMIX_BASE7_MAX_BUF_SIZE (SIZEOF_SIZE_T+1)

#define BASE7_MASK ((1<<7) - 1)
#define BASE7_SHIFT 7
#define BASE7_CONT_FLAG (1<<7)

pmix_status_t pmix_bfrops_base_pack_int_flex(pmix_buffer_t *buffer,
                                             const void *src,
                                             int32_t num_vals,
                                             pmix_data_type_t type);
pmix_status_t pmix_bfrops_base_unpack_int_flex(pmix_buffer_t *buffer,
                                               void *dest,
                                               int32_t *num_vals,
                                               pmix_data_type_t type);
