/* KogMo-RTDB: Real-time Database for Cognitive Automobiles
 * Copyright (c) 2003-2007 Matthias Goebl <matthias.goebl*goebl.net>
 *     Lehrstuhl fuer Realzeit-Computersysteme (RCS)
 *     Technische Universitaet Muenchen (TUM)
 * Licensed under the Apache License Version 2.0.
 */
/*
 Dynamisches mapping zwischen den aufgezeichneten Objekt-IDs und
 den Objekt-IDs, die die Objekte dann beim Abspielen in der
 Datenbank bekommen haben.
 src==0 => Map-Eintrag ist unbenutzt
 dest==0 => "Nullmapping": Objekt wird nicht abgespielt (z.B. wegen -N)
 Bei ueberschauberer Objektanzahl reicht erstmal eine lineare Suche.
 */

typedef struct
{
  kogmo_rtdb_objid_t   src;
  kogmo_rtdb_objid_t   dest;
  kogmo_rtdb_objname_t name;
  kogmo_rtdb_objtype_t otype;
} oid_map_t;

oid_map_t oid_map[KOGMO_RTDB_OIDMAPSLOTS+1];

inline static void
map_init(void)
{
  memset (&oid_map, 0, sizeof(oid_map));
}

inline static int
map_del(kogmo_rtdb_objid_t src)
{
  int i;
  for(i=0;i<KOGMO_RTDB_OIDMAPSLOTS;i++)
    {
      if (oid_map[i].src==src)
        {
          oid_map[i].src=0;
          oid_map[i].dest=0;
          oid_map[i].name[0]='\0';
          oid_map[i].otype=0;
          return 1;
        }
     }
  return 0; // not found
}

inline static int
map_add(kogmo_rtdb_objid_t src, kogmo_rtdb_objid_t dest, char *name, kogmo_rtdb_objtype_t otype)
{
  int i;
  map_del(src);
  for(i=0;i<KOGMO_RTDB_OIDMAPSLOTS;i++)
    {
      if (oid_map[i].src==0)
        {
          oid_map[i].src=src;
          oid_map[i].dest=dest;
          memcpy(oid_map[i].name,name,KOGMO_RTDB_OBJMETA_NAME_MAXLEN);
          oid_map[i].otype=otype;
          return 1;
        }
     }
  DIE("map_add: no free id-map slots available");
  return 0;
}

inline static kogmo_rtdb_objid_t
map_querydest(kogmo_rtdb_objid_t src)
{
  int i;
  for(i=0;i<KOGMO_RTDB_OIDMAPSLOTS;i++)
    {
      if (oid_map[i].src==src)
        {
          return oid_map[i].dest;
        }
     }
  return 0; // not found
}

inline static char *
map_queryname(kogmo_rtdb_objid_t src)
{
  int i;
  for(i=0;i<KOGMO_RTDB_OIDMAPSLOTS;i++)
    {
      if (oid_map[i].src==src)
        {
          return oid_map[i].name;
        }
     }
  return "?"; // not found
}

inline static kogmo_rtdb_objtype_t
map_queryotype(kogmo_rtdb_objid_t src)
{
  int i;
  for(i=0;i<KOGMO_RTDB_OIDMAPSLOTS;i++)
    {
      if (oid_map[i].src==src)
        {
          return oid_map[i].otype;
        }
     }
  return 0; // not found
}

inline static int
map_exists(kogmo_rtdb_objid_t src)
{
  int i;
  for(i=0;i<KOGMO_RTDB_OIDMAPSLOTS;i++)
    {
      if (oid_map[i].src==src)
        {
          return 1;
        }
     }
  return 0; // not found
}

#define for_each_map_entry(id) do {\
 int map_i;\
 for(map_i=0;map_i<KOGMO_RTDB_OIDMAPSLOTS;map_i++) {\
  id = oid_map[map_i].src;

#define for_each_map_entry_end }} while(0);
