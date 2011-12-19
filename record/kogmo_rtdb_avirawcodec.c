/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */
/*! \file avirawcodec.c
 * \brief Fast and simple AVI-Codec for RAW Videos
 *
 * Copyright (c) 2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */


#define _FILE_OFFSET_BITS 64
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#define streamssize (0x7f000000)
#include "kogmo_rtdb_avirawcodec.h"

int aviraw_initheader(avirawheader_t *avirawheader, float fps, int width, int height, int bpp)
{
  int i, nstreams, imgsize, nframes;
  nstreams = NSTREAMS; // bis jetzt fix

  if(!fps || !width || !height || nstreams<1 || nstreams > NSTREAMS)
    return(-1);

  imgsize = width*height*(bpp/8);
  nframes = streamssize / (fps*imgsize);

  memset (avirawheader, 0, sizeof(avirawheader_t));

  avirawheader->riff_header.header_fcc[0] = 'R';
  avirawheader->riff_header.header_fcc[1] = 'I';
  avirawheader->riff_header.header_fcc[2] = 'F';
  avirawheader->riff_header.header_fcc[3] = 'F';
  avirawheader->riff_header.file_cb       =  sizeof(avirawheader_t) -8 + streamssize;
  avirawheader->riff_header.file_fcc[0]   = 'A';
  avirawheader->riff_header.file_fcc[1]   = 'V';
  avirawheader->riff_header.file_fcc[2]   = 'I';
  avirawheader->riff_header.file_fcc[3]   = ' ';

  avirawheader->hdrl_list.list_fcc[0] = 'L';
  avirawheader->hdrl_list.list_fcc[1] = 'I';
  avirawheader->hdrl_list.list_fcc[2] = 'S';
  avirawheader->hdrl_list.list_fcc[3] = 'T';
  avirawheader->hdrl_list.list_cb     =  sizeof(avirawheader->hdrl_list) + sizeof(avirawheader->avih_header) +
#ifndef AVIV2
       sizeof(avirawheader->strl_list) + sizeof(avirawheader->str_headers) - 8;
#else
                                       + sizeof(avirawheader->str_headers) - 8;
#endif 
  avirawheader->hdrl_list.type_fcc[0] = 'h';
  avirawheader->hdrl_list.type_fcc[1] = 'd';
  avirawheader->hdrl_list.type_fcc[2] = 'r';
  avirawheader->hdrl_list.type_fcc[3] = 'l';

  avirawheader->avih_header.fcc[0]                  = 'a';
  avirawheader->avih_header.fcc[1]                  = 'v';
  avirawheader->avih_header.fcc[2]                  = 'i';
  avirawheader->avih_header.fcc[3]                  = 'h';
  avirawheader->avih_header.cb                      =  sizeof(avirawheader->avih_header) - 8;
  avirawheader->avih_header.dwMicroSecPerFrame      =  (uint32_t)(1000000.0 / fps);
  avirawheader->avih_header.dwMaxBytesPerSec        =  (uint32_t)(fps*imgsize); // 0 auch ok
  avirawheader->avih_header.dwPaddingGranularity    =  0;						// ok
  avirawheader->avih_header.dwFlags                 =  0;						// ggf. AVIF_HASINDEX
  avirawheader->avih_header.dwTotalFrames           =  nframes;					// berechnen
  avirawheader->avih_header.dwInitialFrames         =  0;						// ok
  avirawheader->avih_header.dwStreams               =  nstreams;				// i.d.R. 1, 10 auch ok, evtl berechnen
  avirawheader->avih_header.dwSuggestedBufferSize   =  width*height;			// 0 auch ok
  avirawheader->avih_header.dwWidth                 =  width;					// ok
  avirawheader->avih_header.dwHeight                =  height;					// ok

#ifndef AVIV2
  avirawheader->strl_list.list_fcc[0] = 'L';
  avirawheader->strl_list.list_fcc[1] = 'I';
  avirawheader->strl_list.list_fcc[2] = 'S';
  avirawheader->strl_list.list_fcc[3] = 'T';
  avirawheader->strl_list.list_cb     = sizeof(avirawheader->strl_list) + sizeof(avirawheader->str_headers) - 8;
  avirawheader->strl_list.type_fcc[0] = 's';
  avirawheader->strl_list.type_fcc[1] = 't';
  avirawheader->strl_list.type_fcc[2] = 'r';
  avirawheader->strl_list.type_fcc[3] = 'l';
#endif

  for(i=0;i<NSTREAMS;i++)
    {
#ifdef AVIV2
      avirawheader->str_headers[i].strl_list.list_fcc[0] = 'L';
      avirawheader->str_headers[i].strl_list.list_fcc[1] = 'I';
      avirawheader->str_headers[i].strl_list.list_fcc[2] = 'S';
      avirawheader->str_headers[i].strl_list.list_fcc[3] = 'T';
      avirawheader->str_headers[i].strl_list.list_cb     = sizeof(rifflist_t)-8 + sizeof(avistreamheader_t) + sizeof(avibitmapheader_t);
      avirawheader->str_headers[i].strl_list.type_fcc[0] = 's';
      avirawheader->str_headers[i].strl_list.type_fcc[1] = 't';
      avirawheader->str_headers[i].strl_list.type_fcc[2] = 'r';
      avirawheader->str_headers[i].strl_list.type_fcc[3] = 'l';
#endif

      // deactivate header by default
      avirawheader->str_headers[i].strh_header.fcc[0]                = 'J';
      avirawheader->str_headers[i].strh_header.fcc[1]                = 'U';
      avirawheader->str_headers[i].strh_header.fcc[2]                = 'N';
      avirawheader->str_headers[i].strh_header.fcc[3]                = 'K';
      avirawheader->str_headers[i].strh_header.cb                    =  sizeof(avirawheader->str_headers[0].strh_header) - 8 + sizeof(avirawheader->str_headers[0].strf_header);
    }

  avirawheader->movi_list.list_fcc[0] = 'L';
  avirawheader->movi_list.list_fcc[1] = 'I';
  avirawheader->movi_list.list_fcc[2] = 'S';
  avirawheader->movi_list.list_fcc[3] = 'T';
  avirawheader->movi_list.list_cb     = sizeof(avirawheader->movi_list) - 8 + streamssize; // +??
  avirawheader->movi_list.type_fcc[0] = 'm';
  avirawheader->movi_list.type_fcc[1] = 'o';
  avirawheader->movi_list.type_fcc[2] = 'v';
  avirawheader->movi_list.type_fcc[3] = 'i';
  return 0;
}

int aviraw_initstream(avirawheader_t *avirawheader, int stream, float fps, int width, int height, int bpp, uint32_t compression_fourcc)
{
  int imgsize, nframes, j;

  if(!fps || !width || !height || stream<0 || stream > 10)
    return(-1);

  imgsize = width*height*(bpp/8);
  nframes = streamssize / (fps*imgsize);

  avirawheader->str_headers[stream].strh_header.fcc[0]                = 's';
  avirawheader->str_headers[stream].strh_header.fcc[1]                = 't';
  avirawheader->str_headers[stream].strh_header.fcc[2]                = 'r';
  avirawheader->str_headers[stream].strh_header.fcc[3]                = 'h';
  avirawheader->str_headers[stream].strh_header.cb                    =  sizeof(avirawheader->str_headers[0].strh_header) - 8;
  avirawheader->str_headers[stream].strh_header.fccType[0]            = 'v';
  avirawheader->str_headers[stream].strh_header.fccType[1]            = 'i';
  avirawheader->str_headers[stream].strh_header.fccType[2]            = 'd';
  avirawheader->str_headers[stream].strh_header.fccType[3]            = 's';
  avirawheader->str_headers[stream].strh_header.fccHandler[0]         = 'D'; // ok
  avirawheader->str_headers[stream].strh_header.fccHandler[1]         = 'I';
  avirawheader->str_headers[stream].strh_header.fccHandler[2]         = 'B';
  avirawheader->str_headers[stream].strh_header.fccHandler[3]         = ' ';
  avirawheader->str_headers[stream].strh_header.dwFlags               =  0; // ok
  avirawheader->str_headers[stream].strh_header.wPriority             =  0; // ok. NSTREAMS-i bringt nichts
  avirawheader->str_headers[stream].strh_header.wLanguage             =  0; // ok
  avirawheader->str_headers[stream].strh_header.dwInitialFrames       =  0; // ok
  avirawheader->str_headers[stream].strh_header.dwScale               =  1; // ok
  avirawheader->str_headers[stream].strh_header.dwRate                =  (uint32_t)fps; //ok
  avirawheader->str_headers[stream].strh_header.dwStart               =  0; // ok
  avirawheader->str_headers[stream].strh_header.dwLength              =  nframes; // berechnen
  avirawheader->str_headers[stream].strh_header.dwSuggestedBufferSize =  imgsize; // ok
  avirawheader->str_headers[stream].strh_header.dwQuality             =  0; // ok
  avirawheader->str_headers[stream].strh_header.dwSampleSize          =  0; // ok
  avirawheader->str_headers[stream].strh_header.rcFrame.left          =  0; // ok
  avirawheader->str_headers[stream].strh_header.rcFrame.top           =  0; // ok
  avirawheader->str_headers[stream].strh_header.rcFrame.right         =  width;  // ok
  avirawheader->str_headers[stream].strh_header.rcFrame.bottom        =  height; // ok

  avirawheader->str_headers[stream].strf_header.fcc[0]                = 's';
  avirawheader->str_headers[stream].strf_header.fcc[1]                = 't';
  avirawheader->str_headers[stream].strf_header.fcc[2]                = 'r';
  avirawheader->str_headers[stream].strf_header.fcc[3]                = 'f';
  avirawheader->str_headers[stream].strf_header.cb                    =  sizeof(avirawheader->str_headers[0].strf_header) - 8;
  avirawheader->str_headers[stream].strf_header.biSize                =  sizeof(avibitmapheader_t) -8 - NCOLORS*sizeof(rgbquad_t); // ok
  avirawheader->str_headers[stream].strf_header.biWidth               =  width;  // ok
  avirawheader->str_headers[stream].strf_header.biHeight              =  height; // mag der neue mplayer nicht: height * HEIGHT_SIGN; // ok
  avirawheader->str_headers[stream].strf_header.biPlanes              =  1; // ok
  avirawheader->str_headers[stream].strf_header.biBitCount            =  bpp; // ok
  avirawheader->str_headers[stream].strf_header.biCompression         =  compression_fourcc; // ok, default 0: BI_RGB
  avirawheader->str_headers[stream].strf_header.biSizeImage           =  imgsize; // 0 auch ok
  avirawheader->str_headers[stream].strf_header.biXPelsPerMeter       =  0; // 0
  avirawheader->str_headers[stream].strf_header.biYPelsPerMeter       =  0; // 0
  avirawheader->str_headers[stream].strf_header.biClrUsed             =  NCOLORS; // 00 01 00 00  00 01 00 00  00 01 00 00
  avirawheader->str_headers[stream].strf_header.biClrImportant        =  0; // 0

  if (bpp==8)
    {
      for(j=0;j<NCOLORS;j++)
        {
          avirawheader->str_headers[stream].strf_header.bmiColors[j].rgbBlue  = j;
          avirawheader->str_headers[stream].strf_header.bmiColors[j].rgbGreen = j;
          avirawheader->str_headers[stream].strf_header.bmiColors[j].rgbRed   = j;
        }
    }
  else
    {
      riffchunk_t *junk = (riffchunk_t*) &(avirawheader->str_headers[stream].strf_header.bmiColors[0]);
      junk->fcc[0] = 'J';
      junk->fcc[1] = 'U';
      junk->fcc[2] = 'N';
      junk->fcc[3] = 'K';
      junk->cb = NCOLORS*sizeof(rgbquad_t) - 8;
      avirawheader->str_headers[stream].strf_header.cb        =  sizeof(avirawheader->str_headers[0].strf_header) - 8 - NCOLORS*sizeof(rgbquad_t);
      avirawheader->str_headers[stream].strf_header.biClrUsed = 0;
    }

  return 0;
}


int aviraw_fput_header(FILE *fp, avirawheader_t *avirawheader)
{
  int ret;
  ret = fwrite(avirawheader,sizeof(avirawheader_t),1,fp);
  if(ret!=1)
    return ret<0? ret : -1;
  return 0;
}

int aviraw_fput_stream(FILE *fp, int stream, void *data, int size)
{
  int ret;
  riffchunk_t datachunk;
  if(stream<0 || stream > 10)
    return -1;
  datachunk.fcc[0] = '0';
  datachunk.fcc[1] = '0'+stream;
  datachunk.fcc[2] = 'd';
  datachunk.fcc[3] = 'b';
  datachunk.cb     =  size;
  ret = fwrite(&datachunk,sizeof(datachunk),1,fp);
  if(ret!=1)
    return ret<0? ret : -1;

  ret = fwrite(data,size,1,fp);
  if(ret!=1)
    return ret<0? ret : -1;

  if ( size & 1 ) // odd number
    {
      ret = fwrite("\0",1,1,fp);
      if(ret!=1) return ret<0? ret : -1;
    }

  return 0;
}

int aviraw_fput_junk(FILE *fp, void *data, int size)
{
  int ret;
  riffchunk_t datachunk;
  datachunk.fcc[0] = 'J';
  datachunk.fcc[1] = 'U';
  datachunk.fcc[2] = 'N';
  datachunk.fcc[3] = 'K';
  datachunk.cb     =  size;
  ret = fwrite(&datachunk,sizeof(datachunk),1,fp);
  if(ret!=1)
    return ret<0? ret : -1;

  ret = fwrite(data,size,1,fp);
  if(ret!=1)
    return ret<0? ret : -1;

  if ( size & 1 ) // odd number
    {
      ret = fwrite("\0",1,1,fp);
      if(ret!=1)
        return ret<0? ret : -1;
    }

  return 0;
}

int aviraw_fput_chunk(FILE *fp, riffchunk_t *datachunk_p, void *data)
{
  int ret;
  ret = fwrite(datachunk_p,sizeof(riffchunk_t),1,fp);
  if(ret!=1)
    return ret<0? ret : -1;

  ret = fwrite(data,datachunk_p->cb,1,fp);
  if(ret!=1)
    return ret<0? ret : -1;

  if ( datachunk_p->cb & 1 ) // odd number
    {
      ret = fwrite("\0",1,1,fp);
      if(ret!=1)
        return ret<0? ret : -1;
    }

  return 0;
}

int aviraw_fput_data(FILE *fp, void *data, int size)
{
  int ret;
  ret = fwrite(data,size,1,fp);
  if(ret!=1)
    return ret<0? ret : -1;
  if ( size & 1 ) // odd number
    {
      ret = fwrite("\0",1,1,fp);
      if(ret!=1)
        return ret<0? ret : -1;
    }
  return 0;
}

int aviraw_fget_header(FILE *fp, avirawheader_t *avirawheader)
{
  int ret;
  ret = fread(avirawheader,sizeof(avirawheader_t),1,fp);
  if(ret!=1)
    return ret<0? ret : -1;
  if (strncmp (avirawheader->riff_header.file_fcc, "AVI ", 4) != 0)
    return -1;
  if (strncmp (avirawheader->movi_list.type_fcc, "movi", 4) != 0)
    return -1;
  return 0;
}

int aviraw_fget_chunkheader(FILE *fp, riffchunk_t *datachunk_p)
{
  int ret;
  ret = fread(datachunk_p,sizeof(riffchunk_t),1,fp);
  if(ret!=1)
    return ret<0? ret : -1;
  return 0;
}

int aviraw_chunk_is_junk(riffchunk_t *datachunk_p)
{
  if (strncmp (datachunk_p->fcc, "JUNK", 4) != 0)
    return 0;
  return 1;
}

int aviraw_chunk_nr_stream(riffchunk_t *datachunk_p)
{
  if (datachunk_p->fcc[0] != '0') return -1;
  if (datachunk_p->fcc[1] < '0' || datachunk_p->fcc[1] > '9' ) return -1;
  if (datachunk_p->fcc[2] != 'd') return -1;
  if (datachunk_p->fcc[3] != 'b') return -1;
  return datachunk_p->fcc[1] - '0';
}

int aviraw_fget_chunkdata(FILE *fp, riffchunk_t *datachunk_p, void *data, int maxsize)
{
  int ret;
  int pad = (datachunk_p->cb & 1 ) ? 1 : 0; // odd number
  if (datachunk_p->cb > maxsize)
    return -1;
  ret = fread(data,datachunk_p->cb+pad,1,fp);
  if(ret!=1)
    return ret<0? ret : -1;
  return datachunk_p->cb;
}

int aviraw_fskip_chunkdata(FILE *fp, riffchunk_t *datachunk_p)
{
  int ret;
  int pad = (datachunk_p->cb & 1 ) ? 1 : 0; // odd number
  ret = fseek(fp,datachunk_p->cb+pad,SEEK_CUR);
  if(ret!=0)
    return ret<0? ret : -1;
  return 0;
}
