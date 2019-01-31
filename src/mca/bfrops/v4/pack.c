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

int pack_size(uint64_t size, uint8_t out_buf[9])
{
    uint64_t tmp = size;
    int idx = 0;
    do {
        uint8_t val = tmp & BASE7_MASK;
        tmp >>= BASE7_SHIFT;
        if( UNLIKELY(tmp) ) {
            val |= BASE7_CONT_FLAG;
        }
        out_buf[idx++] = val;
    } while(tmp && idx < 8);

    /* If we have leftover (VERY unlikely) */
    if (UNLIKELY(8 == idx && tmp)) {
        out_buf[idx++] = tmp;
    }
    return idx;
}
