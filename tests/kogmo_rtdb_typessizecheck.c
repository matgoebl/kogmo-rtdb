/*! \file kogmo_rtdb_typessizecheck.c
 * \brief This program tests whether the kogmo-rtdb standard types have the correct size
 *
 * Copyright (c) 2005,2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#define _FILE_OFFSET_BITS 64

#include "kogmo_rtdb.h"
#include "../record/kogmo_rtdb_timeidx.h"
#include "../record/kogmo_rtdb_avirawcodec.h"
#include <stdio.h> /* printf */
#include <string.h>

#define SIZECHECK(type,reference) do {\
 int size = sizeof(type); \
 if(size != reference) { \
  printf("ERROR: sizeof(%s) = %lli != %lli\n",#type,(long long int)sizeof(type),(long long int)reference); \
   err=1; \
 } else if (verbose) { \
  printf("ok: sizeof(%s) = %lli\n",#type,(long long int)sizeof(type)); \
 } } while(0)

int
main (int argc, char **argv)
{
 int err=0;
 int verbose = argc > 1 ? 1 : 0;

 SIZECHECK(char, 1);
 SIZECHECK(int,4);
 SIZECHECK(long long int,8);
 SIZECHECK(int32_t,4);

 SIZECHECK(kogmo_rtdb_obj_info_t, 112);
 SIZECHECK(kogmo_rtdb_obj_slot_t, 32);
 // SIZECHECK(kogmo_rtdb_connect_info_t, 20);
 SIZECHECK(kogmo_rtdb_subobj_base_t, 24);
 SIZECHECK(kogmo_rtdb_obj_base_t, 24);
 SIZECHECK(kogmo_rtdb_subobj_c3_rtdb_t, 500);
 SIZECHECK(kogmo_rtdb_obj_c3_rtdb_t, 524);
 SIZECHECK(kogmo_rtdb_subobj_c3_process_t, 188);
 SIZECHECK(kogmo_rtdb_obj_c3_process_t, 212);
 SIZECHECK(kogmo_rtdb_subobj_a2_image_t, 24);
 SIZECHECK(kogmo_rtdb_obj_a2_image_t, 48);

 SIZECHECK(avirawheader_t, 11472);
 SIZECHECK(riffchunk_t, 8);
 SIZECHECK(timeidx_info_t, 76);
 SIZECHECK(timeidx_entry_t, 16);

 // TODO: add my bit-ordering test including doubles!

 return err;
}
