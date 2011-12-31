/*! \file udpsimpleserver.c
 * \brief Simple Server to access remote Objects via UDP/IP - for Debugging and Monitoring - NOT FOR CAR2CAR!!!
 *
 * (c) 2008 Matthias Goebl <mg@tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include "udpsimple.h"
int debug;

// comparison function for qsort to sort oids ascending
static int compare_oid(const void *a, const void *b) {
     kogmo_rtdb_objid_t * oid_a = (kogmo_rtdb_objid_t *) a;
     kogmo_rtdb_objid_t * oid_b = (kogmo_rtdb_objid_t *) b;
     if ( *oid_a < *oid_b ) return -1;
     if ( *oid_a > *oid_b ) return 1;
     return 0;
}

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;

  kogmo_rtdb_obj_info_t *rtdbobj_info_p, rtdbobj_info;
  kogmo_rtdb_obj_base_t *rtdbobj_p;

  kogmo_rtdb_objid_t oid, listoids;
  kogmo_rtdb_objid_list_t idlist;
  kogmo_rtdb_objsize_t osize;
  int i;

  char recv_buf[BUFSIZE];
  int recv_buf_size=sizeof(recv_buf);
  kogmo_rtdb_obj_udpsimplereq_t *req_p = (kogmo_rtdb_obj_udpsimplereq_t*) recv_buf;
  int req_size=sizeof(kogmo_rtdb_obj_udpsimplereq_t);
  int recvlen, recvidx;

  kogmo_rtdb_obj_udpsimplerply_t *rply_p;

  char buf[BUFSIZE];
  int buflen=0,last_buflen=0;
  #ifdef UDPSIMPLEBZIP2
  char bzbuf[BUFSIZE*2+600];
  unsigned int bzbuflen;
  #endif
  int err, sendlen;
  struct addrinfo cfg,*srv;
  struct sockaddr clientaddr;
  socklen_t clientaddrlen;
  int srvfd;
  char *port=UDPMON_PORT_DEFAULT;

  #define CLIENTINFOSIZE 300
  char clientinfo[CLIENTINFOSIZE];

  int debug = getenv("DEBUG") ? atoi(getenv("DEBUG")) : 0;
  if ( argc == 2 && strcmp("-k",argv[1])==0 )
    {
      system("killall udpsimpleserver");
      exit(0);
    }

  if ( argc == 2 && strcmp("-l",argv[1])==0 )
    {
      system("echo UDP Ports used by other Programs or Users:;lsof -n|grep 'IPv4.*UDP.*:'");
      exit(0);
    }

  if ( argc > 2 || ( argc == 2 && strcmp("-h",argv[1])==0 ) )
    {
      fprintf (stderr, "Usage: udpsimpleserver [UDPPORT(default:"UDPMON_PORT_DEFAULT")|-k|-l]\n");
      exit(1);
    }

  if ( argc >= 2 )
    port=argv[1];

  // Connect to RTDB
  kogmo_rtdb_connect_initinfo (&dbinfo, "", "udpsimpleserver", 1.0);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo);
  if ( oid < 0 )
    DIE("ERROR: cannot connect to RTDB");
  
  // Netzwerkverbindung initialisieren
  DBG("Listening for UDP-packets at port %s..", port);
  memset ( &cfg, 0, sizeof(struct addrinfo) ); // defaults are 0/NULL
  cfg.ai_flags = AI_PASSIVE; // for servers
  cfg.ai_family = AF_INET; // AF_INET6 soon..
  cfg.ai_socktype = SOCK_DGRAM; // UDP please
  cfg.ai_protocol = IPPROTO_UDP; // UDP please
  err = getaddrinfo ( NULL, port, &cfg, &srv); DIEonERR(err);
  srvfd = socket ( srv -> ai_addr -> sa_family, srv -> ai_socktype,
                 srv -> ai_protocol); DIEonERR(srvfd);
  // i = IP_PMTUDISC_DONT; setsockopt(srvfd, IPPROTO_IP, IP_MTU_DISCOVER, &i, sizeof(i));
  err = bind ( srvfd, srv -> ai_addr, srv -> ai_addrlen); DIEonERR(err);

  while (1)
    {
      DBG("*** Waiting for new request.. ***");
      clientaddrlen = sizeof ( clientaddr );
      recvlen = recvfrom(srvfd, req_p, recv_buf_size, 0, &clientaddr, &clientaddrlen);
      if ( recvlen < 0 ) { DBG("receive error: %s",strerror(recvlen)); continue; }

      i=0;
      clientinfo[i]='\0';
      err = getnameinfo ( &clientaddr, clientaddrlen,
                          &clientinfo[strlen(clientinfo)], CLIENTINFOSIZE-strlen(clientinfo)-1,
                          NULL, 0, NI_DGRAM | NI_NUMERICHOST);
      strncat ( &clientinfo[strlen(clientinfo)], ":", CLIENTINFOSIZE-strlen(clientinfo)-1 );
      err = getnameinfo ( &clientaddr, clientaddrlen, NULL, 0,
                          &clientinfo[strlen(clientinfo)], CLIENTINFOSIZE-strlen(clientinfo)-1,
                          NI_DGRAM | NI_NUMERICSERV);
      DBG("Received packet from %s with %i bytes",clientinfo,recvlen);

      if ( recvlen < req_size ) { DBG("received request packet has wrong length %i (<%i)",recvlen,req_size ); continue; }

      buflen=0;
      if ( strncmp ( req_p->rtdbfcc, "RTDB", 4 ) != 0 ) { DBG("received request packet has no RTDB identifier" ); continue; }
      if ( req_p->version != kogmo_rtdb_udpmon_protocol_version ) { DBG("received request packet has wrong version %i (!= %i)", req_p->version, kogmo_rtdb_udpmon_protocol_version ); continue; }
      // if ( req_p->request != 1 ) { DBG("received packet contains unknown request %i",req_p->request ); continue; }

      DBG("Request: flags 0x%X, oid %i, req %i", req_p->flags, req_p->oid, req_p->request);
      req_p->name[sizeof(req_p->name)-1]='\0';
      if ( req_p->oid > 0 || ( req_p->flags & UDPSIMPLEFLAG_WRITE && req_p->flags & UDPSIMPLEFLAG_INFO ) )
        {
          DBG("Selected oid %lli", (long long int)req_p->oid);
          idlist[0] = req_p->oid; idlist[1] = 0; listoids = 1;
        }
      else
        {
          listoids = kogmo_rtdb_obj_searchinfo (dbc, req_p->name, req_p->otype, req_p->parent_oid, req_p->proc_oid, req_p->ts, idlist, req_p->nth);
          DBG("Search for %s:0x%08X parent:%lli proc:%lli ts:%lli nth:%i returned %lli",
              req_p->name, req_p->otype, (long long int)req_p->parent_oid, (long long int)req_p->proc_oid, req_p->ts, req_p->nth, (long long int)listoids);
        }
      if ( req_p->flags & UDPSIMPLEFLAG_LIST && ! ( req_p->flags & UDPSIMPLEFLAG_WRITE ) )
        {
          rply_p=(void*)&buf[buflen]; buflen+=sizeof(kogmo_rtdb_obj_udpsimplerply_t);
          memcpy( rply_p->rtdbfcc, "RTDB", 4);
          rply_p->version = kogmo_rtdb_udpmon_protocol_version;
          rply_p->reply = req_p->request;
          rply_p->flags = UDPSIMPLEFLAG_LIST;
          rply_p->oid = listoids;
          for(i=listoids;i<KOGMO_RTDB_OBJIDLIST_MAX;i++)
            idlist[i]=0; // null list for better compression
          if ( buflen + sizeof(idlist) >= BUFSIZE )
                break;
          rply_p=(void*)&buf[buflen]; buflen+=sizeof(idlist);
          memcpy(rply_p,&idlist,sizeof(idlist));
        }
      if (listoids<=0)
        { // Error:
          rply_p=(void*)&buf[buflen]; buflen+=sizeof(kogmo_rtdb_obj_udpsimplerply_t);
          memcpy( rply_p->rtdbfcc, "RTDB", 4);
          rply_p->version = kogmo_rtdb_udpmon_protocol_version;
          rply_p->reply = req_p->request;
          rply_p->flags = 0;
          rply_p->oid = listoids;
        }
      else if ( ( req_p->flags & UDPSIMPLEFLAG_INFO || req_p->flags & UDPSIMPLEFLAG_DATA ) && ! ( req_p->flags & UDPSIMPLEFLAG_WRITE ) )
        { // Read-Request:
          last_buflen = buflen;
          // sort oids numerically ascending so is it guaranteed that a parent objects is created first
          qsort(&idlist[0], listoids, sizeof(kogmo_rtdb_objid_t), compare_oid);

          for(i=0;i<listoids;i++)
            {
              // Write Reply Header
              if ( buflen + sizeof(kogmo_rtdb_obj_udpsimplerply_t) >= BUFSIZE )
                break;
              rply_p=(void*)&buf[buflen]; buflen+=sizeof(kogmo_rtdb_obj_udpsimplerply_t);
              memcpy( rply_p->rtdbfcc, "RTDB", 4);
              rply_p->version = kogmo_rtdb_udpmon_protocol_version;
              rply_p->reply = req_p->request;
              rply_p->flags = 0;
              rply_p->oid = idlist[i];

              if ( req_p->flags & UDPSIMPLEFLAG_INFO )
                {
                  // Write Object Info
                  if ( buflen + sizeof(kogmo_rtdb_obj_info_t) >= BUFSIZE )
                    break;
                  rtdbobj_info_p=(void*)&buf[buflen]; buflen+=sizeof(kogmo_rtdb_obj_info_t);              
                  rply_p->flags |= UDPSIMPLEFLAG_INFO;
                }
              else
                {
                  // Get Object Info
                  if ( buflen + sizeof(kogmo_rtdb_obj_info_t) >= BUFSIZE )
                    break;
                  rtdbobj_info_p = &rtdbobj_info;
                } 

              err = kogmo_rtdb_obj_readinfo (dbc, idlist[i], req_p->ts, rtdbobj_info_p);
              DBG("%i. ReadInfo for OID %lli returned %i", i, (long long int)idlist[i], err);
              if (err < 0)
                break;

              if ( req_p->flags & UDPSIMPLEFLAG_DATA )
                {
                  // Write Object Data
                  if ( buflen + rtdbobj_info_p->size_max >= BUFSIZE )
                    break;
                  rtdbobj_p=(void*)&buf[buflen];

                  osize = kogmo_rtdb_obj_readdata (dbc, idlist[i], req_p->ts, rtdbobj_p, rtdbobj_info_p->size_max);
                  DBG("%i. ReadData for OID %lli returned %lli (bytes)",
                      i, (long long int)idlist[i], (long long int)osize);
                  if (osize > 0)
                    {
                      rply_p->flags |= UDPSIMPLEFLAG_DATA;
                      // Horrible Hack to reduce image size in automotive applications, where the top and bottom of the image are less important
                      if ( rtdbobj_p->base.size > 20000 && rtdbobj_info_p->otype == KOGMO_RTDB_OBJTYPE_A2_IMAGE && ((kogmo_rtdb_obj_a2_image_t*)rtdbobj_p)->image.colormodell != A2_IMAGE_COLORMODEL_RGBJPEG )
                        {
                          #ifdef USEJPEG
                          if ( req_p->flags & UDPSIMPLEFLAG_JPEGIMAGE )
                            {
                              rply_p->flags |= UDPSIMPLEFLAG_JPEGIMAGE;
                              buflen += jpeg_compress_a2_image((kogmo_rtdb_obj_a2_image_t*)rtdbobj_p);
                              DBG("%i. JPEG-Compressed image to %i bytes", i, rtdbobj_p->base.size);
                            }
                          else
                          #endif
                            buflen += reduce_a2_image ((kogmo_rtdb_obj_a2_image_t*)rtdbobj_p);
                        }
                      else
                        buflen += rtdbobj_p->base.size;
                    }
                }
              if ( buflen & 1 ) buflen++;
              last_buflen = buflen;
            }
          buflen = last_buflen;
        }
      else if ( req_p->flags & UDPSIMPLEFLAG_WRITE )
        { // Write Request:
          recvidx = req_size;
          if ( req_p->flags & UDPSIMPLEFLAG_INFO )
            {
              rtdbobj_info_p=(void*)&recv_buf[recvidx];
              if ( recvlen - recvidx < (int)sizeof(kogmo_rtdb_obj_info_t) )
                {
                  DBG("error: write request has %i bytes total, object info needs %i, but only %i bytes available", recvlen, sizeof(kogmo_rtdb_obj_info_t), recvlen - recvidx );
                  continue;
                }
              recvidx += sizeof(kogmo_rtdb_obj_info_t);

              if ( rtdbobj_info_p->deleted_ts != 0 )
                oid = kogmo_rtdb_obj_delete (dbc, rtdbobj_info_p);
              else
                oid = kogmo_rtdb_obj_insert (dbc, rtdbobj_info_p);
              DBG("WriteInfo for %s object '%s' returned OID %lli",
                  rtdbobj_info_p->deleted_ts ? "deleted" : "new", rtdbobj_info_p->name, (long long int)oid);
            }
          else
            {
              oid = idlist[0];
            }
          if ( req_p->flags & UDPSIMPLEFLAG_DATA && oid > 0 )
            {
              rtdbobj_p=(void*)&recv_buf[recvidx];
              if ( rtdbobj_p->base.size != recvlen - recvidx )
                {
                  DBG("error: write request has %i bytes total, given data length is %i, but %i bytes available", recvlen, rtdbobj_p->base.size, recvlen - recvidx );
                  continue;
                }

              err = kogmo_rtdb_obj_writedata (dbc, idlist[0], rtdbobj_p);
              DBG("WriteData for OID %lli returned %lli",
                  (long long int)idlist[0], (long long int)err);
              if ( err < 0 )
                oid = err;
            }
          rply_p=(void*)&buf[buflen]; buflen+=sizeof(kogmo_rtdb_obj_udpsimplerply_t);
          memcpy( rply_p->rtdbfcc, "RTDB", 4);
          rply_p->version = kogmo_rtdb_udpmon_protocol_version;
          rply_p->reply = req_p->request;
          rply_p->flags = 0;
          rply_p->oid = oid;
          DBG("Reply: flags 0x%X, oid %i, rply %i", rply_p->flags, rply_p->oid, rply_p->reply);
        }

      // Transmit Reply
      #ifdef UDPSIMPLEBZIP2
      bzbuflen = sizeof(bzbuf);
      err = BZ2_bzBuffToBuffCompress( bzbuf, &bzbuflen, buf, buflen, 1, 0, 0);
      if ( err != BZ_OK ) { DBG("BZIP2-Compressing send packet failed with %i",err); continue; }
      DBG("BZIP2-Compressed send packet from %i to %i bytes (%.2f%%)",buflen,bzbuflen,(float)bzbuflen/buflen*100);
      sendlen = sendto ( srvfd, bzbuf, bzbuflen, 0, &clientaddr, clientaddrlen);
      DBG("Sending packet to %s with %i bytes returned %i",clientinfo,bzbuflen,sendlen);
      #else
      sendlen = sendto ( srvfd, buf, buflen, 0, &clientaddr, clientaddrlen);
      DBG("Sending packet to %s with %i bytes returned %i",clientinfo,buflen,sendlen);
      #endif
    }

  return 0;
}
