/*! \file kogmo_rtdb_udpsender.c
 * \brief Simple Client to forward Objects from the RTDB via UDP/IP
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
  kogmo_rtdb_handle_t *dbc;
  kogmo_rtdb_connect_info_t dbinfo;
  kogmo_rtdb_obj_info_t obj_info;
  kogmo_rtdb_obj_base_t *obj_p; // das ist zum begin eines jeden objekts kompatibel
  kogmo_rtdb_objtype_t otype;
  kogmo_rtdb_objid_t oid;
  kogmo_rtdb_objsize_t olen;
  int err, sentlen;
  struct addrinfo cfg,*srv;
  int fd;
  char *p;

  if ( argc != 4 )
    {
      fprintf (stderr, "Usage: kogmo_rtdb_udpsender SERVERIP UDPPORT OBJECTTYPEID\n");
      exit(1);
    }
  
  // Netzwerkverbindung initialisieren
  printf ("Sending UDP-packets to %s:%s..\n", argv[1], argv[2]);
  memset ( &cfg, 0, sizeof(struct addrinfo) ); // defaults are 0/NULL
  cfg.ai_family = AF_INET; // AF_INET6 soon..
  cfg.ai_socktype = SOCK_DGRAM; // UDP please
  cfg.ai_protocol = IPPROTO_UDP; // UDP please
  err = getaddrinfo ( argv[1], argv[2], &cfg, &srv); DIEonERR(err);
  fd = socket ( srv -> ai_addr -> sa_family, srv -> ai_socktype,
                 srv -> ai_protocol); DIEonERR(fd);
  err = connect ( fd, srv -> ai_addr, srv -> ai_addrlen ); DIEonERR(err);

  // Verbindung zur Datenbank aufbauen, unsere Zykluszeit is 33 ms
  err = kogmo_rtdb_connect_initinfo (&dbinfo, "", "c3_udpserver", 0); DIEonERR(err);
  oid = kogmo_rtdb_connect (&dbc, &dbinfo); DIEonERR(oid);

  // Auf Object warten
  otype = strtol ( argv[3], NULL, 0 );
  printf ("Waiting for Object with Type 0x%04X ..\n", otype);
  oid = kogmo_rtdb_obj_searchinfo_wait (dbc, NULL, otype, 0, 0); DIEonERR(oid);

  // Infoblock des Objektes holen und anzeigen
  err = kogmo_rtdb_obj_readinfo (dbc, oid, 0, &obj_info); DIEonERR(err);
  p = kogmo_rtdb_obj_dumpinfo_str (dbc, &obj_info);
  printf("%s", p);
  free(p);

  if ( obj_info.size_max >= 1<<16 )
    {
      printf ("Object too large!");
      exit(1);
    }

  obj_p = malloc ( obj_info.size_max );
  if ( !obj_p )
    {
      printf ("Cannot allocate memory!");
      exit(1);
    }

  // Objekt erstmalig holen (initialer Timestamp)
  err = kogmo_rtdb_obj_readdata (dbc, oid, 0, obj_p, obj_info.size_max); DIEonERR(err);
  p = kogmo_rtdb_obj_dumpbase_str (dbc, obj_p);
  printf("%s", p);
  free(p);

  while (1)
    {
      olen = kogmo_rtdb_obj_readdata_waitnext (dbc, oid, obj_p->base.committed_ts, obj_p, obj_info.size_max); DIEonERR(olen);
      p = kogmo_rtdb_obj_dumpbase_str (dbc, obj_p);
      printf("\nSending Object:\n%s", p);
      free(p);

      sentlen = send ( fd, obj_p, olen, 0);
      if ( sentlen!=olen || sentlen < 0 )
        {
          printf("FAILED: Error %i while sending udp paket.\n",-sentlen);
        }
    }

  err = kogmo_rtdb_disconnect(dbc, NULL); DIEonERR(err);

  return 0;
}
