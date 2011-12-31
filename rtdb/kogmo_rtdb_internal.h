/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_internal.h
 * \brief Some internal Definitions
 *
 * Copyright (c) 2003-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */


// Internal Master-Header to access the Real-time Vehicle Database,
// replaces kogmo_rtdb.h while compiling the core kogmo-rtdb!
#ifndef KOGMO_RTDB_INTERNAL_H
#define KOGMO_RTDB_INTERNAL_H
#define KOGMO_RTDB_H


// #define __USE_UNIX98 /* for PTHREAD_MUTEX_ERRORCHECK in pthread.h on CentOS 4 */
#include <stdlib.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/user.h> /* PAGE_SIZE */
#include <regex.h>




// Ugly, but for compatibility:
#define min_cycletime avg_cycletime
#define MIN_CYCLETIME(MIN,MAX) ( (MIN) < (MAX) ? (MIN) : (MAX) )
#define MAX_CYCLETIME(MIN,MAX) ( (MIN) < (MAX) ? (MAX) : (MIN) )

#define UNSIGNED_CEIL(x) ( (x)-(int)(x) > 0 ? (int)((x)+1) : (int)(x) )


// Define RTDB-Types and internal Handle
#define KOGMO_RTDB_DONT_DEFINE_HANDLE
#include "kogmo_rtdb_version.h"
#include "kogmo_time.h"

#include "kogmo_rtdb_types.h"
#include "kogmo_rtdb_obj_base.h"

#include "kogmo_rtdb_ipc_posix.h"
#include "kogmo_rtdb_obj_local.h"

// Define Functions
#include "kogmo_rtdb_funcs.h"

// Define Object Type-IDs
#include "kogmo_rtdb_obj_tids.h"

// Define Base Objects
#include "kogmo_rtdb_obj_base.h"
#include "kogmo_rtdb_obj_base_funcs.h"

#include "kogmo_rtdb_utils.h"


// RTDB-Internal Functions
#include "kogmo_rtdb_rtmalloc.h"
#include "kogmo_rtdb_helpers.h"
#include "kogmo_rtdb_housekeeping.h"
#include "kogmo_rtdb_objdata.h"
#include "kogmo_rtdb_objmeta.h"
#include "kogmo_rtdb_proc_obj_notify.h"
#include "kogmo_rtdb_trace.h"





#define KOGMO_RTDB_IPC_POLL_MUTEX_USECS_DEFAULT (500)
#define KOGMO_RTDB_IPC_POLL_CONDVAR_USECS_DEFAULT (2000)




#if defined(RTMALLOC_tlsf)
#define KOGMO_RTDB_REVSPEC_DSA "+rtmalloc"
#define MALLOC_COPYRIGHT "Includes TLSF allocator by M.Masmano, I.Ripoll, A.Crespo (GPLv2/LGPLv2.1 License)"
#elif defined(RTMALLOC_suba)
#define KOGMO_RTDB_REVSPEC_DSA "+nonrtmalloc"
#define MALLOC_COPYRIGHT "Includes suba allocator by M.Allen (MIT License)"
#else
#define KOGMO_RTDB_REVSPEC_DSA "+staticmalloc"
#endif

#if defined(__XENO__)
#define KOGMO_RTDB_REVSPEC_RT "+xenomai"
#define KOGMO_RTDB_HARDREALTIME
#else
#define KOGMO_RTDB_REVSPEC_RT ""
#undef KOGMO_RTDB_HARDREALTIME
#endif

#if defined(KOGMO_RTDB_IPC_DO_POLLING)
#define KOGMO_RTDB_REVSPEC_POLL "+polling"
#else
#define KOGMO_RTDB_REVSPEC_POLL ""
#endif

#define KOGMO_RTDB_COPYRIGHT \
                "KogMo-RTDB: Real-time Database for Cognitive Automobiles - www.kogmo-rtdb.de\n" \
                "Copyright (c) 2003-2011 Matthias Goebl <matthias.goebl*goebl.net>\n" \
                "Lehrstuhl fuer Realzeit-Computersysteme, Technische Universitaet Muenchen\n" \
                "Licensed under the Apache License Version 2.0.\n" \
                MALLOC_COPYRIGHT"\n"


#define KOGMO_RTDB_REVSPEC KOGMO_RTDB_REVSPEC_DSA""KOGMO_RTDB_REVSPEC_RT""KOGMO_RTDB_REVSPEC_POLL""KOGMO_RTDB_REV_ORIGIN







//#include "rtdb/rtmalloc/tscmeasure.c"
//#define TTA do { if(getenv("MEASURE")) tscmeasure_start(); } while(0)
//#define TTB do { if(getenv("MEASURE")) tscmeasure_stop_print(); } while(0)


#endif /* KOGMO_RTDB_INTERNAL_H */
