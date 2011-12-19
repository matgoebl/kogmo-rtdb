/*! \file kogmo_rtdb_utils.c
 * \brief Some Utility Definitions
 *
 * Copyright (c) 2003-2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdlib.h> /* malloc */
#include <stdio.h> /* snprintf */
#include <stdarg.h> /* snprintf */

int
strprintf (char **strbuf, char **strptr, int *strlen, const char *fmt, ...)
{
  va_list va;
  const int chunk = 1024;
  int strspace = 0;
  int ret = 0;
  while (1)
    {
      // allocate more memory, if this is the first round and there was no
      // malloc before, or if this is an ineration and there was too little
      // memory before
      if ( ret != 0 || *strbuf == NULL )
        {
          *strlen += chunk;
          *strbuf = (char*)realloc (*strbuf, *strlen);
          ///printf("alloc %d bytes at %lu\n",*strlen,*strbuf);
          if (*strbuf == NULL)
            return -1;
          if (*strptr == NULL)
            *strptr = *strbuf;
        }
      // Try to print the requested data
      strspace = *strlen - (*strptr - *strbuf);
      va_start (va, fmt);
      ret = vsnprintf (*strptr, strspace, fmt, va);
      va_end (va);
      if ( ret >= 0 && ret < strspace )
        {
          *strptr += ret;
          return ret;
        }
      if ( ret >= strspace )
        *strlen += ret;
    }
}
