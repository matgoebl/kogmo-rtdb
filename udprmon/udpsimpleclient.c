/*! \file udpsimpleclient.c
 * \brief Simple Client for Objects via UDP/IP - for Debugging and Remote Monitoring - NOT FOR CAR2CAR!!!
 *
 * (c) 2008 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include "udpsimple.h"
int debug;

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_obj_info_t statobj_info;
  kogmo_rtdb_obj_c3_text1k_t statobj;
  int stathdrlen=0;
  char string[100];

  kogmo_rtdb_obj_info_t *rtdbobj_info_p;
  kogmo_rtdb_obj_base_t *rtdbobj_p;

  kogmo_rtdb_objid_list_t *idlist_p;
  kogmo_rtdb_objid_t oid, remoteoid, myoid;
  int i,j, recvok;
  kogmo_rtdb_obj_udpsimplereq_t req;
  kogmo_rtdb_obj_udpsimplerply_t *rply_p;
  int serial = 1;

  char recvbuf[BUFSIZE];
  int bufpos=0;
  #ifdef UDPSIMPLEBZIP2
  char buf[BUFSIZE];
  int bzbuflen;
  unsigned int newbuflen;
  #else
  char *buf=recvbuf;
  #endif

  int err, ret, recvlen, sendlen;
  struct addrinfo cfg,*srv;
  int clifd;
  fd_set rfds;
  struct timeval timeout;

  kogmo_timestamp_t ts0=0,ts=0;
  int tx=0,rx=0,rxobj=0,rxerr=0;
  double dt=0;
  char statustext[1000];

  char *host=NULL,*port=NULL;
  float interval=0;
  #define MAXOBJS 100
  char *objspec[MAXOBJS];
  int objspecs=0;
  char *newname=NULL;
  char infoonly=0;

  memset (&oid_map, 0, sizeof(oid_map));

  int debug = getenv("DEBUG") ? atoi(getenv("DEBUG")) : 0;
  if ( argc != 3 && argc < 5 )
    {
      fprintf (stderr, "Usage: udpsimpleclient HOST UDPPORT INTERVAL [OBJNAME][:[0xOBJTYPE][=NEWNAME]]..\nudpsimpleclient HOST UDPPORT  shows remote object list\n");
      exit(1);
    }
  host=argv[1];
  port=argv[2];

  if ( argc == 3 )
    {
      infoonly = 1;
      objspec[objspecs++]=""; // all objects
    }
  else
    {
      interval=atof(argv[3]);
    }

  // Connect to RTDB
  kogmo_rtdb_connect_initinfo (&dbinfo, "", "udpsimpleclient", interval ? interval : 0.1);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo);
  if ( oid < 0 )
    DIE("ERROR: cannot connect to RTDB");
  myoid = kogmo_rtdb_obj_c3_process_searchprocessobj (dbc, 0, oid); DIEonERR(myoid);

  // Netzwerkverbindung initialisieren
  DBG("Sending UDP-packets to %s:%s..\n", host, port);
  memset ( &cfg, 0, sizeof(struct addrinfo) ); // defaults are 0/NULL
  cfg.ai_family = AF_INET; // AF_INET6 soon..
  cfg.ai_socktype = SOCK_DGRAM; // UDP please
  cfg.ai_protocol = IPPROTO_UDP; // UDP please
  err = getaddrinfo ( host, port, &cfg, &srv); DIEonERR(err);
  clifd = socket ( srv -> ai_addr -> sa_family, srv -> ai_socktype,
                   srv -> ai_protocol); DIEonERR(clifd);
  err = connect ( clifd, srv -> ai_addr, srv -> ai_addrlen ); DIEonERR(err);

  if (!infoonly)
    {
      // Init simple Status-Object
      err = getnameinfo ( srv->ai_addr, sizeof(struct sockaddr), string, sizeof(string), NULL, 0, NI_DGRAM | NI_NUMERICHOST);
      strncat ( string, ":", sizeof(string) );
      err = getnameinfo ( srv->ai_addr, sizeof(struct sockaddr), NULL, 0, &string[strlen(string)], sizeof(string)-strlen(string)-1, NI_DGRAM | NI_NUMERICSERV);
      err = kogmo_rtdb_obj_initinfo (dbc, &statobj_info, string, KOGMO_RTDB_OBJTYPE_C3_TEXT, sizeof(statobj)); DIEonERR(err);
      myoid = kogmo_rtdb_obj_insert (dbc, &statobj_info); DIEonERR(myoid);
      err = kogmo_rtdb_obj_initdata (dbc, &statobj_info, &statobj); DIEonERR(err);

      // remember requests from command line
      snprintf(statobj.text.data, sizeof(statobj.text.data), "Monitored Remote Host: %s:%s/UDP\nRequest Interval: %f\nRequested Objects:", host, port, interval);
      for(i=0;i<MAXOBJS;i++)
        {
          if ( i >= argc-4 )
            break;
          objspec[objspecs++]=argv[4+i];
          strncat(statobj.text.data, " ", sizeof(statobj.text.data));
          strncat(statobj.text.data, objspec[i], sizeof(statobj.text.data));
          DBG("Obj %i: %s",i,objspec[i]);
        }
      strncat(statobj.text.data, "\n", sizeof(statobj.text.data));
      stathdrlen = strlen(statobj.text.data);
    }

  do
    {
      ts = kogmo_timestamp_now();
      dt = kogmo_timestamp_diff_secs(ts0,ts);
      if ( ts0 && !infoonly )
        {
         snprintf(statustext, sizeof(statustext),
                  "TX: %.3f RX: %.3f KBy/s (%i+%i By %i Err in %.3f s) %i Objects\n", (double) tx / dt / 1000, (double) rx / dt / 1000, tx, rx, rxerr, dt, rxobj );
         DBG("%s", statustext);
         if ( getenv("BPS") )
           printf("%s\n", statustext);
         snprintf(&statobj.text.data[stathdrlen], sizeof(statobj.text.data)-stathdrlen,"%s\n", statustext);
         statobj.base.size = sizeof(statobj.base) + strlen(statobj.text.data);
         err = kogmo_rtdb_obj_writedata (dbc, statobj_info.oid, &statobj); DIEonERR(err);
        }
      ts0 = ts; tx=0; rx=0; rxobj=0; rxerr=0;

      usleep(interval*1e6);
      for(i=0;i<objspecs;i++)
        {
          DBG("*** Sending new request for %i. Object %s ***", i, objspec[i]);
          memset(&req,0,sizeof(req));
          memcpy( req.rtdbfcc, "RTDB", 4);
          req.request = serial++;
          req.flags = infoonly ? UDPSIMPLEFLAG_INFO : UDPSIMPLEFLAG_INFO | UDPSIMPLEFLAG_DATA;
          req.flags |= UDPSIMPLEFLAG_LIST;
          #ifdef USEJPEG
          req.flags |= UDPSIMPLEFLAG_JPEGIMAGE;
          #endif
          req.oid = 0;
          strncpy(req.name, objspec[i], strchrnul(objspec[i],':')-objspec[i]);
          req.name[sizeof(req.name)-1]='\0';
          if(strchr(objspec[i],':'))
            req.otype = strtol(strchr(objspec[i],':')+1, (char **)NULL, 0);
          newname = NULL;
          if(strchr(objspec[i],'='))
            newname=strchr(objspec[i],'=')+1;
          DBG("Request Search for %s%s%s:0x%08X parent:%lli proc:%lli ts:%lli nth:%i",
          req.name, newname ? "=" : "", newname ? newname : "",
          req.otype, (long long int)req.parent_oid, (long long int)req.proc_oid, req.ts, req.nth);
          // Transmit
          sendlen = send ( clifd, &req, sizeof(req), 0);
          DBG("Sending packet with %i bytes returned %i", sizeof(req), sendlen);
          tx+=sizeof(req);
          if ( sendlen != sizeof(req) )
            { DBG("send error: %s",strerror(sendlen)); continue; }
          if (interval>=3) usleep(interval*1e6);
          recvok=0;
          do
            {
              // Receive with 1s timeout
              timeout.tv_sec=1; timeout.tv_usec=0; // interval >= 1.0 || interval == 0 ? 999999 : interval*1e6;
              FD_ZERO(&rfds);
              FD_SET(clifd, &rfds);
              ret = select(clifd+1,&rfds,NULL,NULL,&timeout);
              recvlen = recv(clifd, recvbuf, BUFSIZE, MSG_DONTWAIT);
              DBG("Received packet with %i bytes",recvlen);
              #ifdef UDPSIMPLEBZIP2
              if ( recvlen > 0 )
                {
                  bzbuflen = recvlen;
                  newbuflen = BUFSIZE;
                  err = BZ2_bzBuffToBuffDecompress( buf, &newbuflen, recvbuf, bzbuflen, 0, 0);
                  recvlen = newbuflen;
                  if ( err != BZ_OK ) { DBG("BZIP2-Compressing received packet failed with %i",err); continue; }
                  DBG("BZIP2-Decompressed send packet from %i to %i bytes (%.2f%%)",bzbuflen,recvlen,(float)recvlen/bzbuflen*100);
                }
              #endif
              if ( recvlen < 0 )
                break;
              rx+=recvlen;

              bufpos=0;
              while ( bufpos + sizeof(kogmo_rtdb_obj_udpsimplerply_t) < recvlen )
                {
                  // Check Header
                  rply_p=(void*)&buf[bufpos]; bufpos+=sizeof(kogmo_rtdb_obj_udpsimplerply_t);
                  if ( strncmp ( rply_p->rtdbfcc, "RTDB", 4 ) != 0 ) { DBG("received response packet has no RTDB identifier" ); rxerr++; break; }
                  if ( rply_p->reply != req.request ) { DBG("received packet contains unknown reply %i != %i",rply_p->reply, req.request ); rxerr++; break; }
                  if ( req.oid && rply_p->oid != req.oid ) { DBG("received packet contains wrong oid %i != %i",rply_p->oid,req.oid ); rxerr++; break; }

                  if ( rply_p->flags & UDPSIMPLEFLAG_LIST )
                    {
                      if ( bufpos + sizeof(kogmo_rtdb_objid_list_t) >= BUFSIZE )
                        break;
                      idlist_p=(void*)&buf[bufpos]; bufpos+=sizeof(kogmo_rtdb_objid_list_t);
                      DBG("Received Object List:");
                      for(j=0;j<KOGMO_RTDB_OBJIDLIST_MAX;j++)
                        {
                          if ( (*idlist_p)[j] == 0 )
                            break;
                          DBG("- %i",(*idlist_p)[j]);
                        }
                      continue;
                    }

                  // Get Object Info
                  if ( bufpos + sizeof(kogmo_rtdb_obj_info_t) >= BUFSIZE )
                    break;
                  rtdbobj_info_p=(void*)&buf[bufpos]; bufpos+=sizeof(kogmo_rtdb_obj_info_t);
                  DBG("Received Object %s:0x%08X OID:%lli parent:%lli createdproc:%lli maxsize:%lli",
                    rtdbobj_info_p->name, rtdbobj_info_p->otype, (long long int)rtdbobj_info_p->oid,
                    (long long int)rtdbobj_info_p->parent_oid, (long long int)rtdbobj_info_p->created_proc,
                    (long long int)rtdbobj_info_p->size_max);
                  remoteoid = rtdbobj_info_p->oid;
                  rxobj++;
                  if ( infoonly )
                    {
                      printf( "%s:0x%08X#%lli@#%lli;%llib\n",
                              rtdbobj_info_p->name, rtdbobj_info_p->otype, (long long int)rtdbobj_info_p->oid, (long long int)rtdbobj_info_p->parent_oid, (long long int)rtdbobj_info_p->size_max);
                      continue;
                    }
                  oid = oid_map_query( remoteoid );
                  if ( ! oid )
                    {
                      rtdbobj_info_p->parent_oid = oid_map_query(rtdbobj_info_p->parent_oid);
                      if ( ! rtdbobj_info_p->parent_oid )
                        rtdbobj_info_p->parent_oid = myoid;
                      if(newname)
                        strncpy(rtdbobj_info_p->name, newname, sizeof(kogmo_rtdb_objname_t));
                      // fixing the cycle time saves memory on the client side
                      if ( rtdbobj_info_p->avg_cycletime < interval ) rtdbobj_info_p->avg_cycletime = interval;
                      if ( rtdbobj_info_p->max_cycletime < interval ) rtdbobj_info_p->max_cycletime = interval;
                      rtdbobj_info_p->flags.keep_alloc=rtdbobj_info_p->flags.unique=rtdbobj_info_p->flags.cycle_watch=rtdbobj_info_p->flags.persistent=rtdbobj_info_p->flags.parent_delete=0;
                      oid = kogmo_rtdb_obj_insert (dbc, rtdbobj_info_p);
                      DBG("WriteInfo returned %lli", (long long int)oid);
                      if ( oid > 0 )
                        if ( ! oid_map_add(remoteoid,oid) )
                          DIE("ERROR: Remote-OID-Map Overflow. FIXME: delete stale objects!!");
                      // TODO: oid_map_del(..STALE-OBJECTS..)
                    }
                  if ( ! ( rply_p->flags & UDPSIMPLEFLAG_DATA ) )
                    continue;
                  // Get Object Data
                  if ( bufpos + sizeof(kogmo_rtdb_obj_base_t) >= BUFSIZE )
                    break;
                  rtdbobj_p=(void*)&buf[bufpos];
                  if ( bufpos + rtdbobj_p->base.size >= BUFSIZE )
                    break;
                  bufpos+=rtdbobj_p->base.size;
                  if (oid < 0)
                    {
                      DBG("cannot insert object");
                      continue; // jetzt haben wir den data-pointer auf den naechsten block gesetzt
                    }
                  #ifdef USEJPEG
                  if ( rply_p->flags & UDPSIMPLEFLAG_JPEGIMAGE )
                    {
                      DBG("JPEG-Decompressing image from %i bytes", rtdbobj_p->base.size);
                      rtdbobj_p = (kogmo_rtdb_obj_base_t*) jpeg_decompress_a2_image((kogmo_rtdb_obj_a2_image_t*)rtdbobj_p);
                    }
                  #endif
                  err = kogmo_rtdb_obj_writedata (dbc, oid, rtdbobj_p);
                  if (err < 0)
                    {
                      DBG("cannot write object");
                      continue;
                    }
                  DBG("Wrote Object with %i bytes as %s with oid %lli", rtdbobj_p->base.size, rtdbobj_info_p->name, (long long int)oid);
                  if ( bufpos & 1 ) bufpos++;
                  recvok=1; // received correct reply
                }
            }
          while ( recvlen > 0 && !recvok);
        }
      if ( rx == 0 )
        rxerr++;
    }
  while ( interval > 0 );

  return 0;
}
