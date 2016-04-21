KogMo-RTDB
==========

This is KogMo-RTDB, the Real-Time Database for Cognitive Automobiles.

Copyright (c) 2003-2009 Matthias Goebl <matthias.goebl*goebl.net>  
Lehrstuhl fuer Realzeit-Computersysteme (RCS)  
Technische Universitaet Muenchen (TUM)

Licensed under the Apache License Version 2.0.


First steps
-----------

1. Set defaults including instance name of your database: `source ./setenv.sh`

2. Have a look at `kogmo_rtdb_man -h`:

```
KogMo-RTDB: Real-time Database for Cognitive Automobiles - www.kogmo-rtdb.de
Copyright (c) 2003-2011 Matthias Goebl <matthias.goebl*goebl.net>
Lehrstuhl fuer Realzeit-Computersysteme, Technische Universitaet Muenchen
Licensed under the Apache License Version 2.0.
Includes suba allocator by M.Allen (MIT License)
Release 616+nonrtmallocgit://github.com/matgoebl/kogmo-rtdb.git (2011-12-31 09:21:50 +0100)
Usage: kogmo_rtdb_man [.....]
 -n        no-daemon: don't fork into background
 -d        debugging: show debugging information, implies -n
 -s        start in simulation mode (arbitrary commit-timestamps and time)
 -S SIZE   create database with the given size, defaults to 67108864 bytes,
           overrides the environment variable KOGMO_RTDB_HEAPSIZE
 -H DBHOST create database with the given name, must begin with 'local:',
           eg. 'local:bla'. defaults to 'local:system', overrides the 
           environment variable KOGMO_RTDB_DBHOST
 -k        kill old database (specified by -H or KOGMO_RTDB_DBHOST) and exit
 -h        print this help message
```

3. Start the KogMo-RTDB manager process: `kogmo_rtdb_man` (it backgrounds itself by default)
```
KogMo-RTDB: Real-time Database for Cognitive Automobiles - www.kogmo-rtdb.de
Copyright (c) 2003-2011 Matthias Goebl <matthias.goebl*goebl.net>
Lehrstuhl fuer Realzeit-Computersysteme, Technische Universitaet Muenchen
Licensed under the Apache License Version 2.0.
Includes suba allocator by M.Allen (MIT License)
Release 616+nonrtmallocgit://github.com/matgoebl/kogmo-rtdb.git (2011-12-31 09:21:50 +0100)
wait..
OK - connection to just started RTDB successful.
```

4. Have a look at the KogMo-RTDB inspection tool: `kogmo_rtdb_dump -h`
```
KogMo-RTDB Dump (Rev.616) (c) Matthias Goebl <matthias.goebl*goebl.net> RCS-TUM
Usage: kogmo_rtdb_dump [.....]
 -m       dump manager info
 -p       dump connected processes
 -T TS    select data at or just before Timestamp or Time TS
 -i ID    dump object with id ID
  -Y TS   select data that's younger (or equal) than Timestamp or Time TS
  -O TS   select data that's older (or equal) than Timestamp or Time TS
  -R      include data submit history
  -x      include hex dump of object data
 -n NAME  dump object with name NAME (regular expression if starting with ~)
 -r       show free resources
 -a       dump all info
 -q       be quiet
 -w       wait for return before leaving and deleting own test-object
 -t       object list as tree
 -d       show database infos
 -D LVL   use debug-level LVL
 -c NAME  connect as process with name NAME
 -o NAME  create test-object with name NAME
 -W       if there is no database, wait until it gets started
 -H DBHOST connect to database with the given name, must begin with 'local:',
          eg. 'local:yourname'. defaults to 'local:system', overrides the 
          environment variable KOGMO_RTDB_DBHOST
 -v       show version and copyright
 -h       print this help message
```

4. Use the KogMo-RTDB inspection tool to dump the object tree: `kogmo_rtdb_dump -t`=
```
*** Dump of Object-Tree
+--  root: OID 1, TYPE 0xC30001, OWN c3_rtdb_manager(3), UPDATE unknown (no data)
  +--  processes: OID 2, TYPE 0xC30002, OWN c3_rtdb_manager(3), UPDATE unknown (no data)
    +--  c3_rtdb_manager: OID 3, TYPE 0xC30003, OWN c3_rtdb_manager(3), UPDATE 2016-04-21 20:37:55.765936867 by c3_rtdb_manager(3)
      +--  rtdb: OID 4, TYPE 0xC30004, OWN c3_rtdb_manager(3), UPDATE 2016-04-21 20:37:55.765934447 by c3_rtdb_manager(3)
    +--  c3_dump: OID 6, TYPE 0xC30003, OWN c3_dump(6), UPDATE 2016-04-21 20:37:55.791324894 by c3_dump(6)
```

5. Have a more detailed view: `kogmo_rtdb_dump -a`
```
*** KogMo-RTDB: Real-time Database for Cognitive Automobiles ***
KogMo-RTDB: Real-time Database for Cognitive Automobiles - www.kogmo-rtdb.de
Copyright (c) 2003-2011 Matthias Goebl <matthias.goebl*goebl.net>
Lehrstuhl fuer Realzeit-Computersysteme, Technische Universitaet Muenchen
Licensed under the Apache License Version 2.0.
Includes suba allocator by M.Allen (MIT License)
Release 616+nonrtmallocgit://github.com/matgoebl/kogmo-rtdb.git (2011-12-31 09:21:50 +01* Revision of Database: 616 (2011-12-31 09:21:50 +0100)
* Database Host: local:matthias
* Timestamp of Database Start: 2016-04-21 20:37:29.504477969
* Timestamp of Database Dump:  2016-04-21 20:38:19.453061734
* Timestamp of Current Time:   2016-04-21 20:38:19.453112877
* Memory free:    127/127 MB
* Objects free:   994/1000
* Processes free: 100/100

*** Manager:
PID 7357, last activity 0.177 seconds ago

*** Connected Processes:
1. c3_rtdb_manager: PROC_OID 1, OID 3, PID 7357, TID 3075705536, Flags 0x1000, connected 2016-04-21 20:37:29.504477969, inactive -1461263899.277 seconds
2. c3_dump: PROC_OID 7, OID 10, PID 7994, TID 3075177152, Flags 0x4, connected 2016-04-21 20:38:19.452495402, inactive -1461263899.453 seconds

*** Dump of Object(s) with name '~.':
* INFO:
Object: root (1)  Type: 0xC30001  Max.Size: 0  Parent: 0
Created: 2016-04-21 20:37:29.504440847 by c3_rtdb_manager(3)
Deleted: no
History: 1.000000 secs with 1.000000 s cycle time min. (1.000000 max.), slot 0/0 Buffer 0
(no object data yet)

* INFO:
Object: processes (2)  Type: 0xC30002  Max.Size: 0  Parent: 1
Created: 2016-04-21 20:37:29.504469835 by c3_rtdb_manager(3)
Deleted: no
History: 1.000000 secs with 0.001000 s cycle time min. (1.000000 max.), slot 0/0 Buffer 0
(no object data yet)

* INFO:
Object: c3_rtdb_manager (3)  Type: 0xC30003  Max.Size: 212  Parent: 2
Created: 2016-04-21 20:37:29.504477969 by c3_rtdb_manager(3)
Deleted: no
History: 10.000000 secs with 0.250000 s cycle time min. (0.250000 max.), slot 37/41 Buffer 1104
* DATA:
Committed: 2016-04-21 20:38:19.276719063 by c3_rtdb_manager(3), 212 bytes
Data created: 2016-04-21 20:37:29.355490144

* INFO:
Object: rtdb (4)  Type: 0xC30004  Max.Size: 524  Parent: 3
Created: 2016-04-21 20:37:29.504492494 by c3_rtdb_manager(3)
Deleted: no
History: 10.000000 secs with 0.250000 s cycle time min. (1.000000 max.), slot 37/41 Buffer 9808
* DATA:
Committed: 2016-04-21 20:38:19.276713322 by c3_rtdb_manager(3), 524 bytes
Data created: 2016-04-21 20:38:19.276712539

* INFO:
Object: c3_dump (10)  Type: 0xC30003  Max.Size: 212  Parent: 2
Created: 2016-04-21 20:38:19.452495402 by c3_dump(10)
Deleted: no
History: 10.000000 secs with 1.000000 s cycle time min. (1.000000 max.), slot 1/11 Buffer 31304
* DATA:
Committed: 2016-04-21 20:38:19.452531962 by c3_dump(10), 212 bytes
Data created: 2016-04-21 20:38:19.452425800
```

6. Start an example client, that waits for data: `kogmo_rtdb_reader`  
   (nothing happens so far, no output, just waits for data)

7. Open another console, set defaults `source ./setenv.sh` and start an example client that sends data: `kogmo_rtdb_writer`

8. Immediately the blocked reader comes to life:
```
2016-04-21 20:39:26.150462311: i[0]=2, ...
2016-04-21 20:39:27.150543206: i[0]=3, ...
2016-04-21 20:39:28.150623525: i[0]=4, ...
2016-04-21 20:39:29.150697223: i[0]=5, ...
2016-04-21 20:39:30.150774722: i[0]=6, ...
2016-04-21 20:39:31.150852297: i[0]=7, ...
2016-04-21 20:39:32.150929491: i[0]=8, ...
2016-04-21 20:39:33.151003629: i[0]=9, ...
```

9. Have a look at the new object tree: `kogmo_rtdb_dump -t`
```
*** Dump of Object-Tree
+--  root: OID 1, TYPE 0xC30001, OWN c3_rtdb_manager(3), UPDATE unknown (no data)
  +--  processes: OID 2, TYPE 0xC30002, OWN c3_rtdb_manager(3), UPDATE unknown (no data)
    +--  c3_rtdb_manager: OID 3, TYPE 0xC30003, OWN c3_rtdb_manager(3), UPDATE 2016-04-21 20:39:58.322763415 by c3_rtdb_manager(3)
      +--  rtdb: OID 4, TYPE 0xC30004, OWN c3_rtdb_manager(3), UPDATE 2016-04-21 20:39:58.322760531 by c3_rtdb_manager(3)
    +--  c3_dump: OID 14, TYPE 0xC30003, OWN c3_dump(14), UPDATE 2016-04-21 20:39:58.452046626 by c3_dump(14)
    +--  demo_object_writer: OID 12, TYPE 0xC30003, OWN demo_object_writer(12), UPDATE 2016-04-21 20:39:25.150367444 by demo_object_writer(12)
  +--  demo_object: OID 13, TYPE 0xC30051, OWN demo_object_writer(12), UPDATE 2016-04-21 20:39:58.153185304 by demo_object_writer(12)
```

*demo_object* is the new object, that contains above data.

10. Stop the system: `kogmo_rtdb_man -k`
```
KogMo-RTDB: Real-time Database for Cognitive Automobiles - www.kogmo-rtdb.de
Copyright (c) 2003-2011 Matthias Goebl <matthias.goebl*goebl.net>
Lehrstuhl fuer Realzeit-Computersysteme, Technische Universitaet Muenchen
Licensed under the Apache License Version 2.0.
Includes suba allocator by M.Allen (MIT License)
Release 616+nonrtmallocgit://github.com/matgoebl/kogmo-rtdb.git (2011-12-31 09:21:50 +0100)

I will try to kill the database by sending the manager a signal.
The manager will then terminate all its clients (SIGTERM, 2 seconds, SIGKILL).
If this fails, a stale database might hang around.
To completely remove your database:
- Look for processes that are still connected:
  Enter '/usr/sbin/lsof | grep /dev/shm/kogmo'
  (/dev/shm/kogmo.matthias.goebl = database local:matthias.goebl)
- Kill them (this includes the correspondig manager process)
- Remove the shared memory segment ('rm /dev/shm/kogmo.matthias.goebl')
  Only removing the file does neither frees the memory nor kills the processes!

OK: Found old database, sending the manager (pid 7357) the termination signal..
OK: Signal delivered.
OK: Manager is terminated.
```


Releases
---------

##### Release 2009-08-17
This is the first published LGPL version of this software.
I'm currently working on code and documentation cleanup.
An improved package with more examples, gui tools and
instructions for hard real-time use will follow in september 2009.

##### Release 2009-11-20
This is the second published LGPL version of this software.
The code has been cleaned up and the package now contains
more examples and gui tools. The documentation will be cleaned
up and improved soon.

##### Release 2011-12-31
Changed license from LGPL to Apache License Version 2.0.
Added udprmon and tlsf allocator.
Added some documentation and minor updates.
