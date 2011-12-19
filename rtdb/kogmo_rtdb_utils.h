/*! \file kogmo_rtdb_utils.h
 * \brief Some Utility Definitions
 *
 * Copyright (c) 2003-2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#ifndef KOGMO_RTDB_UTILS_H
#define KOGMO_RTDB_UTILS_H

#ifdef __cplusplus
extern "C" {
namespace KogniMobil {
#endif


/* Some functions to simplify the generation of strings with printf.
 Example use:

  NEWSTR(bla);
  STRPRINTF(bla,"hello world, ");
  STRPRINTF(bla,"%s %d %f\n","huhu!", 1, 23.45);
  puts(bla);
  DELETESTR(bla);

 A alternative solution would be to allocate a fixed memory area,
 but this could be too restrictive for dumps of knowledge objects.
*/

#define NEWSTR(str) int str##_len=0; char *str=NULL, *str##_ptr=NULL;
#define STRPRINTF(str,args...) strprintf(&str,&str##_ptr,&str##_len,args);
#define DELETESTR(str) free(str);

int
strprintf (char **strbuf, char **strptr, int *strlen, const char *fmt, ...);


#ifdef __cplusplus
}; /* namespace KogniMobil */
}; /* extern "C" */
#endif

#endif /* KOGMO_RTDB_UTILS_H */
