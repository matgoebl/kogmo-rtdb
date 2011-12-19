/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net,mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the GNU Lesser General Public License v3, see the file COPYING.
 */
/*! \file kogmo_time_convert.c
 * \brief Timestamp to ISO8601-Time (and back) Converter
 *
 * Copyright (c) 2006 Matthias Goebl <mg*tum.de>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */


#include "stdio.h" /* printf */
#include <unistd.h> /* sleep */
#include <stdlib.h> /* calloc */
#include "kogmo_time.h"
#include <time.h>
#include <string.h>
#include <math.h>


void
usage (void)
{
  fprintf(stderr,
"Usage: kogmo_time_convert [ISOTIME|TIMESTAMP]\n"
"Prints the given ISOTIME or TIMESTAMP as timestamp and as ISO8601-time.\n"
"With no parameters it takes the current time.\n"
"Timestamps are the nanoseconds since epoch, e.g. 1152971022779569000,\n"
"ISO8601-times with fractional portion look like 2006-07-15 15:43:42.779569000.\n"
"For a incomplete ISO8601-time the missing elements will be assumed to 0.\n\n"
"Examples:\n"
" kogmo_time_convert '2006-07-15 15:43:42.779569000'\n"
" kogmo_time_convert 1152971022779569000\n"
"\n");
  exit(1);
}

int
main (int argc, char **argv)
{
  kogmo_timestamp_t ts;
  kogmo_timestamp_string_t tstring;
  int err;

  if ( argc > 1 )
    {
      ts = kogmo_timestamp_from_string ( argv[1] );
      if ( ts == 0 )
        usage ();
    }
  else
    {
      ts = kogmo_timestamp_now ();
    }

  printf ("%lli\n", (long long int)ts);
  err = kogmo_timestamp_to_string (ts, tstring);
  if ( !err )
    printf ("%s\n", tstring);

  return 0;
}
