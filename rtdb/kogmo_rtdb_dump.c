/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file kogmo_rtdb_dump.c
 * \brief Dump-Program for the Real-time Database
 *
 * Copyright (c) 2005,2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */


#include "stdio.h"
#include "kogmo_rtdb_utils.h"
#include "kogmo_rtdb_internal.h"

#include <string.h>
#include <getopt.h>

void
usage (void)
{
  fprintf(stderr,
"KogMo-RTDB Dump (Rev.%d) (c) Matthias Goebl <matthias.goebl*goebl.net> RCS-TUM\n"
"Usage: kogmo_rtdb_dump [.....]\n"
" -m       dump manager info\n"
" -p       dump connected processes\n"
" -T TS    select data at or just before Timestamp or Time TS\n"
" -i ID    dump object with id ID\n"
"  -Y TS   select data that's younger (or equal) than Timestamp or Time TS\n"
"  -O TS   select data that's older (or equal) than Timestamp or Time TS\n"
"  -R      include data submit history\n"
"  -x      include hex dump of object data\n"
" -n NAME  dump object with name NAME (regular expression if starting with ~)\n"
" -r       show free resources\n"
" -a       dump all info\n"
" -q       be quiet\n"
" -w       wait for return before leaving and deleting own test-object\n"
" -t       object list as tree\n"
" -d       show database infos\n"
" -D LVL   use debug-level LVL\n"
" -c NAME  connect as process with name NAME\n"
" -o NAME  create test-object with name NAME\n"
" -W       if there is no database, wait until it gets started\n"
" -H DBHOST connect to database with the given name, must begin with 'local:',\n"
"          eg. 'local:yourname'. defaults to '%s', overrides the \n"
"          environment variable KOGMO_RTDB_DBHOST\n"
" -v       show version and copyright\n"
" -h       print this help message\n\n", KOGMO_RTDB_REV, KOGMO_RTDB_DEFAULT_DBHOST);
  exit(1);
}


void dump_recursive (kogmo_rtdb_handle_t *db_h,
                     kogmo_rtdb_objid_t parentoid, kogmo_timestamp_t ts,
                     int generation, kogmo_rtdb_objid_list_t olist);

int
main (int argc, char **argv)
{
  int i;
  char *p;
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_timestamp_t ts, scan_ts;
  kogmo_timestamp_t use_timestamp=0, select_younger=0, select_older=0;
  kogmo_timestamp_string_t tstring;
  kogmo_rtdb_obj_info_t test_om;
  kogmo_rtdb_obj_base_t test_ob;
  kogmo_rtdb_obj_info_t om;
  kogmo_rtdb_obj_base_t ob;
  kogmo_rtdb_objid_t oret;
  kogmo_rtdb_objsize_t olen,olentmp;
  int opt;
  int ret;

  int do_manager=0, do_processes=0, do_resources=0, be_quiet=0, do_hex=0, do_history=0;
  int do_tree=0, do_wait=0, do_version=0, do_rtdbversion = 0, do_dbwait = 0;
  kogmo_rtdb_objid_t do_id=0;
  char *do_name=NULL, *conn_name="c3_dump", *test_obj=NULL;

  kogmo_rtdb_require_revision(KOGMO_RTDB_REV);

  while( ( opt = getopt (argc, argv, "mprT:Y:O:i:Rn:atqdD:c:o:xwhvVH:W") ) != -1 )
    switch(opt)
      {
        case 'm': do_manager = 1; break;
        case 'p': do_processes = 1; break;
        case 'r': do_resources = 1; break;
        case 'i': do_id = atoll(optarg); break;
        case 'n': do_name = optarg; break;
        case 'a': do_manager = 1; do_processes = 1; do_name = "~."; do_rtdbversion =1; break;
        case 't': do_tree = 1; break;
        case 'q': be_quiet = 1; break;
        case 'w': do_wait = 1; break;
        case 'd': do_rtdbversion = 1; break;
        case 'D': kogmo_rtdb_debug = atoi(optarg); break;
        case 'c': conn_name = optarg; break;
        case 'o': test_obj = optarg; break;
        case 'x': do_hex = 1; break;
        case 'R': do_history = 1; break;
        case 'T': use_timestamp = kogmo_timestamp_from_string (optarg);
                  if ( !use_timestamp )
                    DIE("wrong time specification '%s'",optarg);
                  break;
        case 'Y': select_younger = kogmo_timestamp_from_string (optarg); break;
        case 'O': select_older = kogmo_timestamp_from_string (optarg); break;
        case 'W': do_dbwait = 1; break;
        case 'H': setenv("KOGMO_RTDB_DBHOST",optarg,1); break;
        case 'v': do_version = 1; break;
        case 'V': do_version = 1; break;
        case 'h':
        default: usage(); break;
      }

  if ( do_version )
    {
      printf (KOGMO_RTDB_COPYRIGHT
              "Release %i (%s)\n"
              "This is the Database Viewer.\n",
              KOGMO_RTDB_REV, KOGMO_RTDB_DATE);
      exit(0);
    }

  kogmo_rtdb_connect_initinfo(&dbinfo, "", conn_name, 1.0);
  if ( ! do_dbwait )
      dbinfo.flags = KOGMO_RTDB_CONNECT_FLAGS_TRYNOWAIT; // | KOGMO_RTDB_CONNECT_FLAGS_LIVEONERR;
  oret = kogmo_rtdb_connect(&dbc,&dbinfo);
  if ( oret < 0 )
    {
      printf("ERROR: cannot connect to RTDB '%s'.\n",
        getenv("KOGMO_RTDB_DBHOST") ? getenv("KOGMO_RTDB_DBHOST") : KOGMO_RTDB_DEFAULT_DBHOST);
      exit(1);
    }

  if ( test_obj )
    {
      printf("*** Test-Object:\nInsert '%s'\n\n", test_obj);
      kogmo_rtdb_obj_initinfo (dbc,&test_om,test_obj,1,sizeof(test_ob));
      kogmo_rtdb_obj_insert (dbc, &test_om);
      kogmo_rtdb_obj_initdata (dbc, &test_om, &test_ob);
      test_ob.base.data_ts = 0;
      kogmo_rtdb_obj_writedata (dbc, test_om.oid, &test_ob);
    }

  if ( use_timestamp )
    ts = use_timestamp;
  else
    ts = 0; //kogmo_rtdb_timestamp_now(dbc);

  if ( do_rtdbversion || do_resources)
    {
      kogmo_rtdb_obj_c3_rtdb_t rtdbobj;
      kogmo_rtdb_objid_t oid;
      oid = kogmo_rtdb_obj_searchinfo (dbc, "rtdb", KOGMO_RTDB_OBJTYPE_C3_RTDB,
                                       0, 0, 0/*ts*/, NULL, 1);
      if ( oid < 0 ) 
        DIE("cannot find rtdb object");
      olen = kogmo_rtdb_obj_readdata (dbc, oid, 0/*ts*/, &rtdbobj, sizeof(rtdbobj));
      if ( olen < 0 )
        DIE("error accessing rtdb data");
      printf("*** KogMo-RTDB: Real-time Database for Cognitive Automobiles ***\n");
      printf("%s",rtdbobj.rtdb.version_id);
      printf("* Revision of Database: %u (%s)\n",rtdbobj.rtdb.version_rev,rtdbobj.rtdb.version_date);
      printf("* Database Host: %s\n", rtdbobj.rtdb.dbhost);
      kogmo_timestamp_to_string(rtdbobj.rtdb.started_ts,tstring);
      printf("* Timestamp of Database Start: %s\n",tstring);

      if (ts)
        kogmo_timestamp_to_string(ts,tstring);
      else
        kogmo_timestamp_to_string(kogmo_rtdb_timestamp_now(dbc),tstring);
      printf("* Timestamp of Database Dump:  %s\n",tstring);

      kogmo_timestamp_to_string(kogmo_timestamp_now(), tstring);
      printf("* Timestamp of Current Time:   %s\n",tstring);

      printf("* Memory free:    %d/%d MB\n",rtdbobj.rtdb.memory_free/1024/1024, rtdbobj.rtdb.memory_max/1024/1024);
      printf("* Objects free:   %d/%d\n",rtdbobj.rtdb.objects_free, rtdbobj.rtdb.objects_max);
      printf("* Processes free: %d/%d\n",rtdbobj.rtdb.processes_free, rtdbobj.rtdb.processes_max);
      printf("\n");
    }

  if ( do_manager )
    {
      // for manager process data we don't use the given timestamp (-T)
      kogmo_rtdb_obj_c3_process_t po;
      kogmo_rtdb_objid_t manoid;
      manoid = kogmo_rtdb_obj_searchinfo (dbc, "c3_rtdb_manager",
                                          KOGMO_RTDB_OBJTYPE_C3_PROCESS, 0, 0,
                                          0/*ts*/, NULL, 1);
      if ( manoid < 0 )
        DIE("cannot find manager object");

      olen = kogmo_rtdb_obj_readdata (dbc, manoid, 0, &po, sizeof (po) );
      if ( olen < 0 )
        DIE("error accessing manager process data");

      printf("*** Manager:\nPID %d, last activity %.3f seconds ago\n\n",
           po.process.pid,
           kogmo_timestamp_diff_secs (po.base.committed_ts,
                                      kogmo_timestamp_now() ));
    }

  if ( do_processes )
    {
      kogmo_rtdb_obj_info_t om;
      kogmo_rtdb_obj_c3_process_t po;
      kogmo_rtdb_objid_t proclistoid,err;
      kogmo_rtdb_objid_list_t objlist;

      printf("*** Connected Processes:\n");

      proclistoid = kogmo_rtdb_obj_searchinfo (dbc, "processes",
                                           KOGMO_RTDB_OBJTYPE_C3_PROCESSLIST,
                                           0, 0, 0/*ts*/, NULL, 1);
      if ( proclistoid < 0 )
        DIE("cannot find processes object");

      err = kogmo_rtdb_obj_searchinfo (dbc, "", KOGMO_RTDB_OBJTYPE_C3_PROCESS,
                                       proclistoid, 0, 0/*ts*/, objlist, 0);
      if (err < 0 )
        DIE("cannot find processes");

      for (i=0; objlist[i] != 0; i++ )
        {
         ret = kogmo_rtdb_obj_readinfo (dbc, objlist[i], 0/*ts*/, &om );
         if ( ret < 0 )
           DIE("error accessing process data");
         olen = kogmo_rtdb_obj_readdata (dbc,  objlist[i], 0/*ts*/, &po, sizeof (po) );
         if ( olen < 0 )
           DIE("error accessing process data");
         kogmo_timestamp_to_string(om.created_ts, tstring);
         printf("%i. %s: PROC_OID %lli, OID %lli, PID %lu, TID %lu, Flags 0x%x,"
                " connected %s, inactive %.3f seconds",
           i+1,
           om.name,
           (long long int)po.process.proc_oid,
           (long long int)objlist[i],
           (unsigned long)po.process.pid,
           (unsigned long)po.process.tid,
           po.process.flags,
           tstring,
           kogmo_timestamp_diff_secs (po.base.committed_ts, ts ));
         if(om.deleted_ts)
           {
             kogmo_timestamp_to_string(om.deleted_ts, tstring);
             printf(", DISCONNECTED %s", tstring);
           }
         printf("\n");
        }
      printf("\n");
    }

  if ( do_id )
    {
      printf("*** Dump of Object with ID %i:\n", do_id);

      ret = kogmo_rtdb_obj_readinfo (dbc, do_id, ts, &om);
      if ( ret >= 0 )
        {
          p=kogmo_rtdb_obj_dumpinfo_str (dbc, &om);
          printf("%s",p);
          free(p);

          if ( select_younger )
            {
              olen = kogmo_rtdb_obj_readdata_younger (dbc, do_id, select_younger, &ob, sizeof(ob));
            }
          else if ( select_older )
            {
              olen = kogmo_rtdb_obj_readdata_older (dbc, do_id, select_older, &ob, sizeof(ob));
            }
          else
            {
              olen = kogmo_rtdb_obj_readdata (dbc, do_id, ts, &ob, sizeof(ob));
            }

          if ( olen >= 0 )
            {
              olentmp = ob.base.size;
              ob.base.size = olen;
              p=kogmo_rtdb_obj_dumpbase_str (dbc, &ob);
              ob.base.size = olentmp;
              printf("%s",p);
              free(p);
              if (do_hex)
                {
                  p=kogmo_rtdb_obj_dumphex_str (dbc, &ob);
                  printf("%s",p);
                  free(p);
                }

              if (do_history)
                {
                  printf("** COMMIT-HISTORY:\n");
                  scan_ts = ob.base.committed_ts;
                  do
                    {
                      olen = kogmo_rtdb_obj_readdata_older (dbc, om.oid, scan_ts, &ob, sizeof(ob));
                      if ( olen >= 0 )
                        {
                          olentmp = ob.base.size; 
                          ob.base.size = olen; 
                          p=kogmo_rtdb_obj_dumpbase_str (dbc, &ob);
                          ob.base.size = olentmp;
                          printf("%s",p);
                          free(p);
                          if (do_hex)
                            {
                              p=kogmo_rtdb_obj_dumphex_str (dbc, &ob);
                              printf("%s",p);
                              free(p);
                            }
                          scan_ts = ob.base.committed_ts; // skip the current
                        }
                    }
                  while (olen >= 0);
                }
            }
          else
            {
              printf("(no object data yet)\n");
            }
        }
      else
        {
          printf("(object not found)\n");
        }
      printf("\n");
    }

  if ( do_name )
    {
      kogmo_rtdb_objid_t oid;
      printf("*** Dump of Object(s) with name '%s':\n", do_name);

      i=0;
      while( ( oid = kogmo_rtdb_obj_searchinfo ( dbc, do_name, 0,0,0, ts, NULL, ++i ) ) > 0 )
        {
          ret = kogmo_rtdb_obj_readinfo ( dbc, oid, ts, &om );
          if ( ret < 0 )
            DIE("error %i accessing data",ret);

          p=kogmo_rtdb_obj_dumpinfo_str (dbc, &om);
          printf("%s",p);
          free(p);

          olen = kogmo_rtdb_obj_readdata (dbc, om.oid, ts, &ob, sizeof(ob));
          if ( olen >= 0 )
            {

              olentmp = ob.base.size; 
              ob.base.size = olen; 
              p=kogmo_rtdb_obj_dumpbase_str (dbc, &ob);
              ob.base.size = olentmp;
              printf("%s",p);
              free(p);
              if (do_hex)
                {
                  p=kogmo_rtdb_obj_dumphex_str (dbc, &ob);
                  printf("%s",p);
                  free(p);
                }
            }
          else
            {
              printf("(no object data yet)\n");
            }
          printf("\n");
        }
    }

  if ( do_tree )
    {
      kogmo_rtdb_objid_t oid;
      kogmo_rtdb_objid_list_t unlinked_objects;
      int unlinked_objects_found=0;
      printf("*** Dump of Object-Tree\n");

      // get list of all objects.
      // linked objects will be deleted, the unlinked will remain.
      oid = kogmo_rtdb_obj_searchinfo ( dbc, NULL, 0,0,0, ts,
                                        unlinked_objects, 0);
      if ( oid < 0 )
        DIE("error %i accessing data",oid);

      oid = kogmo_rtdb_obj_searchinfo ( dbc, NULL, KOGMO_RTDB_OBJTYPE_C3_ROOT,
                                        0,0, ts, NULL, 1);
      if ( oid < 0 )
        DIE("error %i accessing data",oid);

      dump_recursive(dbc, oid, ts, 1, unlinked_objects);


      i=-1;
      while ( unlinked_objects[++i] != 0 )
        {
          if ( unlinked_objects[i] == -1 )
            continue;
          if ( !unlinked_objects_found )
            printf("\n** Unlinked Objects (without parent)\n");
          unlinked_objects_found = 1;
          dump_recursive(dbc, unlinked_objects[i], ts, 0, unlinked_objects);
        }

    }

  if ( do_wait && !be_quiet )
    printf("<return>\n");

  if ( do_wait )
    getchar();

  if ( test_obj )
    kogmo_rtdb_obj_delete(dbc, &test_om);

  // don't do kogmo_rtdb_disconnect() in order to test exit handler :-)
  // (but not on mac os-x, because it has no on_exit() and lacks the exit handler :-(
#ifdef MACOSX
  kogmo_rtdb_disconnect(dbc, NULL);
#endif

  return 0;
}

void dump_recursive (kogmo_rtdb_handle_t *db_h,
                     kogmo_rtdb_objid_t parentoid, kogmo_timestamp_t ts,
                     int generation, kogmo_rtdb_objid_list_t olist)
{
  kogmo_rtdb_obj_info_t om;
  kogmo_rtdb_objid_t oid;
  kogmo_rtdb_obj_c3_process_info_t pi;
  kogmo_rtdb_obj_base_t ob;
  kogmo_timestamp_string_t tstring;
  kogmo_rtdb_objsize_t olen;
  int ret,i;

  ret = kogmo_rtdb_obj_readinfo ( db_h, parentoid, ts, &om );
  if ( ret < 0 )
    DIE("error %i accessing metadata",ret);

  for (i=1; i<generation; i++)
    printf("  ");

  kogmo_rtdb_obj_c3_process_getprocessinfo (db_h, om.created_proc, ts, pi);
  printf("+--  %s%s: OID %lli, TYPE 0x%llX, OWN %s, ",
          om.deleted_ts ? "D!":"", // "D!"-prefix means deleted
          om.name, (long long int)om.oid, (long long int)om.otype,
          pi);

  olen = kogmo_rtdb_obj_readdata ( db_h, parentoid, ts, &ob, sizeof(ob) );
  if ( olen < 0 )
    {
      switch(olen)
        {
          case -KOGMO_RTDB_ERR_NOPERM:
            printf("UPDATE unknown (protected)\n");break;
          case -KOGMO_RTDB_ERR_NOTFOUND:
            printf("UPDATE unknown (no data)\n");break;
        }
    }
  else
    {
      kogmo_rtdb_obj_c3_process_getprocessinfo (db_h, ob.base.committed_proc, ts, pi);
      kogmo_timestamp_to_string(ob.base.committed_ts,tstring);
      printf("UPDATE %s by %s\n", tstring, pi);
    }

  // mark object as linked (has parent)
  i=-1;
  while ( olist[++i] != 0 )
    {
      if ( olist[i] == parentoid )
        {
          olist[i] = -1;
          break;
        }
    }

  i=0;
  while( ( oid = kogmo_rtdb_obj_searchinfo ( db_h, "", 0, parentoid, 0, ts,
                                             NULL, ++i ) ) > 0 )
    {
      dump_recursive (db_h, oid, ts, generation + 1, olist);
    }
}
