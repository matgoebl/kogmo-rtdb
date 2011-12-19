/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */
/*! \file kogmo_rtdb_ipc_posix.h
 * \brief Interface to Access the Real-time Vehicle Database via IPC (internal)
 *
 * Copyright (c) 2003-2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_IPC_POSIX_H
#define KOGMO_RTDB_IPC_POSIX_H

#include <pthread.h> /* pthread_t */
#include <unistd.h>


#ifdef MACOSX
#define KOGMO_RTDB_IPC_NO_MQUEUES
#define KOGMO_RTDB_IPC_DO_POLLING
#undef O_TRUNC
#define O_TRUNC 0

#else
#include <features.h>
#endif /* MACOSX */


#ifdef KOGMO_RTDB_IPC_NO_MQUEUES
typedef unsigned long mqd_t; // will be unused, but must be defined
#else
#include <mqueue.h>
#endif


#ifdef __cplusplus
extern "C" {
namespace KogniMobil {
#endif


//! maximum size for process names (including terminating null-byte)
#define KOGMO_RTDB_PROC_NAME_MAXLEN      KOGMO_RTDB_OBJMETA_NAME_MAXLEN

/*! \brief This contains information about a process.
 */
struct kogmo_rtdb_ipc_process_t {
 kogmo_rtdb_objid_t proc_oid;
   //!< Process-ID that is unique during runtime on the local system;
   //!< Processes have their own ID-space (so far)!
   //!< (this data is from ipc-connection-management
   //!< and will be partly copied into the object metadata)
 kogmo_timestamp_t  connected_ts;
 kogmo_timestamp_t  disconnected_ts;
 float              cycletime;
 pid_t              pid; // int32
 pthread_t          tid; // uint32
 uint32_t           flags;
 char               name[KOGMO_RTDB_PROC_NAME_MAXLEN];
   //!< Name of the Process; doesn't need to be unique
};


#ifndef KOGMO_RTDB_PROC_MAX
#define KOGMO_RTDB_PROC_MAX 100
#endif
    
struct kogmo_rtdb_ipc_shm_t {
 uint32_t revision;
 // manager internals
 pid_t manager_pid;
 kogmo_timestamp_t manager_alive_ts; // 0: manager at initialization
 pthread_mutex_t global_lock;
 
 // process administration
 struct kogmo_rtdb_ipc_process_t proc[KOGMO_RTDB_PROC_MAX];
 uint32_t proc_free;
 pthread_mutex_t proc_lock;
 uint64_t proc_oid_next;
 
 long int data_size;
 // this struct is followed by the local object data in shared memory
};


#define KOGMO_RTDB_SHMID "/kogmo_rtdb"

struct kogmo_rtdb_ipc_handle_t {
 int shmfd;
 char shmid[KOGMO_RTDB_PROC_NAME_MAXLEN];
 struct kogmo_rtdb_ipc_shm_t *shm_p;
 long int shm_size;
 struct kogmo_rtdb_ipc_process_t this_process;
 int this_process_slot; // so far only for notify-fix
 mqd_t tracefd;
 mqd_t rxtracefd;
};


	


#define KOGMO_RTDB_CONNECT_FLAGS_SPECTATOR  0x0002
#define KOGMO_RTDB_CONNECT_FLAGS_MANAGER    0x1000
#define KOGMO_RTDB_CONNECT_FLAGS_ADMIN      0x2000
#define KOGMO_RTDB_CONNECT_FLAGS_SIMULATION 0x4000
#define KOGMO_RTDB_CONNECT_FLAGS_DO_POLLING 0x8000

kogmo_rtdb_objid_t
kogmo_rtdb_ipc_connect (struct kogmo_rtdb_ipc_handle_t *ipc_h,
                        char *dbhost,
                        char *procname, float cycletime, uint32_t flags,
                        char **data_p, long int *data_size,
                        void (*exit_handler) (int,void *), void *exit_arg);
int
kogmo_rtdb_ipc_connect_enable (struct kogmo_rtdb_ipc_handle_t *ipc_h);

int
kogmo_rtdb_ipc_disconnect (struct kogmo_rtdb_ipc_handle_t *ipc_h,
                           uint32_t flags);

void kogmo_rtdb_require_revision (uint32_t req_rev);

int kogmo_rtdb_ipc_mutex_init(pthread_mutex_t *mutex);
int kogmo_rtdb_ipc_mutex_destroy(pthread_mutex_t *mutex);
int kogmo_rtdb_ipc_mutex_lock(pthread_mutex_t *mutex);
int kogmo_rtdb_ipc_mutex_unlock(pthread_mutex_t *mutex);

int kogmo_rtdb_ipc_condvar_init(pthread_cond_t *condvar);
int kogmo_rtdb_ipc_condvar_destroy(pthread_cond_t *condvar);
int kogmo_rtdb_ipc_condvar_wait(pthread_cond_t *condvar, pthread_mutex_t *mutex, kogmo_timestamp_t wakeup_ts);
int kogmo_rtdb_ipc_condvar_signalall(pthread_cond_t *condvar);

int kogmo_rtdb_ipc_mq_init(mqd_t *mqfd, char *name, int do_init, int size, int len); 
int kogmo_rtdb_ipc_mq_destroy(mqd_t mqfd, char *name, int do_destroy); 
int kogmo_rtdb_ipc_mq_send(mqd_t mqfd, void *msg, int size); 
int kogmo_rtdb_ipc_mq_receive_init(mqd_t *mqfd, char *name);
int kogmo_rtdb_ipc_mq_receive(mqd_t mqfd, void *msg, int size); 
int kogmo_rtdb_ipc_mq_curmsgs(mqd_t mqfd);

#ifdef __cplusplus
}; /* namespace KogniMobil */
}; /* extern "C" */
#endif

#endif /* KOGMO_RTDB_IPC_POSIX_H */
