/*
 * Copyright (c) 2019      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#define UNLIKELY(x) (__builtin_expect(!!(x), 0))

#define BASE7_MASK ((1<<7) - 1)
#define BASE7_SHIFT 7
#define BASE7_CONT_FLAG (1<<7)
