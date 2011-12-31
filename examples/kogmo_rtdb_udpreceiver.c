/*! \file kogmo_rtdb_udpreceiver.c
 * \brief Simple Client to receive Objects from the RTDB via UDP/IP
 *
 * (c) 2006 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 */

#include <stdio.h> // printf
#include <sys/socket.h> // addrinfo
#include <netdb.h> // addrinfo
#include <sys/types.h>
#include <sys/socket.h> // recvfrom,send
#include <errno.h> // E*
#include <string.h> // strlen
#include <unistd.h> // getpid
#include <stdlib.h> // strtol
#include "kogmo_rtdb.h"

#define DIEonERR(value) if ((value)<0) { \
 fprintf(stderr,"%i DIED in %s line %i with error %i\n",getpid(),__FILE__,__LINE__,-(value));exit(1);}

int
main (int argc, char **argv)
{
  kogmo_rtdb_obj_c3_ints256_t demoobj;
  int err, recvlen;
  struct addrinfo cfg,*srv;
  int fd;

  if ( argc != 2 )
    {
      fprintf (stderr, "Usage: kogmo_rtdb_udpreceiver UDPPORT\n");
      exit(1);
    }
  
  // Netzwerkverbindung initialisieren
  printf ("Receiving UDP-packets at port %s..\n", argv[1]);
  memset ( &cfg, 0, sizeof(struct addrinfo) ); // defaults are 0/NULL
  cfg.ai_flags = AI_PASSIVE; // for servers
  cfg.ai_family = AF_INET; // AF_INET6 soon..
  cfg.ai_socktype = SOCK_DGRAM; // UDP please
  cfg.ai_protocol = IPPROTO_UDP; // UDP please
  err = getaddrinfo ( NULL, argv[1], &cfg, &srv); DIEonERR(err);
  fd = socket ( srv -> ai_addr -> sa_family, srv -> ai_socktype,
                 srv -> ai_protocol); DIEonERR(fd);
  err = bind ( fd, srv -> ai_addr, srv -> ai_addrlen); DIEonERR(err);

  while (1)
    {
      kogmo_timestamp_string_t timestring;
      recvlen = recvfrom(fd, &demoobj, sizeof(demoobj), 0, NULL, NULL); DIEonERR(recvlen);
      DIEonERR(-(recvlen!=sizeof(demoobj)));
      kogmo_timestamp_to_string(demoobj.base.data_ts, timestring);
      printf("%s: i[0]=%i, ...\n", timestring, demoobj.ints.intval[0]);
    }

  return 0;
}
