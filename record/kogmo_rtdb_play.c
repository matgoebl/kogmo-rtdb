/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */
/*! \file kogmo_rtdb_play.c
 * \brief Play RTDB-Streams to the RTDB
 *
 * Copyright (c) 2004-2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */


#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "kogmo_rtdb_internal.h"
#include "kogmo_rtdb_stream.h"
#include "kogmo_rtdb_avirawcodec.h"
#include "kogmo_rtdb_version.h"

#define DIEonERR(value) if (value<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-value);exit(1);}

#define WARNonERR(value) if (value<0) { \
 fprintf(stderr,"warning(l%i):%i.\n",__LINE__,-value);}

#undef DIE
#define DIE(msg...) do { \
 fprintf(stderr,"%i DIED in %s line %i: ",getpid(),__FILE__,__LINE__); fprintf(stderr,msg); fprintf(stderr,"\n"); exit(1); } while (0)

#undef ERR
#define ERR(msg...) do { \
 fprintf(stderr,"ERROR(l%i): ",__LINE__); fprintf(stderr,msg); fprintf(stderr,"\n"); } while (0)

#undef DBG
#define DBG(msg...) do { \
 fprintf(stderr,msg); fprintf(stderr,"\n"); } while (0)

#include "kogmo_rtdb_timeidx.h"

// Number of objects the player can handle at once
#define KOGMO_RTDB_OIDMAPSLOTS (KOGMO_RTDB_OBJIDLIST_MAX)
#include "kogmo_rtdb_oid_map.h"

// Maximum amount of -t/-n/-T/-N command line parameters
#define MAXOPTLIST 50

// Adjust Player Speed with this Frequenzy
#define PLAYHZ 1000

// Support old Player-Command-Object (a misused kogmo_rtdb_obj_c3_sixdof_t)
//#define OBSOLETE_COMMAND_OBJECT
//#define CALCULATE_FPS

// Must be >= sizeof(kogmo_rtdb_subobj_base_t)+sizeof(kogmo_rtdb_subobj_a2_image_t)
#define PREBUFSZ ((int)(sizeof(kogmo_rtdb_subobj_base_t)+sizeof(kogmo_rtdb_subobj_a2_image_t)))
// We use the chunk buffer also for the initial header, not only for objects
#define MINBUFSZ ((int)(sizeof(avirawheader_t)-PREBUFSZ))

// Initial Object buffer size (will be extended when required - but this takes time!)
#define INITBUFSZ ( getenv("KOGMO_RTDB_PLAY_INITBUFSZ") ? strtol ( getenv ("KOGMO_RTDB_PLAY_INITBUFSZ"), NULL, 0) : 1*1024*1024 )




void
usage (void)
{
  fprintf(stderr,
"KogMo-RTDB Player (Rev.%d) (c) Matthias Goebl <mg*tum.de> RCS-TUM\n"
#ifdef NODB
"THIS IS THE STANDALONE-VERSION THAT DOES NOT CONNECT TO A KOGMO-RTDB!\n"
#endif
"Usage: kogmo_rtdb_play [.....]\n"
" -i FILE  read recorded data from file (default: stdin)\n"
" -I FILE  read/write index from/to file (appended to input if leading dot, .idx)\n"
#ifdef CALCULATE_FPS
" -P       calculate inter-frame times (=1/FPS)\n"
#endif
" -l       log all object inserts/updates/deletes to stdout\n"
" -v       verbose logging (-v -v: very verbose/debugging)\n"
#ifndef NODB
" -p       start in pause mode (starts playing when command object gets modified)\n"
" -w       wait for key at end of file\n"
" -s SPEED play with relative speed SPEED (1.0 = normal, 0.5 = half speed)\n"
#endif
" -L       loop, play file again and again\n"
" -K       keep created objects while looping (default: delete and create again)\n"
" -S DATE  start at this date and time (e.g. '-S 2007-01-31 20:13:08.702276000')\n"
" -E DATE  stop at this date and time (e.g. '-E 2007-01-31 20:14')\n"
" -T TID   do not play objects with type TID\n"
" -N NAME  do not play objects with name NAME\n"
" -t TID   play only objects with type TID\n"
" -n NAME  play only objects with name NAME\n"
"NOTE: By default all objects will be played, but if you use -t and/or -n\n"
"      the default changes to no objects.\n"
"      -t/-n/-T/-N can be given up to %d times each.\n"
" -o FILE  save played data to file, implies -D\n"
" -x       extract every metadata and data block to (many!) single files\n"
" -X CMD   pipe extracted (meta)data to command CMD (overrides -x)\n"
"          format (repeating): 'TIME [+.] SIZE TID NAME\\nSizeBytesOfData'\n"
#ifndef NODB
" -D       do not connect to RTDB, useful to analyze stream with -l\n"
"          -x/-X also imply -D (will be extended in the future)\n"
#endif
" -h       print this help message\n"
"If you stop the player, the rtdb will free the allocated memory 10 seconds\n"
"later! If you start and stop the player rapidly, you will get errors and\n"
"you have to wait 10 seconds.\n"
"\n"
"Log output (-l) format description:\n"
"DATE TIME.NANOSECONDS X OBJECTID OBJECTNAME OBJECTTYPE BYTES\n"
"X can be:\n"
" +  object created (metadata)\n"
" -  object deleted (metadata)\n"
" .  object updated\n"
" ,  object updated, first part: header (size is given as current/total)\n"
" ;  object updated, second part: the real avi stream\n"
" #  object metadata refresh (mostly at the beginning of the stream: the\n"
"      object already existed at the time the recorder was started)\n"
" %%  object created/refreshed in data file, but ignored in this playback\n"
"    (filtered out or error)\n"
" !  object update in data file, but ignored in this playback\n"
"Lines beginning with # give additional information or error messages.\n"
"\n"
"Example: kogmo_rtdb_play -i shoreline1_pos4.avi -L -K -S '2007-06-06 04:05:43.195542000' -E '2007-06-06 04:05:46.693025000' -s 0.1\n"
"\n",KOGMO_RTDB_REV,MAXOPTLIST);
  exit(1);
}

FILE *write_extract_pipe_fp=NULL;
void
write_extract(kogmo_timestamp_t ts, char *name, kogmo_rtdb_objtype_t otype, void *data, int size, int is_meta)
{
  int ret;
  if ( write_extract_pipe_fp )
    {
      // time [+.] size type name\n
      ret = fprintf(write_extract_pipe_fp,"%020lli %s %i 0x%X %s\n",
              (long long int)ts, is_meta ? "+" : ".", size, otype, name);
      if(ret<0)
        DIE("cannot write extracted data header to output pipe: %s",strerror(ret));
      ret = fwrite (data, size, 1, write_extract_pipe_fp);
      if(ret!=1)
        DIE("cannot write extracted data to output pipe: %s",strerror(ferror(write_extract_pipe_fp)));
    }
  else
    {
      FILE *fpx;
      char filename[500];
      snprintf (filename, sizeof(filename),
                 "%020lli.%s.0x%X.%s",
                 (long long int)ts, name, otype, is_meta ? "meta" : "data");
      fpx = fopen (filename, "w");
      if ( fpx==NULL ) DIE("cannot open file '%s' for extracting data",filename);
      ret = fwrite (data, size, 1, fpx);
      if(ret!=1)
        DIE("cannot write extracted data to output file '%s': %s",filename,strerror(ferror(fpx)));
      fclose (fpx);
    }
}


int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_objid_t oid,tmpoid,destoid,playeroid=0;
  #ifdef OBSOLETE_COMMAND_OBJECT
  kogmo_rtdb_obj_info_t cmdobj_info;
  kogmo_rtdb_obj_c3_sixdof_t cmdobj;
  kogmo_timestamp_t last_cmdobj_ts=0;
  #endif //OBSOLETE_COMMAND_OBJECT
  kogmo_timestamp_t last_ctrlobj_ts=0;
  kogmo_rtdb_obj_info_t ctrlobj_info,statobj_info;
  kogmo_rtdb_obj_c3_playerctrl_t ctrlobj;
  kogmo_rtdb_obj_c3_playerstat_t statobj;
  kogmo_rtdb_obj_slot_t ctrlobj_objslot;
  kogmo_rtdb_objsize_t olen;
  kogmo_timestamp_t do_goto=0, last_do_goto=0;

  // chunk buffer
  riffchunk_t dc;
  char listfcc[4];
  unsigned char pre_buf[PREBUFSZ];
  unsigned char *base_buf=NULL, *buf=NULL;
  int base_bufsz=INITBUFSZ;
  struct kogmo_rtdb_stream_chunk_t *rtdbchunk = NULL;
  kogmo_rtdb_obj_info_t *info_p = NULL;
  kogmo_rtdb_subobj_base_t *base_p = NULL;

  int ret, size, n, err;
  off_t filepos;

  #define FRAME_GO_INDEX_MAX (60*60*33)
  kogmo_rtdb_objid_t frame_oid=0;
  off_t frameidx_pos[FRAME_GO_INDEX_MAX];

  int do_log=0, do_verbose=0, do_wait=0, do_pause=0, do_scan=0;
#ifdef CALCULATE_FPS
  int do_fps=0;
#endif
  int do_loop=0, do_keepcreated=0, do_extract=0;
  int do_tid=0, do_name=0, do_xtid=0, do_xname=0;
  double do_speed=1.0;
  kogmo_timestamp_t do_begin=0, do_end=0, initial_ts=0;
  char *do_input=NULL, *do_output=NULL, *do_index=NULL, *do_extract_pipe=NULL;
  kogmo_rtdb_objtype_t   tid_list[MAXOPTLIST], xtid_list[MAXOPTLIST];
  char                 *name_list[MAXOPTLIST], *xname_list[MAXOPTLIST];
  int opt;
  FILE *fp=stdin, *fout=NULL;
  off_t skip_pos=0;
  int playit, default_playit = 1;
  int i;

#ifdef NODB
#define do_db 0
  do_speed=0;
#else
  int do_db=1;
  kogmo_rtdb_require_revision(KOGMO_RTDB_REV);
#endif

  for(i=0;i<MAXOPTLIST;i++)
    {
      tid_list[i]=0;
      xtid_list[i]=0;
      name_list[i]=NULL;
      xname_list[i]=NULL;
    }

  while( ( opt = getopt (argc, argv, "lvi:I:o:DxX:Pwps:S:E:t:n:T:N:LKh") ) != -1 )
    switch(opt)
      {
        case 'l': do_log = 1; break;
        case 'v': do_verbose++; timeidx_debug=1; break;
        case 'i': do_input = optarg; break;
        case 'I': do_index = optarg; break;
#ifdef NODB
        case 'o': do_output = optarg; break;
        case 'x': do_extract = 1; break;
        case 'X': do_extract = 1; do_extract_pipe = optarg; break;
#else
        case 'o': do_output = optarg; do_db = 0; break;
        case 'x': do_extract = 1; do_db = 0; break;
        case 'X': do_extract = 1; do_db = 0; do_extract_pipe = optarg; break;
        case 'D': do_db = 0; break;
#endif
#ifdef CALCULATE_FPS
        case 'P': do_fps = 1; break;
#endif
        case 'w': do_wait = 1; break;
        case 'p': do_pause = 1; break;
        case 's': do_speed = atof(optarg); break;
        case 'L': do_loop = 1; break;
        case 'K': do_keepcreated = 1; break;
        case 'S': do_begin = kogmo_timestamp_from_string(optarg); break;
        case 'E': do_end = kogmo_timestamp_from_string(optarg); break;
        case 't': if (++do_tid>MAXOPTLIST) DIE("ERROR: at maximum %d -%c items are allowed!",MAXOPTLIST,opt);
                  tid_list[do_tid-1] = strtol(optarg, (char **)NULL, 0); default_playit = 0; break;
        case 'n': if (++do_name>MAXOPTLIST) DIE("ERROR: at maximum %d -%c items are allowed!",MAXOPTLIST,opt);
                  name_list[do_name-1] = optarg; default_playit = 0; break;
        case 'T': if (++do_xtid>MAXOPTLIST) DIE("ERROR: at maximum %d -%c items are allowed!",MAXOPTLIST,opt);
                  xtid_list[do_xtid-1] = strtol(optarg, (char **)NULL, 0); break;
        case 'N': if (++do_xname>MAXOPTLIST) DIE("ERROR: at maximum %d -%c items are allowed!",MAXOPTLIST,opt);
                  xname_list[do_xname-1] = optarg; break;
        case 'h':
        default: usage(); break;
      }

  if ( base_bufsz < MINBUFSZ ) base_bufsz = MINBUFSZ;
  base_bufsz += PREBUFSZ;
  base_buf = malloc ( base_bufsz );
  buf = base_buf+PREBUFSZ;
  if ( base_buf == NULL )
    DIE("cannot allocate %i bytes of initial chunk buffer", base_bufsz);

  if (do_index != NULL && do_input != NULL && do_index[0]=='.')
    {
      char *filename = malloc ( strlen(do_input) + strlen(do_index) + 1 );
      if (filename == NULL)
        DIE("cannot allocate memory for index filename buffer");
      strcpy(filename,do_input);
      strcat(filename,do_index);
      do_index = filename;
    }
  timeidx_init (do_index);

  if ( do_extract_pipe )
    {
      write_extract_pipe_fp = popen (do_extract_pipe, "w");
      if ( write_extract_pipe_fp==NULL ) DIE("cannot open pipe to command '%s' for data extraction: %s",do_output,strerror(errno));
    }

  if ( do_output )
    {
      fout = fopen (do_output, "w");
      if ( fout==NULL ) DIE("cannot open output file '%s'",do_output);
    }

  if ( do_db )
    { 
      // Verbindung zur Datenbank aufbauen, unsere Zykluszeit ist 33 ms
      err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "kogmo_rtdb_play", 0.033); DIEonERR(err);
      dbinfo.flags = KOGMO_RTDB_CONNECT_FLAGS_IMMEDIATELYDELETE;
      oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);
      playeroid = kogmo_rtdb_obj_c3_process_searchprocessobj (dbc, 0, oid); DIEonERR(playeroid);

      #ifdef OBSOLETE_COMMAND_OBJECT
      // OBSOLETES Object fuer Kommandos erstellen, initialisieren und initiale Werte eintragen
      err = kogmo_rtdb_obj_initinfo (dbc, &cmdobj_info,
        "kogmo_rtdb_play_cmd", KOGMO_RTDB_OBJTYPE_C3_SIXDOF, sizeof (cmdobj)); DIEonERR(err);
      cmdobj_info.flags.write_allow=1; // damit andere Prozesse ein neues Kommando schreiben koennen
      cmdobj_info.parent_oid = playeroid;
      oid = kogmo_rtdb_obj_insert (dbc, &cmdobj_info); DIEonERR(oid);
      err = kogmo_rtdb_obj_initdata (dbc, &cmdobj_info, &cmdobj); DIEonERR(err);
      cmdobj.sixdof.x = do_pause;
      cmdobj.sixdof.y = do_speed;
      err = kogmo_rtdb_obj_writedata (dbc, cmdobj_info.oid, &cmdobj); DIEonERR(err);
      last_cmdobj_ts = cmdobj.base.data_ts;
      #endif //OBSOLETE_COMMAND_OBJECT

      // Object fuer Kommandos erstellen, initialisieren und initiale Werte eintragen
      err = kogmo_rtdb_obj_initinfo (dbc, &ctrlobj_info,
        "playerctrl", KOGMO_RTDB_OBJTYPE_C3_PLAYERCTRL, sizeof (ctrlobj)); DIEonERR(err);
      ctrlobj_info.flags.write_allow=1; // damit andere Prozesse ein neues Kommando schreiben koennen
      ctrlobj_info.parent_oid = playeroid;
      oid = kogmo_rtdb_obj_insert (dbc, &ctrlobj_info); DIEonERR(oid);
      err = kogmo_rtdb_obj_initdata (dbc, &ctrlobj_info, &ctrlobj); DIEonERR(err);
      ctrlobj.playerctrl.flags.log   = do_log;
      ctrlobj.playerctrl.flags.pause = do_pause;
      ctrlobj.playerctrl.flags.scan  = do_scan;
      ctrlobj.playerctrl.flags.loop  = do_loop;
      ctrlobj.playerctrl.flags.keepcreated = do_keepcreated;
      ctrlobj.playerctrl.speed       = do_speed;
      ctrlobj.playerctrl.begin_ts    = do_begin;
      ctrlobj.playerctrl.end_ts      = do_end;
      ctrlobj.playerctrl.goto_ts     = 0;
      ctrlobj.playerctrl.frame_go    = 0;
      err = kogmo_rtdb_obj_writedata (dbc, ctrlobj_info.oid, &ctrlobj); DIEonERR(err);
      last_ctrlobj_ts = ctrlobj.base.data_ts;
      olen = kogmo_rtdb_obj_readdataslot_init (dbc, &ctrlobj_objslot, ctrlobj_info.oid); DIEonERR(olen);


      // Object fuer Status erstellen, initialisieren und initiale Werte eintragen
      err = kogmo_rtdb_obj_initinfo (dbc, &statobj_info,
        "playerstat", KOGMO_RTDB_OBJTYPE_C3_PLAYERSTAT, sizeof (statobj)); DIEonERR(err);
      statobj_info.flags.no_notifies=1; // nonblocking and syscall-free. this is updated so often, that there is no reason for notifies
      statobj_info.parent_oid = playeroid;
      oid = kogmo_rtdb_obj_insert (dbc, &statobj_info); DIEonERR(oid);
      err = kogmo_rtdb_obj_initdata (dbc, &statobj_info, &statobj); DIEonERR(err);
      statobj.playerstat.current_ts = timeidx_get_first();
      statobj.playerstat.first_ts = timeidx_get_first();
      statobj.playerstat.last_ts = timeidx_get_last();
      err = kogmo_rtdb_obj_writedata (dbc, statobj_info.oid, &statobj); DIEonERR(err);

      // playeroid = cmdobj_info.oid;
    }

  map_init();

  do
    {
      kogmo_timestamp_string_t timestring;
      kogmo_timestamp_t last_real_time=0,last_avi_time=0,last_time_diff=0;
#ifdef CALCULATE_FPS
      kogmo_timestamp_t last_fps_time=0,fps_time;
      int fps_n=0;
      kogmo_timestamp_t ts, last_ts=0;
      kogmo_timestamp_t ts_stream[10]={0,0,0,0,0,0,0,0,0,0};
#endif
      int next_prebuf_size=0;
      kogmo_rtdb_objid_t rawnext_oid=0, rawnext_frame_go=0;
      kogmo_timestamp_t rawnext_ts=0;
      int init_phase=1;
      int frameidx_last=-1, frame_go=0;

      if ( do_input )
        {
          fp = fopen (do_input, "r");
          if ( fp==NULL ) DIE("cannot open input file '%s'",do_input);
        }


      if ( do_begin && !do_goto )
        do_goto = do_begin;

      if ( do_goto && do_loop && do_keepcreated )
        {
          if (do_verbose)
            printf("%% INITIAL GOTO: %lli\n",(long long int)do_goto);
          if ( skip_pos && last_do_goto == do_goto ) // wir haben eine exakte ruecksprungstelle
            fseeko(fp,skip_pos,SEEK_SET);
          else
            {
              filepos = timeidx_lookup ( do_goto );
              if ( filepos >= 0 ) // wir haben zumindest eine ungefaehre sprungstelle
                fseeko(fp,filepos,SEEK_SET);
            }
        }

      while ( ! feof(fp) )
        {
          if ( do_db && !init_phase && rtdbchunk != NULL ) // Check Player Command Object
            {
              olen = kogmo_rtdb_obj_readdata (dbc, ctrlobj_info.oid, 0, &ctrlobj, sizeof(ctrlobj));
              //olen = kogmo_rtdb_obj_readdataslot_next (dbc, &ctrlobj_objslot, +1, &ctrlobj, sizeof(ctrlobj));
              if ( olen != (int)sizeof(ctrlobj) )
                {
                  ERR("cannot read my player command object (%lli < %lli)",(long long int)olen,(long long int)sizeof(ctrlobj));
                  last_ctrlobj_ts = ctrlobj.base.data_ts;
                }
              else if ( ctrlobj.base.data_ts != last_ctrlobj_ts )
                {
                  if (do_verbose)
                    printf("%% CTRL-OBJECT CHANGED (%lli)\n", (long long int)ctrlobj.base.data_ts);
                  do_log   = ctrlobj.playerctrl.flags.log;
                  do_loop  = ctrlobj.playerctrl.flags.loop;
                  do_keepcreated = ctrlobj.playerctrl.flags.keepcreated;
                  if ( do_speed != ctrlobj.playerctrl.speed )
                    last_real_time = 0; // reset time-keeping, otherwise a 100 -> 0.01 speed change causes a long wait
                  do_speed = ctrlobj.playerctrl.speed;
                  if ( ctrlobj.playerctrl.begin_ts != do_begin )
                    skip_pos = 0;
                  do_begin = ctrlobj.playerctrl.begin_ts;
                  do_end   = ctrlobj.playerctrl.end_ts;
                  do_pause = ctrlobj.playerctrl.flags.pause;
                  do_scan = ctrlobj.playerctrl.flags.scan;
                  last_ctrlobj_ts = ctrlobj.base.data_ts;
                  do_goto = 0; // stop goto with pause etc..
                  if ( ctrlobj.playerctrl.goto_ts )
                    {
                      do_goto = ctrlobj.playerctrl.goto_ts;
                      filepos = timeidx_lookup ( do_goto );
                      if (do_verbose)    
                        printf("%% CTRL GOTO: %lli (pos: %lli)\n",(long long int)do_goto, (long long int)filepos);
                      if ( do_goto < rtdbchunk->ts ) // Zurueck
                        fseeko ( fp, filepos >= 0 ? filepos : 0, SEEK_SET); // jump to known position or start from the beginning
                      if ( do_goto > rtdbchunk->ts ) // Vorwaerts
                        if ( filepos >= 0 && !do_scan)
                          fseeko(fp,filepos,SEEK_SET); // jump to known position if not scanning
                      frame_go = 0;
                      frameidx_last = -1;
                      last_real_time = 0;
                    }
                  frame_go = 0; // stop frame-go with pause etc..
                  rawnext_frame_go = 0;
                  if ( ctrlobj.playerctrl.frame_go != 0 ) // Objektweises weiter/zurueck
                    {
                      oid = kogmo_rtdb_obj_searchinfo ( dbc, ctrlobj.playerctrl.frame_objname, 0,0,0, 0, NULL, 1);
                      if ( oid > 0 )
                        {
                          if ( oid != frame_oid )
                            frameidx_last = -1;
                          frame_oid = oid;
                          if ( ctrlobj.playerctrl.frame_go < 0 ) // backwards
                            {
                              i = frameidx_last + ctrlobj.playerctrl.frame_go - 1;
                              if ( i >=0 && i < FRAME_GO_INDEX_MAX )
                                {
                                  fseeko(fp,frameidx_pos[i],SEEK_SET);
                                  frameidx_last = i-1;
                                  frame_go = 2;
                                }
                            }
                          else // forward
                            {
                              frame_go = ctrlobj.playerctrl.frame_go;
                            }
                          last_real_time = 0;
                        }
                    }
                }

              #ifdef OBSOLETE_COMMAND_OBJECT
              olen = kogmo_rtdb_obj_readdata (dbc, cmdobj_info.oid, 0, &cmdobj, sizeof(cmdobj));
              if ( olen != (int)sizeof(cmdobj) )
                {
                  ERR("cannot read my player control object (%lli < %lli)",(long long int)olen,(long long int)sizeof(cmdobj));
                  last_cmdobj_ts = cmdobj.base.data_ts;
                }
              else if ( cmdobj.base.data_ts != last_cmdobj_ts) // achtung: die rtdb-zeit kann im simmode springen!
                {
                  do_pause = cmdobj.sixdof.x;
                  do_speed = cmdobj.sixdof.y;
                  last_cmdobj_ts = cmdobj.base.data_ts;
                }
              #endif //OBSOLETE_COMMAND_OBJECT

            }


          if ( do_pause && !do_goto && !init_phase && !(do_goto && do_scan) && !frame_go )
            {
              usleep(50000); // achtung: die rtdb-zeit kann im simmode stehen!! daher hier ein simples sleep (player wird nicht im rt-mode betrieben)
              continue;
            }


          // READING AVI CHUNK: FIRST READ ONLY HEADER

          ret = aviraw_fget_chunkheader(fp,&dc);
          if (feof(fp)) continue;
          if (ret!=0) DIE("cannot read data chunk header");


          // SKIP/COPY GLOBAL AVI HEADERS

          if ( memcmp(dc.fcc,"LIST",4) == 0 || memcmp(dc.fcc,"RIFF",4) == 0 )
            {
              if ( do_output )
                {
                  aviraw_fput_data(fout, &dc, 8);
                }

              size = dc.cb;
              dc.cb = 4;
              ret = aviraw_fget_chunkdata(fp, &dc, listfcc, 4);
              if (ret<0)
                DIE("error reading list header data");

              if ( do_output )
                {
                  aviraw_fput_data(fout, &listfcc, 4);
                }

              if (do_verbose>=2)
                printf("%% '%c%c%c%c' ('%c%c%c%c') %u bytes\n", dc.fcc[0], dc.fcc[1], dc.fcc[2], dc.fcc[3],
                     listfcc[0], listfcc[1], listfcc[2], listfcc[3], size);

              if ( memcmp(dc.fcc,"RIFF",4) == 0 && memcmp(listfcc,"AVI ",4) == 0 )
                  continue;

              if ( memcmp(dc.fcc,"LIST",4) == 0 && memcmp(listfcc,"movi",4) == 0 )
                  continue;

              // LIST 'hdrl'
              dc.cb = size - 4;
              if ( dc.cb > base_bufsz )
                DIE("cannot read avi header - buffer too small (%i < %i)",base_bufsz,dc.cb);
              ret = aviraw_fget_chunkdata(fp, &dc, base_buf, base_bufsz);
              // write a copy of whatever we got - may be incomplete
              if ( do_output )
                {
                  aviraw_fput_data(fout, base_buf, dc.cb);
                }
              if (ret<0)
                DIE("error reading avi header list data");
              continue;
            }

          if (do_verbose>=2)
            printf("%% '%c%c%c%c' %u bytes\n", dc.fcc[0], dc.fcc[1], dc.fcc[2], dc.fcc[3], dc.cb);


          // READ FULL CHUNK

          if ( dc.cb > base_bufsz-PREBUFSZ )
            {
              unsigned char *new_base_buf = NULL;
              int new_base_bufsz = dc.cb;
              if (do_log)
                printf("# chunk buffer (%i bytes) too small, automatically increasing it to %i bytes. try setting environment KOGMO_RTDB_PLAY_INITBUFSZ=%i\n",(int)(base_bufsz-PREBUFSZ),new_base_bufsz,new_base_bufsz);
              new_base_bufsz += PREBUFSZ;
              new_base_buf = realloc ( base_buf, new_base_bufsz );
              if ( new_base_buf == NULL )
                DIE("no more memory for automatic increasement of chunk buffer from %i to %i bytes", base_bufsz, new_base_bufsz);
              base_bufsz = new_base_bufsz;
              base_buf=new_base_buf;
              buf = base_buf+PREBUFSZ;
            }
          size = aviraw_fget_chunkdata(fp, &dc, buf, base_bufsz-PREBUFSZ);
          if ( size < 0 )
            {
            //xDIE("error reading chunk data");
              if (do_log)
                printf("# error reading chunk from file (file damaged or truncated?)\n");
              break; // and run next loop if desired
            }


          if ( memcmp(dc.fcc,"RTDB",4) == 0 ) // THIS IS A RTDB CHUNK
            {
//static kogmo_timestamp_t lts = 0;
              rtdbchunk=(struct kogmo_rtdb_stream_chunk_t*)(buf-8);
//printf("%lli\n",rtdbchunk->ts - lts); lts = rtdbchunk->ts;
              kogmo_timestamp_to_string(rtdbchunk->ts, timestring);
              if (do_verbose>=2)
                printf("%% time: %lli oid: %lli event: %i\n",(long long int)rtdbchunk->ts,
                     (long long int)rtdbchunk->oid, rtdbchunk->type );
              // info/base begins right after the rtdbchunk-header
              info_p=(kogmo_rtdb_obj_info_t*)(buf-8+sizeof(struct kogmo_rtdb_stream_chunk_t));
              base_p=(kogmo_rtdb_subobj_base_t*)(buf-8+sizeof(struct kogmo_rtdb_stream_chunk_t));
              size -= sizeof(struct kogmo_rtdb_stream_chunk_t)-8;

              // The end of the initialization phase is reached as soon as the timestamps begin to increase
              // Beware: There can be a gap between the initial_ts and the "normal" timestamps, caused by cutting the file
              if ( init_phase && rtdbchunk->type != KOGMO_RTDB_STREAM_TYPE_ERROR )
                {
                  if ( init_phase == 1 )
                    initial_ts = rtdbchunk->ts;
                  init_phase = 2;
                  if ( rtdbchunk->ts != initial_ts )
                    {
                      timeidx_add ( rtdbchunk->ts, 0 );
                      init_phase = 0;
                    }
                }

              // Populate Time-to-Position-Index
              if ( !init_phase && rtdbchunk->type != KOGMO_RTDB_STREAM_TYPE_ERROR )
                if ( rtdbchunk->ts >= timeidx_next_needed() )
                  timeidx_add ( rtdbchunk->ts, ftello(fp) );

              // Vorgegebene Geschwindigkeit einhalten
              if (rtdbchunk->type != KOGMO_RTDB_STREAM_TYPE_ERROR &&
                  rtdbchunk->type != KOGMO_RTDB_STREAM_TYPE_RFROBJ &&
                  do_speed && (!do_goto || (do_goto && do_scan) ) && do_db && !init_phase)
                {
                  if (last_real_time)
                    {
                      kogmo_timestamp_t next_avi_time = rtdbchunk->ts;
                      kogmo_timestamp_t real_time_diff = ( next_avi_time - last_avi_time ) / do_speed;
                      if ( real_time_diff - last_time_diff > KOGMO_TIMESTAMP_TICKSPERSECOND/PLAYHZ)
                        {
                          kogmo_timestamp_t sleep_time, next_real_time;
                          sleep_time = real_time_diff - last_time_diff;
                          //kogmo_rtdb_sleep_until(dbc, next_real_time);
                          if (do_verbose>=2)
                            printf("speed adaption: avi:%lli real:%lli -> delay %.9f s\n", (long long int)real_time_diff, (long long int)last_time_diff,(double)sleep_time/KOGMO_TIMESTAMP_TICKSPERSECOND);
                          usleep( sleep_time / (KOGMO_TIMESTAMP_TICKSPERSECOND/1000000) ); // achtung: die rtdb-zeit kann im simmode stehen!! daher hier ein simples sleep (player wird nicht im rt-mode betrieben)
                          next_real_time = last_real_time + real_time_diff;
                          last_real_time = kogmo_timestamp_now();
                          last_time_diff = last_real_time - next_real_time; // das haben wir "zu viel" gewartet, "guthaben"
                          if ( last_time_diff < 0 || last_time_diff > 1*KOGMO_TIMESTAMP_TICKSPERSECOND )
                            last_time_diff = 0; // max diff: 0..1 sec (wg. evtl ctrl-s/q gedrueckt -> sonst versucht der player, die zeit aufzuholen)
                          last_avi_time = rtdbchunk->ts;
                        }
                    }
                  else
                    {
                      last_real_time = kogmo_timestamp_now();
                      last_avi_time = rtdbchunk->ts;
                      last_time_diff = 0;
                    }
                }
              else
                {
                  last_real_time = 0;
                }

              // Schon ueber das Ende hinaus: Abbruch und Rueckfallen in Neustart-Schleife
              if ( do_end && rtdbchunk->ts > do_end )
                {
                  if (do_log)
                    printf("# loop end (-E reached)\n");
                  break;
                }

              // Wenn Zielposition noch nicht erreicht: Suche vorwaerts
              if ( do_goto && !do_scan && !init_phase && rtdbchunk->ts < do_goto )
                {
                  if (do_log && do_verbose)
                    printf("# skip: %f...                        \r",kogmo_timestamp_diff_secs(rtdbchunk->ts,do_goto));
                  skip_pos = ftello(fp) - dc.cb - 8;
                  continue;
                }

              // Status-Objekt updaten
              if ( do_db && rtdbchunk->type != KOGMO_RTDB_STREAM_TYPE_ERROR )
                {
                  kogmo_rtdb_timestamp_set(dbc,rtdbchunk->ts);
                  statobj.playerstat.current_ts = rtdbchunk->ts;
                  if ( !statobj.playerstat.first_ts && !init_phase )
                    statobj.playerstat.first_ts = rtdbchunk->ts;
                  if ( statobj.playerstat.last_ts < rtdbchunk->ts )
                    statobj.playerstat.last_ts = rtdbchunk->ts;
                  err = kogmo_rtdb_obj_writedata (dbc, statobj_info.oid, &statobj); DIEonERR(err);
                }

              // Wenn Zielposition erreicht: Suche beenden
              if ( do_goto && rtdbchunk->ts >= do_goto )
                {
                  skip_pos = ftello(fp) - dc.cb - 8;
                  last_do_goto = do_goto;
                  do_goto = 0;
                  if ( do_pause && !do_scan)
                    continue;
                }

              // Ab hier: normale Wiedergabe

              switch(rtdbchunk->type)
                {
                  case KOGMO_RTDB_STREAM_TYPE_ADDOBJ:
                  case KOGMO_RTDB_STREAM_TYPE_RFROBJ:
                  case KOGMO_RTDB_STREAM_TYPE_CHGOBJ: // TODO! kogmo_rtdb_obj_changeinfo()
                    playit = default_playit;
                    for(i=0;i<MAXOPTLIST;i++)
                      {
                        if ( tid_list[i] && tid_list[i] == info_p->otype ) playit++;
                        if ( name_list[i] && strncmp(name_list[i],info_p->name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 ) playit++;
                      }
                    for(i=0;i<MAXOPTLIST;i++)
                      {
                        if ( xtid_list[i] && xtid_list[i] == info_p->otype ) playit = 0;
                        if ( xname_list[i] && strncmp(xname_list[i],info_p->name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN)==0 ) playit = 0;
                      }
                    if (do_log)
                      {
                        char info = '+';
                        if ( rtdbchunk->type == KOGMO_RTDB_STREAM_TYPE_RFROBJ )
                          info = '#';
                        if ( !playit )
                          info = '%';
                        printf("%s %c %lli '%s'\n",timestring,info,(long long int)info_p->oid, info_p->name);
                      }
                    if ( do_output && playit )
                      {
                        aviraw_fput_chunk(fout, &dc, buf);
                      }
                    oid = info_p -> oid;
                    if ( map_exists(oid) )
                      {
                        // bei einen RFR refresh ist es erlaubt, dass das objekt schon existiert, bei einen ADD nicht
                        if (playit && do_log && rtdbchunk->type == KOGMO_RTDB_STREAM_TYPE_ADDOBJ)
                          printf("#ERROR: oid %lli already exists!\n",(long long int)info_p->oid);
                        // wenn diese src-oid schon existiert, kann es sein, dass wir loop-en mit keep-objects
                        break;
                      }
                    if ( !playit )
                      {
                        map_add (oid,0,info_p->name,info_p->otype);
                        break;
                      }
                    if ( do_extract )
                      {
                        write_extract (rtdbchunk->ts, info_p->name, info_p->otype, info_p, sizeof(kogmo_rtdb_obj_info_t), 1);
                      }
                    if ( !do_db )
                      {
                        map_add (oid,1,info_p->name,info_p->otype);
                        break;
                      }
                    info_p -> oid = 0;
                    tmpoid = map_querydest(info_p->parent_oid);
                    if ( tmpoid )
                      info_p -> parent_oid = tmpoid;
                    else
                      // Nicht sehr schoen, ab das sinnvollste: Default-Parent ist Player
                      info_p -> parent_oid = playeroid;
                    info_p -> flags.cycle_watch = 0; // sonst geht mehrfache Abspielgeschwindigkeit nicht
                    destoid = kogmo_rtdb_obj_insert (dbc, info_p);
                    if(destoid==-KOGMO_RTDB_ERR_NOMEMORY)
                      {
                        printf("#ERROR: out of database memory for object '%s'!\n",info_p->name);
                        // printf("# (did you start/stop the player rapidly? memory will be freed 10 seconds later, so let's wait 10 seconds..)\n");
                        // sleep(12);
                        // destoid = kogmo_rtdb_obj_insert (dbc, info_p);
                      }
                    if(destoid==-KOGMO_RTDB_ERR_NOTUNIQ)
                      {
                        if (do_log)
                          printf("#ERROR: unique objecttype 0x%X already exists, cannot create (and update) a second object '%s'!\n",info_p->otype,info_p->name);
                          map_add (oid,0,info_p->name,info_p->otype);
                        break;
                      }
                    WARNonERR(destoid);
                    if ( destoid < 0 )
                      destoid = 0;
                    map_add (oid,destoid,info_p->name,info_p->otype);
                    break;
                  case KOGMO_RTDB_STREAM_TYPE_DELOBJ:
                    if (do_log)
                      printf("%s - %lli '%s'\n",timestring,(long long int)info_p->oid, info_p->name);
                    oid = info_p->oid;
                    destoid = map_querydest(oid);
                    map_del (oid);
                    if ( destoid == 0 )
                      break; // can be 0 if there was an error (was not unique) or it was filtered
                    if ( do_output )
                      {
                        aviraw_fput_chunk(fout, &dc, buf);
                      }
                    if ( !do_db )
                        break;
                    info_p -> oid = destoid;
                    if ( destoid != 0 ) // can be 0 if there was an error (was not unique) or it was filtered
                      {
                        err = kogmo_rtdb_obj_delete (dbc, info_p); WARNonERR(err);
                      }
                    map_del (oid);
                    break;
                  case KOGMO_RTDB_STREAM_TYPE_UPDOBJ:
                    oid = map_querydest (rtdbchunk->oid);
                    if ( !oid )
                      break;
                    if (do_log)
                      printf("%s %c %lli '%s' 0x%X %i/%i\n",timestring,
                             oid ? '.' : '!',
                             (long long int)rtdbchunk->oid,
                             map_queryname(rtdbchunk->oid),
                             map_queryotype(rtdbchunk->oid),
                             size, base_p->size);
                    if ( do_output )
                      {
                        aviraw_fput_chunk(fout, &dc, buf);
                      }
                    if ( do_extract )
                      {
                        write_extract (rtdbchunk->ts, map_queryname(rtdbchunk->oid), map_queryotype(rtdbchunk->oid), base_p, base_p->size, 0);
                      }
                    if ( oid == frame_oid )
                      {
                        if ( frameidx_last < FRAME_GO_INDEX_MAX-1 )
                          frameidx_pos[++frameidx_last] = ftello(fp) - dc.cb - 8;
                        if ( frame_go > 0 )
                          frame_go--;
                      }
                    if ( !do_db )
                        break;
                    err = kogmo_rtdb_obj_writedata (dbc, oid, base_p);
                    if ( err == -KOGMO_RTDB_ERR_NOPERM || err == -KOGMO_RTDB_ERR_NOTFOUND)
                      break;
                    WARNonERR(err);
                    break;
                  case KOGMO_RTDB_STREAM_TYPE_UPDOBJ_NEXT:
                    oid = map_querydest (rtdbchunk->oid);
                    if ( !oid )
                      break;
                    if (do_log)
                      printf("%s , %lli '%s' 0x%X %i/%i\n", timestring,
                             (long long int)rtdbchunk->oid,
                             map_queryname(rtdbchunk->oid),
                             map_queryotype(rtdbchunk->oid),
                             size, base_p->size);
                    if( size > PREBUFSZ )
                      DIE("prebuf %i too small (need %i)", PREBUFSZ, size);
                    rawnext_oid = rtdbchunk->oid;
                    rawnext_ts = rtdbchunk->ts;
                    memcpy(pre_buf,base_p,size);
                    next_prebuf_size = size;
                    if ( do_output )
                      {
                        aviraw_fput_chunk(fout, &dc, buf);
                      }
                    if ( oid == frame_oid )
                      {
                        //DBGprintf("frame %i. to go %i\n",frameidx_last+1,frame_go);
                        if ( frameidx_last < FRAME_GO_INDEX_MAX-1 )
                          frameidx_pos[++frameidx_last] = ftello(fp) - dc.cb - 8;
                        if ( frame_go > 0 )
                          rawnext_frame_go=1;
                      }
#ifdef CALCULATE_FPS
                    last_ts = base_p->committed_ts; //rtdbchunk->ts;
                    if (do_log && do_verbose)
                      printf("%% image header with %i bytes\n",next_prebuf_size);
#endif
                    break;
                  case KOGMO_RTDB_STREAM_TYPE_ERROR:
                    if (do_log)
                      printf("%s ERROR: LOST DATA\n",timestring);
                    if ( do_output )
                      {
                        aviraw_fput_chunk(fout, &dc, buf);
                      }
                    break;
                  default :
                    if (do_log)
                      printf("%s ERROR: UNKNOWN EVENT\n",timestring);
                }
              continue;
            }

          if ( dc.fcc[0] == '0' && dc.fcc[1] >= '0' && dc.fcc[1] <= '9' && dc.fcc[2] == 'd' && dc.fcc[3] == 'b' )
            {
              if ( rawnext_frame_go && frame_go > 0)
                {
                  rawnext_frame_go=0;
                  frame_go--;
                }
              if ( !rawnext_oid )
                continue;
              n=dc.fcc[1]-'0';
#ifdef CALCULATE_FPS
              if (do_verbose)
                printf("%% image %i\n",n);
              ts = last_ts - ts_stream[n];
              if ( ts_stream[n] && last_ts)
                if (do_log && do_verbose)
                  printf("& image stream %i committed time delta %lli\n", n, (long long int)ts);
              ts_stream[n] = last_ts;
#endif

              base_p=(void*)buf-next_prebuf_size;
              memcpy(base_p,pre_buf,next_prebuf_size);
              if (do_log)
                printf("%s ; %lli '%s' 0x%X %i/%i\n", timestring,
                       (long long int)rawnext_oid,
                       map_queryname(rawnext_oid),
                       map_queryotype(rawnext_oid),
                       size, base_p->size);
#ifdef CALCULATE_FPS
              if (do_fps)
                {
                  fps_time = kogmo_timestamp_now();
                  if (last_fps_time && ++fps_n>=30)
                    {
                      double fps_timediff;
                      fps_timediff = kogmo_timestamp_diff_secs(last_fps_time, fps_time);
                      printf("FPS: %f\n", (double) fps_n / fps_timediff);
                      fps_n=0;
                      last_fps_time = fps_time;
                    }
                  if (!last_fps_time)
                    last_fps_time = fps_time;
                }
#endif
              oid = map_querydest (rawnext_oid);
              if ( oid && do_output )
                {
                  aviraw_fput_chunk(fout, &dc, buf);
                }
              if ( oid && do_extract )
                {
                  write_extract (rawnext_ts, map_queryname(rawnext_oid), map_queryotype(rawnext_oid), base_p, base_p->size, 0);
                }
              rawnext_oid = 0;
              if (!oid)
                continue;
              if ( !do_db )
                continue;
              err = kogmo_rtdb_obj_writedata (dbc, oid, base_p);
              if ( err != -EACCES)
                WARNonERR(err);
              rawnext_oid = 0;
              continue;
            }
        } // while ( ! feof(fp) )


      if (do_verbose)
        printf("end of file.\n");

      if ( do_input )
        {
          fclose (fp);
        }

      if (do_wait)
        {
          printf("#END - press return!\n");
          getchar();
        }

      if ( ( ! ( do_loop && do_keepcreated ) ) && do_db )
        {
          kogmo_rtdb_obj_info_t tmp_objinfo;
          // see oid_map.h:
          for_each_map_entry(oid)
            {
              if (oid)
                {
                  destoid = map_querydest(oid);
                  if (do_log && do_verbose)
                    printf("#END %c %lli '%s' 0x%X \n",
                           destoid ? '-' : '!',
                           (long long int)oid,
                           map_queryname(oid),
                           map_queryotype(oid));
                  if (destoid)
                    {
                      tmp_objinfo.oid = destoid;
                      err = kogmo_rtdb_obj_delete (dbc, &tmp_objinfo);
                      //xWARNonERR(err);
                    }
                  map_del (oid);
                }
             }
          for_each_map_entry_end;
        }
    }
  while(do_loop);

  if ( do_output )
    {
      fclose (fout);
    }

  timeidx_write (do_index);

  if ( do_extract_pipe )
    {
      ret = pclose (write_extract_pipe_fp);
      if ( ret < 0 ) DIE("cannot close pipe to command '%s' for data extraction: %s",do_output,strerror(errno));
    }

  if ( do_db )
    {
      kogmo_rtdb_disconnect(dbc, NULL);
    }
  return 0;
}
