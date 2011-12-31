/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */

#ifndef KOGMO_RTDB_IPC_HOUSEKEEPING_H
#define KOGMO_RTDB_IPC_HOUSEKEEPING_H


// Only for the Manager Process:

#define KOGMO_RTDB_MANAGER_ALIVE_INTERVAL 0.25

int
kogmo_rtdb_objmeta_purge_objs (kogmo_rtdb_handle_t *db_h);

int
kogmo_rtdb_objmeta_purge_procs (kogmo_rtdb_handle_t *db_h);

int
kogmo_rtdb_kill_procs (kogmo_rtdb_handle_t *db_h, int sig);

int
kogmo_rtdb_objmeta_upd_stats (kogmo_rtdb_handle_t *db_h);

#ifdef KOGMO_RTDB_HARDREALTIME
#define KOGMO_RTDB_PURGE_KEEPALLOC_PERCFREE 0
#define KOGMO_RTDB_PURGE_KEEPALLOC_MAXSECS 0
#else
#define KOGMO_RTDB_PURGE_KEEPALLOC_PERCFREE 20
#define KOGMO_RTDB_PURGE_KEEPALLOC_MAXSECS 10
#endif

#endif /* KOGMO_RTDB_IPC_HOUSEKEEPING_H */
