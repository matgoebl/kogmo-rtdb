/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*! \file avirawcodec.h
 * \brief Fast and simple AVI-Codec for RAW Videos
 *
 * Copyright (c) 2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <inttypes.h>
#include <stdio.h>

// Structures to Construct an AVI-Header
// Based on specifications from microsoft.com
// see also http://de.wikipedia.org/wiki/Windows_Bitmap

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
  uint32_t  cb;
  uint32_t  dwMicroSecPerFrame;
  uint32_t  dwMaxBytesPerSec;
  uint32_t  dwPaddingGranularity;
  uint32_t  dwFlags;
#define AVIF_HASINDEX       0x00000010
#define AVIF_MUSTUSEINDEX   0x00000020
#define AVIF_ISINTERLEAVED  0x00000100
#define AVIF_TRUSTCKTYPE    0x00000800
#define AVIF_WASCAPTUREFILE 0x00010000
#define AVIF_COPYRIGHTED    0x00020000
  uint32_t  dwTotalFrames;
  uint32_t  dwInitialFrames;
  uint32_t  dwStreams;
  uint32_t  dwSuggestedBufferSize;
  uint32_t  dwWidth;
  uint32_t  dwHeight;
  uint32_t  dwReserved[4];
} avimainheader_t;


typedef struct
{
  char      fcc[4];
  uint32_t  cb;
  char      fccType[4];
  char      fccHandler[4];
  uint32_t  dwFlags;
#define AVISF_DISABLED         0x00000001
#define AVISF_VIDEO_PALCHANGES 0x00010000
  uint16_t  wPriority;
  uint16_t  wLanguage;
  uint32_t  dwInitialFrames;
  uint32_t  dwScale;
  uint32_t  dwRate;
  uint32_t  dwStart;
  uint32_t  dwLength;
  uint32_t  dwSuggestedBufferSize;
  uint32_t  dwQuality;
  uint32_t  dwSampleSize;
  struct
  {
   uint16_t left;
   uint16_t top;
   uint16_t right;
   uint16_t bottom;
  } rcFrame;
} avistreamheader_t;



#define NCOLORS 256
typedef struct {
  uint8_t rgbBlue;
  uint8_t rgbGreen;
  uint8_t rgbRed;
  uint8_t rgbReserved;
} rgbquad_t;

typedef struct
{
  char      fcc[4];
  uint32_t  cb;
  uint32_t  biSize;
  int32_t   biWidth;
  int32_t   biHeight;
#define HEIGHT_SIGN (-1)
  uint16_t  biPlanes;
  uint16_t  biBitCount;
  uint32_t  biCompression;

  uint32_t  biSizeImage;
  int32_t   biXPelsPerMeter;
  int32_t   biYPelsPerMeter;
  uint32_t  biClrUsed;
  uint32_t  biClrImportant;
  rgbquad_t bmiColors[NCOLORS];
} avibitmapheader_t;

#define NSTREAMSV1 10
typedef struct
{
  riffheader_t          riff_header;
  rifflist_t             hdrl_list;
  avimainheader_t         avih_header;
  rifflist_t             strl_list;
  struct
  {
    avistreamheader_t     strh_header;
    avibitmapheader_t     strf_header;
  } str_headers[NSTREAMSV1];
  rifflist_t            movi_list;
} avirawheaderv1_t;


#define NSTREAMSV2 9
typedef struct
{
  riffheader_t          riff_header;
  rifflist_t             hdrl_list;
  avimainheader_t         avih_header;
  struct
  {
    rifflist_t            strl_list;
    avistreamheader_t     strh_header;
    avibitmapheader_t     strf_header;
  } str_headers[NSTREAMSV2];
  riffchunk_t           pad_chunk;
  char                  pad_data[1032];
  rifflist_t            movi_list;
} avirawheaderv2_t;

typedef avirawheaderv1_t avirawheader_t;
#define NSTREAMS NSTREAMSV1


typedef struct
{
  char      fcc[4]; // == 'idx1'
  uint32_t  cb;
} aviidxheader_tx;

typedef struct
{
  char      dwChunkId[4]; // == fcc
  uint32_t  dwFlags;
#define AVIIF_LIST       0x00000001
#define AVIIF_KEYFRAME   0x00000010
#define AVIIF_NO_TIME    0x00000100
  uint32_t  dwOffset; // relativ zu start der 'movi' list
  uint32_t  dwSize; // == cb
} aviidxentry_t;

typedef struct
{
  char      fcc[4]; // == 'idx1'
  uint32_t  cb;
  aviidxentry_t aviidxentry[0];
} aviidxheader_t;


int aviraw_initheader(avirawheader_t *avirawheader, float fps, int width, int height, int bpp);
int aviraw_initstream(avirawheader_t *avirawheader, int stream, float fps, int width, int height, int bpp, uint32_t compression_fourcc);
int aviraw_fput_header(FILE *fp, avirawheader_t *avirawheader);
int aviraw_fput_stream(FILE *fp, int stream, void *data, int size);
int aviraw_fput_junk(FILE *fp, void *data, int size);
int aviraw_fget_header(FILE *fp, avirawheader_t *avirawheader);
int aviraw_fget_chunkheader(FILE *fp, riffchunk_t *datachunk_p);
int aviraw_chunk_is_junk(riffchunk_t *datachunk_p);
int aviraw_chunk_nr_stream(riffchunk_t *datachunk_p);
int aviraw_fget_chunkdata(FILE *fp, riffchunk_t *datachunk_p, void *data, int maxsize);
int aviraw_fskip_chunkdata(FILE *fp, riffchunk_t *datachunk_p);
int aviraw_fput_data(FILE *fp, void *data, int size);
int aviraw_fput_chunk(FILE *fp, riffchunk_t *datachunk_p, void *data);
