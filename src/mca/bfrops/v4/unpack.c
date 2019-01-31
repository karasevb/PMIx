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

uint64_t unpack_size(uint8_t in_buf[])
{
    uint64_t size = 0, shift = 0;
    int idx = 0;
    uint8_t val = 0;
    do {
        val = in_buf[idx++];
        size = size + (((uint64_t)val & BASE7_MASK) << shift);
        shift += BASE7_SHIFT;
    } while( UNLIKELY((val & BASE7_CONT_FLAG) && (idx < 8)) );

    /* If we have leftover (VERY unlikely) */
    if (UNLIKELY(8 == idx && (val & BASE7_CONT_FLAG))) {
        val = in_buf[idx++];
        size = size + ((uint64_t)val << shift);
    }
    return size;
}
