/*
 * Copyright (c) 2019      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <src/include/pmix_config.h>

#include "internal.h"

static int pmix_bfrop_unpack_flex(char in_buf[], uint64_t *out_val)
{
    uint64_t value = 0, shift = 0;
    int idx = 0;
    uint8_t val = 0;
    do {
        val = in_buf[idx++];
        value = value + (((uint64_t)val & BASE7_MASK) << shift);
        shift += BASE7_SHIFT;
    } while( PMIX_UNLIKELY((val & BASE7_CONT_FLAG) && (idx < SIZEOF_SIZE_T)) );

    /* If we have leftover (VERY unlikely) */
    if (PMIX_UNLIKELY(8 == idx && (val & BASE7_CONT_FLAG))) {
        val = in_buf[idx++];
        value = value + ((uint64_t)val << shift);
    }
    *out_val = value;
    return idx;
}

pmix_status_t pmix_bfrops_base_unpack_int_flex(pmix_buffer_t *buffer, void *dest,
                                           int32_t *num_vals,
                                           pmix_data_type_t type)
{
    int32_t i;
    uint64_t *desttmp = (uint64_t*) dest;
    uint64_t tmp;
    int unpack_sz;
    size_t val_size;
    bool is_signed = false;

    pmix_output_verbose(20, pmix_bfrops_base_framework.framework_output,
                        "pmix_bfrops_base_unpack_flex * %d\n", (int)*num_vals);

    /* stub to identify that unpacking is ended */
    if (buffer->pack_ptr == buffer->unpack_ptr) {
        return PMIX_ERR_UNPACK_READ_PAST_END_OF_BUFFER;
    }

    switch (type) {
        case PMIX_INT16:
            is_signed = true;
        case PMIX_UINT16:
            val_size = SIZEOF_SHORT;
            break;
        case PMIX_INT32:
            is_signed = true;
        case PMIX_UINT32:
            val_size = SIZEOF_INT;
            break;
        case PMIX_INT64:
            is_signed = true;
        case PMIX_SIZE:
        case PMIX_UINT64:
            val_size = SIZEOF_SIZE_T;
            break;
        default:
            return PMIX_ERR_BAD_PARAM;
    }

    /* unpack the data */
    for (i = 0; i < (*num_vals); ++i) {
        bool sign_bit = 0;
        unpack_sz = pmix_bfrop_unpack_flex(buffer->unpack_ptr, &tmp);
        if ((uint32_t)unpack_sz > (val_size + 1)) {
            return PMIX_ERR_UNPACK_FAILURE;
        }
        sign_bit = tmp & 1;
        if (is_signed) {
            tmp >>= 1;
            if (sign_bit) {
                tmp = (tmp + 1) * (-1);
            }
        }
        memcpy(&desttmp[i], &tmp, sizeof(tmp));
        buffer->unpack_ptr += unpack_sz;
    }
    return PMIX_SUCCESS;
}
