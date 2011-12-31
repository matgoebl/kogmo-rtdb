/*! \file udpsimple.h
 * \brief Helpers for the Simple Client & Server for Objects via UDP/IP
 *
 * (c) 2008 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> // printf
#include <sys/socket.h> // addrinfo
#include <netdb.h> // addrinfo
#include <sys/types.h>
#include <sys/socket.h> // recvfrom,send
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h> // E*
#include <string.h> // strlen
#include <unistd.h> // getpid
#include <stdlib.h> // strtol,qsort
#include "kogmo_rtdb.h"


#ifdef UDPSIMPLEBZIP2
#include <bzlib.h>
// i want to transmit a reduced 300k image, but it's hard to estimate the allowed size before compression
#define BUFSIZE 650000
#else
#define BUFSIZE 65000
#endif

#define UDPMON_PORT_DEFAULT "64000"

#define DIEonERR(value) if ((value)<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-(value));exit(1);}
#define DIE(msg...) do { \
 fprintf(stderr,"%d DIED in %s line %i: ",getpid(),__FILE__,__LINE__); fprintf(stderr,msg); fprintf(stderr,"\n"); exit(1); } while (0)
#define ERR(msg...) do { \
 fprintf(stderr,"%d ERROR in %s line %i: ",getpid(),__FILE__,__LINE__); fprintf(stderr,msg); fprintf(stderr,"\n"); } while (0)
#define DBG(msg...) if (debug) { \
 fprintf(stderr,"%d DBG: ",getpid()); fprintf(stderr,msg); fprintf(stderr,"\n"); }


// UDP-Simple "Protocol"

static short int kogmo_rtdb_udpmon_protocol_version = 2;
typedef PACKED_struct
{
  char rtdbfcc[4];
  unsigned short int    version;
  unsigned short int    request;
  unsigned short int    flags;
  kogmo_rtdb_objid_t    oid;
  kogmo_timestamp_t     ts;
  kogmo_rtdb_objname_t	name;
  kogmo_rtdb_objtype_t  otype;
  kogmo_rtdb_objid_t    parent_oid;
  kogmo_rtdb_objid_t    proc_oid;
  int nth;
} kogmo_rtdb_obj_udpsimplereq_t;

typedef PACKED_struct
{
  char rtdbfcc[4];
  unsigned short int    version;
  unsigned short int    reply;
  unsigned short int    flags;
  kogmo_rtdb_objid_t    oid; // also used for return codes
} kogmo_rtdb_obj_udpsimplerply_t;

#define UDPSIMPLEFLAG_INFO      0x01
#define UDPSIMPLEFLAG_DATA      0x02
#define UDPSIMPLEFLAG_LIST      0x04
#define UDPSIMPLEFLAG_WRITE     0x08
#define UDPSIMPLEFLAG_JPEGIMAGE 0x80

// Necessary Remote-to-Local OID-Mapping

typedef struct
{
  kogmo_rtdb_objid_t   src;
  kogmo_rtdb_objid_t   dest;
} oid_map_t;
oid_map_t oid_map[KOGMO_RTDB_OBJIDLIST_MAX+1];

inline kogmo_rtdb_objid_t
oid_map_query(kogmo_rtdb_objid_t src)
{
  int i;
  for(i=0;i<KOGMO_RTDB_OBJIDLIST_MAX;i++)
    if (oid_map[i].src==src)
      return oid_map[i].dest;
  return 0; // not found
}

inline int
oid_map_add(kogmo_rtdb_objid_t src, kogmo_rtdb_objid_t dest)
{
  int i;
  for(i=0;i<KOGMO_RTDB_OBJIDLIST_MAX;i++)
    if (oid_map[i].src == 0)
      {
        oid_map[i].src=src;
        oid_map[i].dest=dest;
        return 1; // ok
      }
  return 0; // map overflow
}

inline void
oid_map_del(kogmo_rtdb_objid_t src)
{
  int i;
  for(i=0;i<KOGMO_RTDB_OBJIDLIST_MAX;i++)
    if (oid_map[i].src == src)
      {
        oid_map[i].src=0;
        oid_map[i].dest=0;
      }
}


// Reduce image size to fit into a udp packet
// (Horrible Hack, because we "optimize" the image for better bzip2 compression)
inline int
reduce_a2_image (kogmo_rtdb_obj_a2_image_t *imgobj_p)
{
  int line;
  memset(imgobj_p->image.data,0,imgobj_p->image.widthstep*imgobj_p->image.height*1/2);
  for(line=imgobj_p->image.height*1/2;line<imgobj_p->image.height;line+=4)
    {
      int px;
      for(px=0;px<imgobj_p->image.widthstep;px+=4)
        {
          imgobj_p->image.data[imgobj_p->image.widthstep*line+px+2] =
           imgobj_p->image.data[imgobj_p->image.widthstep*line+px+1] =
           imgobj_p->image.data[imgobj_p->image.widthstep*line+px];
        }

      memcpy(imgobj_p->image.data + imgobj_p->image.widthstep*(line+1),
             imgobj_p->image.data + imgobj_p->image.widthstep*line,
             imgobj_p->image.widthstep);
      memcpy(imgobj_p->image.data + imgobj_p->image.widthstep*(line+2),
             imgobj_p->image.data + imgobj_p->image.widthstep*line,
             imgobj_p->image.widthstep);
      memcpy(imgobj_p->image.data + imgobj_p->image.widthstep*(line+3),
             imgobj_p->image.data + imgobj_p->image.widthstep*line,
             imgobj_p->image.widthstep);
    }
  memset(imgobj_p->image.data + imgobj_p->image.widthstep*imgobj_p->image.height*7/8,
         0,imgobj_p->image.widthstep*imgobj_p->image.height*1/8);
  return imgobj_p->base.size; // bzip2 does it for us (TODO: use jpeg)
}

#ifdef USEJPEG
#define JPEGQUALITY 20
#include "jmemio.h"
inline int
jpeg_compress_a2_image (kogmo_rtdb_obj_a2_image_t *imgobj_p)
{
  static void* buf=NULL;
  const int bufsize=2000000;
  unsigned long usedsize;
  if (!buf) buf=malloc(bufsize);
  if (!buf) return 0;
  write_JPEG_memory (imgobj_p->image.data, imgobj_p->image.width, imgobj_p->image.height,
                     buf, bufsize, JPEGQUALITY, &usedsize, imgobj_p->image.channels);
  usedsize += sizeof(kogmo_rtdb_obj_a2_image_t);
  if(usedsize > imgobj_p->base.size) return 0;
  memcpy(imgobj_p->image.data, buf, usedsize);
  imgobj_p->base.size = usedsize;
  return usedsize;
}

inline kogmo_rtdb_obj_a2_image_t*
jpeg_decompress_a2_image (kogmo_rtdb_obj_a2_image_t *imgobj_p)
{
  static kogmo_rtdb_obj_a2_image_t *buf=NULL;
  const int bufsize=2000000;
  int w,h;
  if (!buf) buf=malloc(bufsize);
  if (!buf) return NULL;
  memcpy(buf, imgobj_p, sizeof(kogmo_rtdb_obj_a2_image_t));
  read_JPEG_memory (buf->image.data, &w, &h, imgobj_p->image.data, imgobj_p->base.size - sizeof(kogmo_rtdb_obj_a2_image_t));
  buf->base.size = buf->image.widthstep*buf->image.height + sizeof(kogmo_rtdb_obj_a2_image_t); // FIX: retrieve from jpeg-decompressor?
  return buf;
}
#endif
