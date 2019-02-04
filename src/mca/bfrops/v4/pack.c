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

static int pmix_bfrop_pack_flex(uint64_t val, uint8_t out_buf[PMIX_BASE7_MAX_BUF_SIZE])
{
    uint64_t tmp = val;
    int idx = 0;

    do {
        uint8_t val = tmp & BASE7_MASK;
        tmp >>= BASE7_SHIFT;
        if( PMIX_UNLIKELY(tmp) ) {
            val |= BASE7_CONT_FLAG;
        }
        out_buf[idx++] = val;
    } while(tmp && idx < SIZEOF_SIZE_T);

    /* If we have leftover (VERY unlikely) */
    if (PMIX_UNLIKELY(SIZEOF_SIZE_T == idx && tmp)) {
        out_buf[idx++] = tmp;
    }
    return idx;
}

pmix_status_t pmix_bfrops_base_pack_int_flex(pmix_buffer_t *buffer,
                                             const void *src,
                                             int32_t num_vals,
                                             pmix_data_type_t type)
{
    int32_t i;
    uint64_t tmp;
    char *dst;
    int tmp_size;
    uint8_t tmp_buf[PMIX_BASE7_MAX_BUF_SIZE];
    size_t val_size;
    bool is_signed = false;

    pmix_output_verbose(20, pmix_bfrops_base_framework.framework_output,
                        "pmix_bfrops_base_pack_flex * %d\n", num_vals);

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

    for (i = 0; i < num_vals; ++i) {
        bool sign_bit = 0;
        tmp = 0;
        memcpy(&tmp, (char *)src + i * val_size, val_size);
        if (is_signed) {
            sign_bit = (tmp >> (val_size * 8 - 1));
            if (sign_bit) {
                tmp = ~tmp;
                tmp <<= (sizeof(tmp) - val_size) * 8;
                tmp >>= (sizeof(tmp) - val_size) * 8;
            }
            tmp <<= 1;
            tmp |= sign_bit;
        }
        tmp_size = pmix_bfrop_pack_flex(tmp, tmp_buf);

        /* check to see if buffer needs extending */
        if (NULL == (dst = pmix_bfrop_buffer_extend(buffer, tmp_size))) {
            return PMIX_ERR_OUT_OF_RESOURCE;
        }
        memcpy(dst, tmp_buf, sizeof(uint8_t) *tmp_size);
        dst += tmp_size;
        buffer->pack_ptr += tmp_size;
        buffer->bytes_used += tmp_size;
    }

    return PMIX_SUCCESS;
}
