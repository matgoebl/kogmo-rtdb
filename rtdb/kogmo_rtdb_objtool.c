/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2009 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_objtool.c
 * \brief Object-Manipulation-Tool for the Real-time Database
 *
 * Copyright (c) 2009 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */


#include "stdio.h"
#include "kogmo_rtdb_utils.h"
#include "kogmo_rtdb_internal.h"

#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void
usage (void)
{
  fprintf(stderr,
"KogMo-RTDB Object-Manipulation-Tool (Rev.%d) (c) Matthias Goebl <matthias.goebl*goebl.net> RCS-TUM\n"
"Usage: kogmo_rtdb_objtool [.....]\n"
" -i OID | -n NAME      use object with OID or NAME (regular expression if starting with ~)\n"
" -o [r|w|c|i|d]   select operation: read/write/change/insert/delete object (and write/,, to file)\n"
" -m FILE  read/insert object (with) meta data to/from FILE\n"
" -d FILE  read/write/insert object (with) data from/to FILE\n"
" -N NAME  set name to NAME\n"
" -P NAME  set parent oid to object referenced by NAME (or eg. OID 7: '~(7)' )\n"
" -T TID   set type id to TID\n"
//" -X FLAGS set flags to FLAGS (DANGEROUS!)\n"
" -M CYCLE set max cycle time to CYCLE\n"
" -I CYCLE set min/avg cycle time to CYCLE\n"
" -h       print this help message\n\n", KOGMO_RTDB_REV);
  exit(1);
}


int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_objid_t oret, oid;
  kogmo_rtdb_objsize_t olen;
  kogmo_rtdb_obj_info_t objmeta;
  struct {
    kogmo_rtdb_subobj_base_t base;
    char data[5*1024*1024];
  } objdata;
  int ret, fd, opt;
  ssize_t len;

  char *do_name=NULL, do_op='\0', *do_datafile=NULL, *do_metafile=NULL;
  char *do_set_name=NULL, *do_set_parent=NULL;
  kogmo_rtdb_objid_t do_oid=0;
  kogmo_rtdb_objtype_t do_set_typeid=0;
  float do_set_maxcycle=0, do_set_mincycle=0; 
  uint32_t do_set_flags;


  kogmo_rtdb_require_revision(KOGMO_RTDB_REV);

  while( ( opt = getopt (argc, argv, "i:n:o:d:m:N:P:T:X:M:I:h") ) != -1 )
    switch(opt)
      {
        case 'n': do_name = optarg; break;
        case 'i': do_oid = strtoll(optarg,NULL,0); break;
        case 'o': do_op = optarg[0]; break;
        case 'm': do_metafile = optarg; break;
        case 'd': do_datafile = optarg; break;
        case 'N': do_set_name = optarg; break;
        case 'P': do_set_parent = optarg; break;
        case 'T': do_set_typeid = strtoll(optarg,NULL,0); break;
        case 'X': do_set_flags = strtoll(optarg,NULL,0); break;
        case 'M': do_set_maxcycle = atof(optarg); break;
        case 'I': do_set_mincycle = atof(optarg); break;
        case 'h':
        default: usage(); break;
      }

  if ( do_op != 'r' && do_op != 'w' && do_op != 'c' && do_op != 'i' && do_op != 'd' )
    usage();

  kogmo_rtdb_connect_initinfo(&dbinfo, "", "kogmo_rtdb_objtool", 1.0);
  dbinfo.flags = KOGMO_RTDB_CONNECT_FLAGS_ADMIN;
  oret = kogmo_rtdb_connect(&dbc,&dbinfo);
  if ( oret < 0 )
    DIE("cannot connect to rtdb (error %i)", oret);


  if ( do_name )
    {
      do_oid = kogmo_rtdb_obj_searchinfo (dbc, do_name, 0, 0, 0, 0, NULL, 1);
      if ( do_oid <= 0 ) 
        DIE("cannot find rtdb object '%s'", do_name);
    }

  if ( do_op == 'r' || do_op == 'c' || do_op == 'd' )
    if ( do_oid <= 0 )
      DIE("no object given by oid or name");

  if ( do_op == 'd' )
    {
      objmeta.oid = do_oid;
      ret = kogmo_rtdb_obj_delete (dbc, &objmeta);
      if ( ret < 0 )
        DIE("cannot delete object %i (error  %i)", do_oid, ret);
      exit(0);
    }

  if ( do_op == 'r' )
    {
      if ( do_metafile )
        {
          fd = open( do_metafile, O_WRONLY|O_CREAT, S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
          if ( fd < 0 )
            DIE("cannot open file '%s' for writing meta data (error %i)",do_metafile,errno);
          ret = kogmo_rtdb_obj_readinfo ( dbc, do_oid, 0, &objmeta );
          if ( ret < 0 )
            DIE("cannot read meta data of object %i (error %i)",do_oid,ret);
          len = write ( fd, &objmeta, sizeof(objmeta) );
          if ( len != sizeof(objmeta) )
            DIE("cannot write meta data to file '%s' (error %i)",do_metafile,errno);
          ret = close ( fd );
          if ( ret < 0 )
            DIE("cannot close file '%s' after writing meta data (error %i)",do_metafile,errno);
        }

      if ( do_datafile )
        {
          fd = open( do_datafile, O_WRONLY|O_CREAT, S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
          if ( fd < 0 )
            DIE("cannot open file '%s' for writing data (error %i)",do_datafile,errno);
          olen = kogmo_rtdb_obj_readdata ( dbc, do_oid, 0, &objdata, sizeof(objdata) );
          if ( olen > sizeof(objdata) )
            DIE("object too large ( %lli > %lli )", (long long int)olen, (long long int)sizeof(objdata) );
          if ( olen < 0 )
            DIE("cannot read data of object %i (error %i)",do_oid,olen);
          len = write ( fd, &objdata, olen );
          if ( len != olen )
            DIE("cannot write data to file '%s' (error %i)",do_datafile,olen);
          ret = close ( fd );
          if ( ret < 0 )
            DIE("cannot close file '%s' after writing data (error %i)",do_datafile,errno);
        }
      exit(0);
    }


  len = 0;
  if ( do_datafile )
    {
      fd = open( do_datafile, O_RDONLY);
      if ( fd < 0 )
        DIE("cannot open file '%s' for reading data (error %i)",do_datafile,errno);
      len = read ( fd, &objdata, sizeof(objdata) );
      if ( len <= 0 )
        DIE("cannot read data from file '%s' (error %i)",do_datafile,errno);
      ret = close ( fd );
      if ( ret < 0 )
        DIE("cannot close file '%s' after reading data (error %i)",do_datafile,errno);
      if ( objdata.base.size != len )
        ERR("object data from file '%s' contains wrong size %lli! fixed with real size %lli!",do_datafile,(long long int)objdata.base.size,(long long int)len);
      objdata.base.size = len;
    }

  if ( do_op == 'i' || do_op == 'c' )
    {
      kogmo_rtdb_obj_initinfo (dbc, &objmeta, do_set_name != NULL ? do_set_name : "",
                               do_set_typeid, len);
      if ( do_metafile )
        {
          fd = open( do_metafile, O_RDONLY);
          if ( fd < 0 )
            DIE("cannot open file '%s' for reading meta data (error %i)",do_metafile,errno);
          len = read ( fd, &objmeta, sizeof(objmeta) );
          if ( len != sizeof(objmeta) )
            DIE("cannot read meta data from file '%s' (error %i)",do_metafile,errno);
          ret = close ( fd );
          if ( ret < 0 )
            DIE("cannot close file '%s' after reading meta data (error %i)",do_metafile,errno);
        }
      if ( do_set_name )
        strncpy(objmeta.name,do_set_name,sizeof(objmeta.name));
      //if ( do_set_flags )
      //  objmeta.flags = do_set_flags;
      if ( do_set_typeid )
        objmeta.otype = do_set_typeid;
      if ( do_set_parent )
        {
          oid = kogmo_rtdb_obj_searchinfo (dbc, do_set_parent, 0, 0, 0, 0, NULL, 1);
          if ( oid <= 0 ) 
          DIE("cannot find parent rtdb object '%s'", do_set_parent);
          objmeta.parent_oid = oid;
        }

      if ( do_op == 'i' )
        {
          objmeta.flags.write_allow = 1;
          objmeta.flags.persistent = 1;
          oid = kogmo_rtdb_obj_insert (dbc, &objmeta);
          if ( oid < 0 )
            DIE("cannot insert object (error %i)", oid);
          do_oid = oid;
        }

      if ( do_op == 'c' )
        {
          oid = kogmo_rtdb_obj_changeinfo (dbc, do_oid, &objmeta);
          if ( oid < 0 )
            DIE("cannot change object (error %i)", oid);
          do_oid = oid;
        }
    }

  if ( len > 0 && ( do_op == 'w' || do_op == 'i' || do_op == 'c' ) )
    {
      ret = kogmo_rtdb_obj_writedata (dbc, do_oid, &objdata);
      if ( ret < 0 )
        DIE("cannot write object data (error %i)", ret);
    }
  return 0;
}
