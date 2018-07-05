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

#include <pmix_common.h>
 
#include "src/include/pmix_globals.h"
#include "src/class/pmix_list.h"

#define ESH_INIT_SESSION_TBL_SIZE 2

pmix_value_array_t *_session_array = NULL;
