/*! \file libkogmo_rtdb_udpmon.c
 * \brief Fake Library for Simple Clients that get Objects via UDP/IP - for Debugging and Remote Monitoring - NOT FOR CAR2CAR!!!
 *
 * (c) 2009 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include "udpsimple.h"
#include <pthread.h>
#include <signal.h>

// in usecs, <1 sec!
#define POLLTIME 100000
#define RECVTIMEOUT 500000
#define SENDDELAY 1000

const kogmo_rtdb_objid_t udpmon_fake_oid=0x7ffffff;
inline void udpmon_poll_wait(void) { usleep(POLLTIME); }
int kogmo_rtdb_debug;
int debug;
int kogmo_rtdb_udpmon_req_serial = 0;

kogmo_rtdb_objid_t
kogmo_rtdb_connect (kogmo_rtdb_handle_t **db_h,
                    kogmo_rtdb_connect_info_t *conninfo)
{
  struct addrinfo cfg,*srv;
  int *clifd, err;
  char *host=NULL,*port=NULL, *p;
  char *dbhost = getenv("KOGMO_RTDB_DBHOST") ? getenv("KOGMO_RTDB_DBHOST") : conninfo->dbhost;

  debug = kogmo_rtdb_debug = getenv("KOGMO_RTDB_DEBUG") ? atoi(getenv("KOGMO_RTDB_DEBUG")) : 1023;
  DBG("WARNING! Using libkogmo_rtdb_udpmon to connect to remote KogMo-RTDB!");
  DBG("This is just a simple and very limited minimal implementation to");
  DBG("remotely monitor another rtdb - many function calls won't work. Use this");
  DBG("only for simple single-threaded GUIs. Access is read-only!");
  DBG("(mg)");

  if ( dbhost!=NULL && dbhost[0] != '\0' &&
       strncmp ( dbhost, "udpmon:", 7 ) == 0 )
    {
      host=&dbhost[7];
      p = strrchr ( host, ':' );
      if (p) { p[0]='\0'; port=&p[1]; } // warning: modifies conninfo!
        else { port=UDPMON_PORT_DEFAULT; }
    }
  if ( !host || !port )
    DIE("illegal KOGMO_RTDB_DBHOST specification '%s', only 'udpmon:' implemented in libkogmo_rtdb_udpmon!\nexample: export KOGMO_RTDB_DBHOST=udpmon:192.168.28.1:"UDPMON_PORT_DEFAULT,dbhost);

  clifd = malloc(sizeof(int)); if(!clifd) DIE("out of memory");
  *((int**)db_h) = clifd;

  // Netzwerkverbindung initialisieren
  DBG("Sending UDP-packets to %s:%s..\n", host, port);
  memset ( &cfg, 0, sizeof(struct addrinfo) ); // defaults are 0/NULL
  cfg.ai_family = AF_INET; // AF_INET6 soon..
  cfg.ai_socktype = SOCK_DGRAM; // UDP please
  cfg.ai_protocol = IPPROTO_UDP; // UDP please
  err = getaddrinfo ( host, port, &cfg, &srv); DIEonERR(err);
  *clifd = socket ( srv -> ai_addr -> sa_family, srv -> ai_socktype,
                   srv -> ai_protocol); DIEonERR(*clifd);
  err = connect ( *clifd, srv -> ai_addr, srv -> ai_addrlen ); DIEonERR(err);

  return udpmon_fake_oid;
}


int
kogmo_rtdb_disconnect (kogmo_rtdb_handle_t *db_h,
                       void *discinfo)
{ close (*((int*)db_h)); free(db_h); return 0; }



int
udpmon_request (int clifd, kogmo_rtdb_obj_udpsimplereq_t *req_p,
                kogmo_rtdb_obj_udpsimplerply_t *rply_p,
                void *data_p, int data_size)
{
  int req_size = sizeof(*req_p);
  char sendbuf[BUFSIZE];
  char recvbuf[BUFSIZE];
  #ifdef UDPSIMPLEBZIP2
  char buf[BUFSIZE];
  int bzbuflen;
  unsigned int newbuflen;
  #else
  char *buf=recvbuf;
  #endif

  int ret, recvlen, sendlen, datalen=-1;
  fd_set rfds;
  struct timeval timeout;

  DBG("*** Sending new request with flags 0x%X, Object %i ***", req_p->flags, req_p->oid);
  memcpy( req_p->rtdbfcc, "RTDB", 4);
  req_p->version = kogmo_rtdb_udpmon_protocol_version;
  req_p->request = ++kogmo_rtdb_udpmon_req_serial;
  DBG("Request Search for oid:%lli name:%s tid:%08X parent:%lli proc:%lli ts:%lli nth:%i",
      (long long int)req_p->oid, req_p->name, req_p->otype, (long long int)req_p->parent_oid, (long long int)req_p->proc_oid, req_p->ts, req_p->nth);

  #ifdef USEJPEG
  req_p->flags |= UDPSIMPLEFLAG_JPEGIMAGE;
  #endif

  if ( req_p->flags & UDPSIMPLEFLAG_WRITE )
    {
      // we need to copy the request-header and the data into a continuous packet buffer
      memcpy(sendbuf, req_p, req_size );
      req_p = (kogmo_rtdb_obj_udpsimplereq_t *) sendbuf;
      memcpy(&sendbuf[req_size], data_p, data_size);
      req_size += data_size;
    }

  // Transmit
  sendlen = send ( clifd, req_p, req_size, 0);
  DBG("Sending packet with %i bytes returned %i", req_size, sendlen);
  if ( sendlen != req_size )
    { DBG("send error: %s",strerror(sendlen)); return -1; }
  usleep(SENDDELAY); // inter-request interval

  do
    {
      timeout.tv_sec=0; timeout.tv_usec= RECVTIMEOUT;
      FD_ZERO(&rfds);
      FD_SET(clifd, &rfds);
      ret = select(clifd+1,&rfds,NULL,NULL,&timeout);
      recvlen = recv(clifd, recvbuf, BUFSIZE, MSG_DONTWAIT);
      DBG("Received packet with %i bytes",recvlen);
      if ( recvlen < 0 )
        DBG("Receive error %i",errno);
      #ifdef UDPSIMPLEBZIP2
      if ( recvlen > 0 )
        {
          bzbuflen = recvlen;
          newbuflen = BUFSIZE;
          ret = BZ2_bzBuffToBuffDecompress( buf, &newbuflen, recvbuf, bzbuflen, 0, 0);
          recvlen = newbuflen;
          if ( ret != BZ_OK ) { DBG("BZIP2-Compressing received packet failed with %i",ret); continue; }
          DBG("BZIP2-Decompressed send packet from %i to %i bytes (%.2f%%)",bzbuflen,recvlen,(float)recvlen/bzbuflen*100);
        }
      #endif
      if ( recvlen < 0 )
        return -1;

      if ( recvlen < sizeof(kogmo_rtdb_obj_udpsimplerply_t) )
        continue;

      // Check Header
      memcpy(rply_p, buf, sizeof(kogmo_rtdb_obj_udpsimplerply_t));
      datalen = recvlen - sizeof(kogmo_rtdb_obj_udpsimplerply_t);

      if ( strncmp ( rply_p->rtdbfcc, "RTDB", 4 ) != 0 ) { DBG("received response packet has no RTDB identifier" ); continue; }
      if ( rply_p->version != kogmo_rtdb_udpmon_protocol_version ) { DBG("received request packet has wrong version %i (!= %i)", rply_p->version, kogmo_rtdb_udpmon_protocol_version ); continue; }
      if ( rply_p->reply != req_p->request ) { DBG("received packet contains unknown reply %i != %i",rply_p->reply, req_p->request ); continue; }
      if ( req_p->oid && req_p->oid && rply_p->oid != req_p->oid ) { DBG("received packet contains wrong oid %i != %i",rply_p->oid,req_p->oid ); continue; }
      break;
    }
  while(1);
  DBG("OK received packet contains reply %i = %i",rply_p->reply, req_p->request );

  #ifdef USEJPEG
  if ( rply_p->flags & UDPSIMPLEFLAG_JPEGIMAGE )
    {
      char *data2_p = buf+sizeof(kogmo_rtdb_obj_udpsimplerply_t);
      kogmo_rtdb_obj_a2_image_t *pout;
      DBG("JPEG-Decompressing image from %i bytes", ((kogmo_rtdb_obj_a2_image_t*)data2_p)->base.size);
      pout = jpeg_decompress_a2_image((kogmo_rtdb_obj_a2_image_t*)data2_p);
      datalen = pout->base.size;
      memcpy(data_p, pout, datalen < data_size ? datalen : data_size);
    }
  else
  #endif
    {
      memcpy(data_p, buf+sizeof(kogmo_rtdb_obj_udpsimplerply_t), datalen < data_size ? datalen : data_size );
    } 
  return datalen;
}




kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo (kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_timestamp_t ts,
                           kogmo_rtdb_objid_list_t idlist,
                           int nth)
{
  kogmo_rtdb_obj_udpsimplereq_t req;
  kogmo_rtdb_obj_udpsimplerply_t rply;
  kogmo_rtdb_objid_list_t tmpidlist, *idlist_p;
  int ret,i;

  DBG("API: kogmo_rtdb_obj_searchinfo(%s,0x%X,%i,%i,%lli,%s%i)",name?name:"''",otype,parent_oid,proc_oid,ts,idlist?"list,":"",nth);
  memset(&req,0,sizeof(req));
  req.flags = UDPSIMPLEFLAG_LIST;
  req.name[0]='\0';
  if ( name != NULL )
    strncpy(req.name, name, sizeof(req.name));
  req.otype = otype;
  req.parent_oid = parent_oid;
  req.proc_oid = proc_oid;
  req.ts = ts;
  req.nth = 0; // nth;
  // idlist_p = idlist != NULL ? idlist : tmpidlist;
  idlist_p = &tmpidlist;
  if ( idlist != NULL )
    idlist_p = idlist;

  ret = udpmon_request ( *((int*)db_h), &req, &rply, idlist_p, sizeof(kogmo_rtdb_objid_list_t) );
  if ( ret != sizeof(kogmo_rtdb_objid_list_t) )
    ret = -KOGMO_RTDB_ERR_UNKNOWN; // KOGMO_RTDB_ERR_NOTFOUND;
  else if ( nth > 0 )
    ret = (*idlist_p)[nth-1] > 0 ? (*idlist_p)[nth-1] : -KOGMO_RTDB_ERR_NOTFOUND ;
  else {
    for(i=0;i<KOGMO_RTDB_OBJIDLIST_MAX;i++)
      if ( (*idlist_p)[i] == 0 ) break;
    ret = i;
  }
  DBG("=API: kogmo_rtdb_obj_searchinfo(): %i\n", ret); 
  return ret;
}



int
kogmo_rtdb_obj_readinfo (kogmo_rtdb_handle_t *db_h,
                                kogmo_rtdb_objid_t oid,
                                kogmo_timestamp_t ts,
                                kogmo_rtdb_obj_info_t *metadata_p)
{
  kogmo_rtdb_obj_udpsimplereq_t req;
  kogmo_rtdb_obj_udpsimplerply_t rply;
  int ret;

  DBG("API: kogmo_rtdb_obj_readinfo(%i)",oid);
  memset(&req,0,sizeof(req));
  req.flags = UDPSIMPLEFLAG_INFO;
  req.oid = oid;
  req.ts = ts;

  ret = udpmon_request ( *((int*)db_h), &req, &rply, metadata_p, sizeof(kogmo_rtdb_obj_info_t) );
  if ( ret != sizeof(kogmo_rtdb_obj_info_t) )
    return -KOGMO_RTDB_ERR_NOTFOUND;
  DBG("=API: kogmo_rtdb_obj_readinfo(%i): OK\n",oid);
  return 0;
}



kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                       kogmo_timestamp_t ts,
                       void *data_p, kogmo_rtdb_objsize_t size)
{
  kogmo_rtdb_obj_udpsimplereq_t req;
  kogmo_rtdb_obj_udpsimplerply_t rply;
  int ret;

  DBG("API: kogmo_rtdb_obj_readdata(%i)",oid);
  memset(&req,0,sizeof(req));
  req.flags = UDPSIMPLEFLAG_DATA;
  req.oid = oid;
  req.ts = ts;

  ret = udpmon_request ( *((int*)db_h), &req, &rply, data_p, size);
  DBG("=API: kogmo_rtdb_obj_readdata(%i): %i\n",oid, ret);
  if ( ret > 0 )
    ( (kogmo_rtdb_subobj_base_t*) data_p ) -> size = ret;
  return ret;
}



int
kogmo_rtdb_obj_initdata (kogmo_rtdb_handle_t *db_h,
                         kogmo_rtdb_obj_info_t *metadata_p,
                         void *data_p)
{
  memset (data_p, 0, metadata_p->size_max);
  ( (kogmo_rtdb_subobj_base_t*) data_p ) -> size = metadata_p->size_max;
  return 0;
}



int
kogmo_rtdb_obj_writedata (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_objid_t oid, void *data_p)
{
  kogmo_rtdb_obj_udpsimplereq_t req;
  kogmo_rtdb_obj_udpsimplerply_t rply;
  int ret;

  DBG("API: kogmo_rtdb_obj_writedata(%i)",oid);
  if ( oid == udpmon_fake_oid )
    return 0;
  memset(&req,0,sizeof(req));
  req.flags = UDPSIMPLEFLAG_WRITE | UDPSIMPLEFLAG_DATA;
  req.oid = oid;

  ret = udpmon_request ( *((int*)db_h), &req, &rply, data_p, ( (kogmo_rtdb_subobj_base_t*) data_p ) -> size );
  DBG("=API: kogmo_rtdb_obj_writedata(%i): %i\n",oid, ret);
  return ret < 0 ? ret : 0;
}



kogmo_timestamp_t
kogmo_rtdb_timestamp_now (kogmo_rtdb_handle_t *db_h)
{
/*
  struct timeval tval;
  int err = gettimeofday ( &tval, NULL);
  if (err != 0) return 0;
  return (kogmo_timestamp_t) tval.tv_sec*KOGMO_TIMESTAMP_TICKSPERSECOND
         + tval.tv_usec*1000/KOGMO_TIMESTAMP_NANOSECONDSPERTICK;
*/
  kogmo_rtdb_obj_c3_rtdb_t rtdbobj;
  kogmo_rtdb_objid_t oid;
  kogmo_rtdb_objsize_t olen;
  DBG("API: kogmo_rtdb_timestamp_now()");
  oid = kogmo_rtdb_obj_searchinfo (db_h, "rtdb", KOGMO_RTDB_OBJTYPE_C3_RTDB, 0, 0, 0, NULL, 1);
  if ( oid < 0 ) return 0;  // DIE("cannot find rtdb object");
  olen = kogmo_rtdb_obj_readdata (db_h, oid, 0, &rtdbobj, sizeof(rtdbobj));
  if ( olen < 0 ) return 0;  // DIE("error accessing rtdb data");
  DBG("=API: kogmo_rtdb_timestamp_now(): %lli\n",rtdbobj.base.committed_ts);
  return rtdbobj.base.committed_ts;
}






int
kogmo_rtdb_obj_initinfo (kogmo_rtdb_handle_t *db_h,
                         kogmo_rtdb_obj_info_t *metadata_p,
                         _const char* name, kogmo_rtdb_objtype_t otype,
                         kogmo_rtdb_objsize_t size_max)
{
  memset (metadata_p, 0, sizeof(kogmo_rtdb_obj_info_t));
  strncpy(metadata_p->name,name,sizeof(metadata_p->name));
  metadata_p->otype=otype;
  metadata_p->size_max = size_max;
  return 0;
}


kogmo_rtdb_objid_t
kogmo_rtdb_obj_insert (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *metadata_p)
{
  kogmo_rtdb_obj_udpsimplereq_t req;
  kogmo_rtdb_obj_udpsimplerply_t rply;
  int ret;

  DBG("API: kogmo_rtdb_obj_insert(%s)",metadata_p->name);
  memset(&req,0,sizeof(req));
  req.flags = UDPSIMPLEFLAG_WRITE | UDPSIMPLEFLAG_INFO;

  ret = udpmon_request ( *((int*)db_h), &req, &rply, metadata_p, sizeof(kogmo_rtdb_obj_info_t) );
  DBG("=API: kogmo_rtdb_obj_insert(%s): %i\n", metadata_p->name, ret);
  if ( ret < 0 )
    return ret;
  metadata_p->oid = rply.oid;  // udpmon_fake_oid;
  return metadata_p->oid;
}


int
kogmo_rtdb_obj_delete (kogmo_rtdb_handle_t *db_h,
                       kogmo_rtdb_obj_info_t *metadata_p)
{
  kogmo_rtdb_obj_udpsimplereq_t req;
  kogmo_rtdb_obj_udpsimplerply_t rply;
  int ret;

  DBG("API: kogmo_rtdb_obj_delete(%i)",metadata_p->oid);
  memset(&req,0,sizeof(req));
  req.flags = UDPSIMPLEFLAG_WRITE | UDPSIMPLEFLAG_INFO;
  req.oid = metadata_p->oid;
  metadata_p->deleted_ts = kogmo_rtdb_timestamp_now (db_h);

  ret = udpmon_request ( *((int*)db_h), &req, &rply, metadata_p, sizeof(kogmo_rtdb_obj_info_t) );
  DBG("=API: kogmo_rtdb_obj_delete(%i): %i\n", metadata_p->oid, ret);
  if ( ret < 0 )
    return ret;
  metadata_p->oid = 0;
  return 0;
}



kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo_wait(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid)
{
  kogmo_rtdb_objid_t oret=-1;
  while (oret < 0 )
    {
      udpmon_poll_wait();
      oret = kogmo_rtdb_obj_searchinfo (db_h, name, otype, parent_oid, proc_oid, 0, NULL, 1);
    }
  return oret;
}


int
kogmo_rtdb_obj_searchinfo_waitnext(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_rtdb_objid_list_t known_idlist,
                           kogmo_rtdb_objid_list_t added_idlist,
                           kogmo_rtdb_objid_list_t deleted_idlist)
{ return -KOGMO_RTDB_ERR_INVALID; }


/* fall through..
char *unimplemented="(unimplemented)\n";
char *
kogmo_rtdb_obj_dumpinfo_str (kogmo_rtdb_handle_t *db_h,
                                  kogmo_rtdb_obj_info_t *metadata_p)
{ return unimplemented; }
*/



kogmo_rtdb_objsize_t
kogmo_rtdb_obj_writedata_ptr_begin (kogmo_rtdb_handle_t *db_h,
                                    kogmo_rtdb_objid_t oid,
                                    void *data_pp)
{ return -KOGMO_RTDB_ERR_INVALID; }


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_writedata_ptr_commit (kogmo_rtdb_handle_t *db_h,
                                     kogmo_rtdb_objid_t oid,
                                     void *data_pp)
{ return -KOGMO_RTDB_ERR_INVALID; }




kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_ptr (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                       kogmo_timestamp_t ts, void *data_pp)
{ return -KOGMO_RTDB_ERR_INVALID; }


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_older (kogmo_rtdb_handle_t *db_h,
                             kogmo_rtdb_objid_t oid,
                             kogmo_timestamp_t ts,
                             void *data_p, kogmo_rtdb_objsize_t size)
{ return kogmo_rtdb_obj_readdata ( db_h, oid, ts-KOGMO_TIMESTAMP_TICKSPERSECOND, data_p, size ); }


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_younger (kogmo_rtdb_handle_t *db_h,
                               kogmo_rtdb_objid_t oid,
                               kogmo_timestamp_t ts,
                               void *data_p, kogmo_rtdb_objsize_t size)
{ return kogmo_rtdb_obj_readdata ( db_h, oid, ts+KOGMO_TIMESTAMP_TICKSPERSECOND, data_p, size ); }


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_datatime (kogmo_rtdb_handle_t *db_h, kogmo_rtdb_objid_t oid,
                       kogmo_timestamp_t ts,
                       void *data_p, kogmo_rtdb_objsize_t size)
{ return kogmo_rtdb_obj_readdata ( db_h, oid, ts, data_p, size ); }


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_dataolder (kogmo_rtdb_handle_t *db_h,
                             kogmo_rtdb_objid_t oid,
                             kogmo_timestamp_t ts,
                             void *data_p, kogmo_rtdb_objsize_t size)
{ return kogmo_rtdb_obj_readdata ( db_h, oid, ts-KOGMO_TIMESTAMP_TICKSPERSECOND, data_p, size ); }


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_datayounger (kogmo_rtdb_handle_t *db_h,
                               kogmo_rtdb_objid_t oid,
                               kogmo_timestamp_t ts,
                               void *data_p, kogmo_rtdb_objsize_t size)
{ return kogmo_rtdb_obj_readdata ( db_h, oid, ts+KOGMO_TIMESTAMP_TICKSPERSECOND, data_p, size ); }


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext (kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_p, kogmo_rtdb_objsize_t size)
{
  kogmo_rtdb_objsize_t oret;
  do
    {
      udpmon_poll_wait();
      oret = kogmo_rtdb_obj_readdata ( db_h, oid, 0, data_p, size );
      if ( oret < 0 ) return oret;
    }
  while ( old_ts >= ( (kogmo_rtdb_obj_base_t*) data_p ) -> base.committed_ts );
  return oret;
}


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext_ptr (kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_p, kogmo_rtdb_objsize_t size)
{ return -KOGMO_RTDB_ERR_INVALID; }


#define HAVE_wait_until
kogmo_rtdb_objid_t
kogmo_rtdb_obj_searchinfo_wait_until(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_timestamp_t wakeup_ts)
{ udpmon_poll_wait(); return kogmo_rtdb_obj_searchinfo (db_h, name, otype, parent_oid, proc_oid, 0, NULL, 1); }


int
kogmo_rtdb_obj_searchinfo_waitnext_until(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_rtdb_objid_list_t known_idlist,
                           kogmo_rtdb_objid_list_t added_idlist,
                           kogmo_rtdb_objid_list_t deleted_idlist,
                           kogmo_timestamp_t wakeup_ts)
{ return -KOGMO_RTDB_ERR_INVALID; }


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext_until(
                            kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_p, kogmo_rtdb_objsize_t size,
                            kogmo_timestamp_t wakeup_ts)
{ return kogmo_rtdb_obj_readdata_waitnext (db_h, oid, old_ts, data_p, size); }


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdata_waitnext_until_ptr(
                            kogmo_rtdb_handle_t *db_h,
                            kogmo_rtdb_objid_t oid, kogmo_timestamp_t old_ts,
                            void *data_p, kogmo_rtdb_objsize_t size,
                            kogmo_timestamp_t wakeup_ts)
{ return -KOGMO_RTDB_ERR_INVALID; }




int
kogmo_rtdb_timestamp_set (kogmo_rtdb_handle_t *db_h,
                          kogmo_timestamp_t new_ts)
{ return -KOGMO_RTDB_ERR_NOPERM; }


int
kogmo_rtdb_connect_initinfo (kogmo_rtdb_connect_info_t *conninfo,
                             _const char *dbhost,
                             _const char *procname, float cycletime)
{ return 0; }

int  
kogmo_rtdb_setstatus (kogmo_rtdb_handle_t *db_h, uint32_t status, _const char* msg, uint32_t flags)
{ return 0; }
#define HAVE_DEFINED_setstatus
int  
kogmo_rtdb_cycle_done (kogmo_rtdb_handle_t *db_h, uint32_t flags)
{ return 0; }        

int
kogmo_rtdb_pthread_create(kogmo_rtdb_handle_t *db_h,
                          pthread_t *thread,
                          pthread_attr_t *attr,
                          void *(*start_routine)(void*), void *arg)
{ return pthread_create(thread, attr, start_routine, arg); }

int
kogmo_rtdb_pthread_kill(kogmo_rtdb_handle_t *db_h,
                        pthread_t thread, int sig)
{ return pthread_kill(thread,sig); }

int
kogmo_rtdb_pthread_join(kogmo_rtdb_handle_t *db_h,
                        pthread_t thread, void **value_ptr)
{ return pthread_join(thread, value_ptr); }


int
kogmo_rtdb_sleep_until(kogmo_rtdb_handle_t *db_h,
                       _const kogmo_timestamp_t wakeup_ts)
{ udpmon_poll_wait(); return 0; }


int
kogmo_rtdb_obj_searchinfo_lists_nonblocking(kogmo_rtdb_handle_t *db_h,
                           _const char *name,
                           kogmo_rtdb_objtype_t otype,
                           kogmo_rtdb_objid_t parent_oid,
                           kogmo_rtdb_objid_t proc_oid,
                           kogmo_rtdb_objid_list_t known_idlist,
                           kogmo_rtdb_objid_list_t added_idlist,
                           kogmo_rtdb_objid_list_t deleted_idlist)
{ return -KOGMO_RTDB_ERR_INVALID; }


kogmo_rtdb_objsize_t
kogmo_rtdb_obj_readdataslot_ptr (kogmo_rtdb_handle_t *db_h,
                                 int32_t mode, int32_t offset,  
                                 kogmo_rtdb_obj_slot_t *objslot,
                                 void *data_pp)
{ return -KOGMO_RTDB_ERR_INVALID; }
