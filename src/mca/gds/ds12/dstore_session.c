#include <pmix_common.h>
 
#include "src/include/pmix_globals.h"
#include "src/class/pmix_list.h"

#include "dstore_session.h"
#include "dstore_lock.h"
#include "dstore_nspace.h"

#define ESH_INIT_SESSION_TBL_SIZE 2

pmix_value_array_t *_session_array = NULL;
