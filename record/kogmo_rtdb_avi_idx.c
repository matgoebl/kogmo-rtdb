/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */
/*! \file kogmo_rtdb_avi_idx.c
 * \brief Append AVI-Index to RTDB-Streams
 *
 * Copyright (c) 2008 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */


#define _FILE_OFFSET_BITS 64
#if !defined(O_LARGEFILE) && defined(MACOSX) // not supported?
 #define O_LARGEFILE 0
#endif

#include <stdio.h> /* printf */
#include <unistd.h> /* usleep */
#include <string.h> /* strerror */
#include <getopt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
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

void
usage (void)
{
  fprintf(stderr,
"KogMo-RTDB AVI-Index (Rev.%d) (c) Matthias Goebl <mg*tum.de> RCS-TUM\n"
"Usage: kogmo_rtdb_avi_idx [-c] -o FILE\n"
"Creates an avi-index for rtdb avi files and updates the avi header with the correct size.\n"
"Afterwards even the windows media player should play this avi.\n"
"If this tool is used on a already indexed rtdb avi file, the index will be rebuild.\n"
"This tools only works on files smaller than 2 GB.\n"
"\n"
" -c       compatibility mode: videos will be shown upside-down, but can be played by every version of mplayer\n"
" -o FILE avi file to use, will be modified in place\n"
"\n",KOGMO_RTDB_REV);
  exit(1);
}

int
main (int argc, char **argv)
{
  char *filename=NULL;
  int fd, njunk, i, aviidxheader_size;
  off_t filesize, newfilesize, maxfilesize;
  struct stat filestat;
  avirawheaderv1_t *avirawheader;  
  char *filemap, *ptr, *movistart;
  aviidxheader_t *aviidxheader;
  int aviidxentries,aviidxentries_stream[NSTREAMS], streamnr, stream9used=0;
  int opt, do_compatibility = 0, do_verbose=0;

  while( ( opt = getopt (argc, argv, "o:cvh") ) != -1 )
    switch(opt)
      {
        case 'o': filename = optarg; break;
        case 'c': do_compatibility = 1; break;
        case 'v': do_verbose = 1; break;
        case 'h':
        default: usage(); break;
      }

  if ( !filename ) usage();

  fd = open(filename, O_RDWR | O_LARGEFILE );
  if ( fd < 0 ) DIE("cannot open avi file '%s'",filename);

  // 64 bit???
  if ( fstat (fd, &filestat) != 0 )
    DIE("cannot stat file to find out file size");

  filesize = filestat.st_size;
  maxfilesize = (off_t)1<<31;

  printf("File %s has %lli bytes (%.3fMB).\n",filename,(long long int)filesize,(double)filesize/1024/1024);

  if ( filesize >= maxfilesize )
    DIE("File too large. Can only work with files <%.fGB.",(double)maxfilesize/1024/1024/1024);

  filemap = mmap(NULL, filesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if ( filemap == MAP_FAILED || filemap == NULL )
    DIE("cannot mmap() file: %s",strerror(errno));

  avirawheader = (void*) filemap;


  //***** Check if it fits in our fixed-struct

  if ( avirawheader->movi_list.type_fcc[0] != 'm' ||
       avirawheader->movi_list.type_fcc[1] != 'o' ||
       avirawheader->movi_list.type_fcc[2] != 'v' ||
       avirawheader->movi_list.type_fcc[3] != 'i' )
    DIE("not a rtdb avi!");



  //***** Fix misc. Data

  for(i=0;i<NSTREAMS;i++)
    {
      if ( !do_compatibility && avirawheader->str_headers[i].strf_header.biHeight * HEIGHT_SIGN < 0 )
        avirawheader->str_headers[i].strf_header.biHeight *= -1; //!
      if ( do_compatibility && avirawheader->str_headers[i].strf_header.biHeight < 0 )
        avirawheader->str_headers[i].strf_header.biHeight *= -1; //!
      avirawheader->str_headers[i].strh_header.dwQuality = 0; //!
      avirawheader->str_headers[i].strf_header.biSize = sizeof(avibitmapheader_t) -8 - NCOLORS*sizeof(rgbquad_t); //avirawheader->str_headers[i].strf_header.cb - 4; //!
      // avirawheader->str_headers[i].strf_header.biCompression = 0; // 0x20776172; // alternative: 'RAW '=0x20574152 'RGB '=0x20424752
      // avirawheader->str_headers[i].strh_header.fccHandler[0..3]=0;
    }


  //***** Create Index

  movistart = filemap + sizeof (avirawheader_t);
  ptr = movistart;
  njunk = 0;

  aviidxheader = malloc( sizeof(aviidxheader_t) );
  if ( aviidxheader == NULL )
    DIE("cannot allocate memory for index entries"); 
  aviidxentries = 0;
  for(i=0;i<NSTREAMS;i++)
    aviidxentries_stream[i]=0;

  while ( ptr < filemap + filesize - 8 )
    {
      unsigned long int cb = (unsigned long int)((riffchunk_t*)ptr) -> cb;
      char *fcc = ((riffchunk_t*)ptr)->fcc;
      if (do_verbose)
        printf("- ChunkID %.4s (%lu bytes @ %lu = %.3fMB )\n", fcc, cb, (unsigned long int)(ptr - movistart), (double)(ptr - movistart)/1024/1024);
      if ( ( fcc[0] == '0' && fcc[1] >= '0' && fcc[1] <= '9' && fcc[2] == 'd' && fcc[3] == 'b' )
        || ( do_compatibility && !aviidxentries ) ) // wenn der erste index-eintrag zu gross ist, nimmt mplayer faelschlicherweise an, dass es absolute offsets sind
        {
          streamnr = fcc[1]-'0';
          aviidxentries++;
          aviidxentries_stream[streamnr]++;
          if (do_verbose)
            printf("  Index %i: %i. for stream %i\n", aviidxentries, aviidxentries_stream[streamnr], streamnr);
          aviidxheader = realloc(aviidxheader, sizeof(aviidxheader_t) + sizeof(aviidxentry_t) * aviidxentries);
          if ( aviidxheader == NULL )
            DIE("cannot allocate more memory for index entries");
          memcpy( aviidxheader->aviidxentry[aviidxentries-1].dwChunkId, fcc, 4);
          aviidxheader->aviidxentry[aviidxentries-1].dwFlags  = AVIIF_KEYFRAME;
          aviidxheader->aviidxentry[aviidxentries-1].dwOffset = ptr - movistart + 4; // abs: ptr - filemap
          aviidxheader->aviidxentry[aviidxentries-1].dwSize   = cb;
          if (fcc[1] == '9') stream9used=1;
        }
      if ( ( cb & 1 ) != 0 ) cb+=1; // odd number
      ptr += cb + 8; // *VOR* idx1-Erkennung!
      if ( memcmp(fcc, "idx1", 4) == 0 )
        {
          break;
        }
    }

  if(!stream9used)
    {
      memcpy(&avirawheader->str_headers[9], &avirawheader->str_headers[0], sizeof(avirawheader->str_headers[0]));
    }

  //printf("ptr-filemap: %llu, filesize: %llu, remaining: %lli\n",
  //  (long long unsigned int)(ptr-filemap), filesize, filesize-(ptr-filemap));

  newfilesize = ptr-filemap;

  if ( lseek( fd, newfilesize, SEEK_SET) != newfilesize )
    DIE("cannot seek to end of file");

  memcpy(aviidxheader->fcc, "idx1", 4);
  aviidxheader->cb = sizeof(aviidxentry_t) * aviidxentries;

  aviidxheader_size = sizeof(aviidxheader_t) + sizeof(aviidxentry_t) * aviidxentries;
  if ( write( fd, aviidxheader, aviidxheader_size ) != aviidxheader_size )
    DIE("error while writing index");

  avirawheader->movi_list.list_cb = newfilesize - sizeof(avirawheader_t) + 4; //!
  newfilesize += aviidxheader_size;
  avirawheader->riff_header.file_cb = newfilesize - 8; //!
  avirawheader->avih_header.dwFlags |= AVIF_HASINDEX | AVIF_ISINTERLEAVED; //!
  avirawheader->avih_header.dwTotalFrames = aviidxentries_stream[0]; //! aviidxentries<2?aviidxentries:999; reicht nicht

  for(i=0;i<NSTREAMS;i++)
    avirawheader->str_headers[i].strh_header.dwLength = aviidxentries_stream[i]; //! aviidxentries<2?aviidxentries:999;

  printf("Wrote %i index entries.\n",aviidxentries);
  if (do_verbose)
    for(i=0;i<NSTREAMS;i++)
      printf("  Stream %i: %i entries\n", i, aviidxentries_stream[i]);

  printf("AVI header updated.\n");

  if ( munmap(avirawheader, filesize ) != 0)
    DIE("cannot munmap() file");

  if ( ftruncate(fd, newfilesize ) != 0 )
    DIE("cannot truncate file to the correct size");

  if ( close(fd) != 0)
    DIE("cannot close() file");

  return 0;
}
