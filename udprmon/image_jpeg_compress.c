/*! \file image_jpeg_compress.c
 * \brief Compresses Images from a Video with JPEG (from an to the RTDB)
 *
 * (c) 2008 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "jmemio.h" // write_JPEG_memory()

// RTDB Interface
#include "kogmo_rtdb.h"

#define DIEonERR(value) if (value<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-value);exit(1);}
#define DIE(msg...) do { \
 fprintf(stderr,"%d DIED in %s line %i: ",getpid(),__FILE__,__LINE__); fprintf(stderr,msg); fprintf(stderr,"\n"); exit(1); } while (0)
#define DBG(msg...) if (getenv("DEBUG")) { \
 fprintf(stderr,"%d DBG: ",getpid()); fprintf(stderr,msg); fprintf(stderr,"\n"); }

int
main (int argc, char **argv)
{
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;

  kogmo_rtdb_obj_info_t videoinobj_info, videooutobj_info;
  kogmo_rtdb_obj_a2_image_t *videoinobj_p, *videooutobj_p;
  kogmo_rtdb_objid_t oid;
  kogmo_rtdb_objsize_t olen;

  unsigned long usedsize;
  int err;

  char *videoinname =  "left_image";
  char *videooutname = "left_image_jpeg";
  double interval = 0.5;
  int jpeg_quality = 50;

  if ( argc == 2 && strcmp("-h",argv[1])==0 )
    {
      fprintf (stderr, "Usage: image_jpeg_compress INIMAGENAME OUTIMAGENAME INTERVAL JPEGQUALITY\n");
      exit(1);
    }

  if(argc>=2) videoinname = argv[1];
  if(argc>=3) videooutname = argv[2];
  if(argc>=4) interval = atof(argv[3]);
  if(argc>=5) jpeg_quality = atoi(argv[4]);

  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "image_jpeg_compress", interval); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  // Auf Object fuer Video warten
  oid = kogmo_rtdb_obj_searchinfo_wait (dbc, videoinname, KOGMO_RTDB_OBJTYPE_A2_IMAGE, 0, 0); DIEonERR(oid);
  err = kogmo_rtdb_obj_readinfo (dbc, oid, 0, &videoinobj_info); DIEonERR(err);
  videoinobj_p = malloc (videoinobj_info.size_max);
  if(videoinobj_p==NULL) DIE("cannot allocate memory");
  olen = kogmo_rtdb_obj_readdata_waitnext (dbc, videoinobj_info.oid, 0, videoinobj_p, videoinobj_info.size_max); DIEonERR(olen);

  // Eigenes Video-Objekt erstellen
  err = kogmo_rtdb_obj_initinfo (dbc, &videooutobj_info, videooutname, KOGMO_RTDB_OBJTYPE_A2_IMAGE, videoinobj_info.size_max); DIEonERR(err);
  videooutobj_info.parent_oid = videoinobj_info.oid;
  videooutobj_info.history_interval = videoinobj_info.history_interval;
  videooutobj_info.avg_cycletime = interval ? interval : videoinobj_info.avg_cycletime;
  videooutobj_info.max_cycletime = interval ? interval : videoinobj_info.max_cycletime;
  oid = kogmo_rtdb_obj_insert (dbc, &videooutobj_info); DIEonERR(oid);

  // Daten des eigenen Video-Objekts initialisieren und Werte eintragen
  videooutobj_p = malloc (videoinobj_info.size_max);
  if(videooutobj_p==NULL) DIE("cannot allocate memory");
  err = kogmo_rtdb_obj_initdata (dbc, &videooutobj_info, videooutobj_p); DIEonERR(err);
  memcpy(videooutobj_p,videoinobj_p,videoinobj_info.size_max);
  videooutobj_p->image.width     = videoinobj_p->image.width;
  videooutobj_p->image.height    = videoinobj_p->image.height;
  videooutobj_p->image.depth     = videoinobj_p->image.depth;
  videooutobj_p->image.channels  = videoinobj_p->image.channels;
  videooutobj_p->image.colormodell = videoinobj_p->image.colormodell;
  videooutobj_p->image.widthstep = videoinobj_p->image.widthstep;

  // erstes frame testweise kopieren:
  //memcpy(videooutobj_p->image.data, videoinobj_p->image.data, videoinobj_p->base.size);
  //err = kogmo_rtdb_obj_writedata (dbc, videooutobj_info.oid, videooutobj_p); DIEonERR(err);

  free(videoinobj_p);

  do
    {
      olen = kogmo_rtdb_obj_readdata_waitnext_ptr (dbc, videoinobj_info.oid, 0, &videoinobj_p, videoinobj_info.size_max); DIEonERR(olen);
      if ( olen!=videoinobj_info.size_max || olen!=videoinobj_p->base.size)
        DIE("input object has wrong size");

      err = kogmo_rtdb_obj_writedata_ptr_begin (dbc, videooutobj_info.oid, (void**)&videooutobj_p); DIEonERR(err);
      #ifdef USEJPEG
      if (getenv("TESTONLYCOPY"))
        memcpy(videooutobj_p, videoinobj_p, videoinobj_p->base.size);
      else
        {
          write_JPEG_memory (videoinobj_p->image.data, videoinobj_p->image.width, videoinobj_p->image.height,
                     videooutobj_p->image.data, videooutobj_info.size_max, jpeg_quality, &usedsize, videoinobj_p->image.channels);
          memcpy(videooutobj_p, videoinobj_p, sizeof(kogmo_rtdb_obj_a2_image_t));
          videooutobj_p->base.size = usedsize + sizeof(kogmo_rtdb_obj_a2_image_t);
          videooutobj_p->image.colormodell = A2_IMAGE_COLORMODEL_RGBJPEG;
        }
      #else
      memcpy(videooutobj_p, videoinobj_p, videoinobj_p->base.size);
      #endif
      err = kogmo_rtdb_obj_writedata_ptr_commit (dbc, videooutobj_info.oid, (void**)&videooutobj_p); DIEonERR(err);

      usleep(interval*1e6);
    }
  while(1);

  err = kogmo_rtdb_disconnect(dbc, NULL); DIEonERR(err);
  return 0;
}
