/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_rtmalloc.h
 * \brief Memory Allocation within the Real-time Vehicle Database
 *
 * Copyright (c) 2005-2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_RTMALLOC_H
#define KOGMO_RTDB_RTMALLOC_H


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_mem_alloc (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objsize_t size );

void
kogmo_rtdb_obj_mem_free (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objsize_t idx,
                         kogmo_rtdb_objsize_t size );

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_mem_init (kogmo_rtdb_handle_t *db_h);

kogmo_rtdb_objsize_t
kogmo_rtdb_obj_mem_attach (kogmo_rtdb_handle_t *db_h);

void
kogmo_rtdb_obj_mem_destroy (kogmo_rtdb_handle_t *db_h);


#endif /* KOGMO_RTDB_RTMALLOC_H */
