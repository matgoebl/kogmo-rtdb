#!/bin/bash

# udpmon.sh must be sourced! ". ./udpmon.sh"
# if [ -f udprmon/udpmon.sh ]; then source udprmon/udpmon.sh; fi

udpmon () {
 case "$1" in
  -h|'')
     echo "udpmon [-u|-s|HOST[:PORT]]]"
     ;;
  -u)
     unset LD_PRELOAD
     unset GUI_UPDATE_FREQ
     unset GUI_KOGMO_RTDB_NO_PTR_READ
     KOGMO_RTDB_DEBUG="$KOGMO_RTDB_DEBUG_saved"
     KOGMO_RTDB_DBHOST="$KOGMO_RTDB_DBHOST_saved"
     echo "UDP Remote Monitoring Library deactivated. Restored old db-host: $KOGMO_RTDB_DBHOST"
     ;;
  -s)
     kogmo_rtdb_dump -r
     ;;
  *)
     host="$1"
     [ "$host" = "refsys" ] && host=129.187.45.169
     [ "$host" = "kmg" ] && host=129.187.45.33:28999 #kogmomuc in garching
     KOGMO_RTDB_DEBUG_saved="$KOGMO_RTDB_DEBUG"
     KOGMO_RTDB_DBHOST_saved="$KOGMO_RTDB_DBHOST"
     export KOGMO_RTDB_DBHOST="udpmon:$host"
     export KOGMO_RTDB_DEBUG=0
     export GUI_UPDATE_FREQ=1
     export GUI_KOGMO_RTDB_NO_PTR_READ=1
     export LD_PRELOAD=""
     X=`pwd`/libkogmo_rtdb_udpmon.so.1; [ -f "$X" ] && LD_PRELOAD="$X"
     X=`pwd`/udprmon/libkogmo_rtdb_udpmon.so.1; [ -f "$X" ] && LD_PRELOAD="$X"
     if [ -z "$LD_PRELOAD" ]; then
       echo "ERROR: cannot find libkogmo_rtdb_udpmon.so.1";
     else
       if kogmo_rtdb_dump -r; then
         echo "OK. UDP Remote Monitoring Library activated. Now using $KOGMO_RTDB_DBHOST"
       fi
     fi
     ;;
  esac
}
