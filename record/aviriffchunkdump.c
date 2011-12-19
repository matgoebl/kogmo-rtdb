#define _FILE_OFFSET_BITS 64
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
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>
typedef char FOURCC[4];
typedef unsigned long int DWORD;
typedef unsigned int WORD;
typedef unsigned int UINT;
typedef unsigned long int DWORDLONG;
typedef unsigned long int LONG;
typedef unsigned char BYTE;

#define DIEonERR(value) if ((value)<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-(value));exit(1);}
#define DIEif(cond,msg) if (cond) { \
 fprintf(stderr,"%i DIED in %s line %i: %s (%i)\n",getpid(),__FILE__,__LINE__,msg,cond);exit(1);}

typedef struct
{
  char      header_fcc[4];
  uint32_t  file_cb;
  char      file_fcc[4];
} riffheader_t;

typedef struct
{
  char      fcc[4];
  uint32_t  cb;
} riffchunk_t;

typedef struct
{
  char      list_fcc[4];
  uint32_t  list_cb;
  char      type_fcc[4];
} rifflist_t;

typedef struct
{
  char      fcc[4];
  uint32_t  flags;
  uint32_t  off;
  uint32_t  cb;
} aviidxentry_t;

int 
main (int argc, char **argv)
{
  riffheader_t riffheader;
  aviidxentry_t *idxe;
  off_t pos;

  union {
   char buf[80];
   riffchunk_t riffchunk;
   rifflist_t rifflist;
  } buf;
  int err;

  pos = ftello(stdin);
  err = fread(&riffheader,sizeof(riffheader),1,stdin);
  DIEif(err!=1,"cannot read riff header");
  printf("RIFF-ID: '%c%c%c%c', fileType: '%c%c%c%c', fileSize: %u bytes (%lli .. %lli)\n",
   riffheader.header_fcc[0], riffheader.header_fcc[1], riffheader.header_fcc[2], riffheader.header_fcc[3],
   riffheader.file_fcc[0], riffheader.file_fcc[1], riffheader.file_fcc[2], riffheader.file_fcc[3],
   riffheader.file_cb, (long long int)pos, (long long int)pos+8+riffheader.file_cb-1);
  //DIEif(memcmp(riffheader.header_fcc,"RIFF",4),"no RIFF file");
  if(memcmp(riffheader.header_fcc,"RIFF",4)!=0)
    {
      printf("WARNING: no RIFF file!\n");
        err = fseek(stdin,0,SEEK_SET);
        DIEif(err!=0,"cannot seek to begin of file");
    }
      


  while (!feof(stdin))
  {
    pos = ftello(stdin);
    err = fread(&buf,sizeof(riffchunk_t),1,stdin);
    DIEif(err!=1,"cannot read chunk header");
    if( memcmp(buf.riffchunk.fcc,"LIST",4)==0 )
      {
        err = fread(buf.rifflist.type_fcc,sizeof(buf.rifflist.type_fcc),1,stdin);
        DIEif(err!=1,"cannot read list type");
        printf("- '%c%c%c%c', listType: '%c%c%c%c', listSize: %u bytes (%lli .. %lli)\n",
         buf.rifflist.list_fcc[0], buf.rifflist.list_fcc[1], buf.rifflist.list_fcc[2], buf.rifflist.list_fcc[3],
         buf.rifflist.type_fcc[0], buf.rifflist.type_fcc[1], buf.rifflist.type_fcc[2], buf.rifflist.type_fcc[3],
         buf.rifflist.list_cb, (long long int)pos, (long long int)pos+8+buf.rifflist.list_cb-1);
      }
    else
      {
        #define BUFSZ 256
        unsigned char cbuf[BUFSZ];
        long buflen;
        int i,l;
        printf("- '%c%c%c%c', chunkSize: %u bytes (%llu .. %llu = 0x%llX .. 0x%llX)\n",
         buf.riffchunk.fcc[0], buf.riffchunk.fcc[1], buf.riffchunk.fcc[2], buf.riffchunk.fcc[3],
         buf.riffchunk.cb, (unsigned long long int)pos, (unsigned long long int)pos+8+buf.riffchunk.cb-1,
                           (unsigned long long int)pos, (unsigned long long int)pos+8+buf.riffchunk.cb-1 );
        //pos = ftell(stdin);
        buflen = BUFSZ<buf.riffchunk.cb?BUFSZ:buf.riffchunk.cb;
        err = fread(cbuf,buflen,1,stdin);
        DIEif(err!=1,"cannot read chunk");
        printf(" ");
        for(i=0;i<buflen;i++) {
         printf("%c",cbuf[i]>=32 && cbuf[i]<127 ? cbuf[i] : '.');
         if(i%4==3) printf(" ");
         if(i%64==63) printf("\n ");
        }
        printf("\n ");
        for(i=0;i<buflen;i++) {
         printf("%02X ",cbuf[i]);
         if(i%4==3) printf(" ");
         if(i%16==15) printf("\n ");
        }
        printf("\n");

        if ( memcmp(buf.riffchunk.fcc, "idx1", 4) == 0 ) {
         l = buf.riffchunk.cb;
         for(i=0;i< (buf.riffchunk.cb<BUFSZ?buf.riffchunk.cb:BUFSZ)/sizeof(aviidxentry_t);i++) {
          idxe = (aviidxentry_t*) &cbuf[i * sizeof(aviidxentry_t)];
          printf("  - '%c%c%c%c', chunkSize: %u bytes at %llu = 0x%llX\n",
          idxe->fcc[0], idxe->fcc[1], idxe->fcc[2], idxe->fcc[3],
          idxe->cb, (unsigned long long int)idxe->off, (unsigned long long int)idxe->off);
         }
        }

        if( (buf.riffchunk.cb & 1) != 0) buf.riffchunk.cb++; // must be even
        buf.riffchunk.cb-=buflen;
        err = fseek(stdin,buf.riffchunk.cb,SEEK_CUR);
        DIEif(err!=0,"cannot seek to end of chunk");
      }
  }
  return 0;
}
