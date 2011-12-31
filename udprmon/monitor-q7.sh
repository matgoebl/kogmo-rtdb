#!/bin/bash

#IP=129.187.45.169 #refsys
#IP=192.168.1.1 #q7
IP=129.187.45.33
PORT=28999
kogmo_rtdb_man -S 40m

c3_gui_videoannot -geometry -0-0 &  
a3_gui_sickverysimple -geometry -0+0 >/dev/null &
c3_gui_rtdbview -geometry +0-0 &

#export DEBUG=1
export BPS=1
udpsimpleclient $IP $PORT 0.5 \
 processes:0xC30002=remote_processes :0xC30003 rtdb:0xC30004=remote_rtdb \
 LidarData_Front :0xA30001 :0xA30003 :0xD10900 \
 insdata gpsdata:=gpsdata-q7 \
 VehicleControl VehicleFeedback vehicle_maneuver_request vehicle_maneuver_response c3_vehiclestatus:=c3_vehiclestatus-q7 \
 image_right image_left image_tele right_image left_image tele_image \
 :0xA20034

# c3_gui_rtdbview &
# udpsimpleserver &
# GUI_GPSMAP_AUTOCLEAR=1 GUI_UPDATE_FREQ=1 c3_gui_gpsmap &
# GPSDEV=/dev/ttyUSB0 GPSBAUD=4800 GPSOBJ=gpsdata_aklasse c3_gps_interface &
# udpsimpleclient 192.168.1.1 64000 0.5 gpsdata gpsdata_mouse_ublox &
# udpsimpleclient 192.168.1.1 64000 0.5 left_image

#Q7: udpsimpleserver &  udpsimpleclient 192.168.1.1 64000 0.2 gpsdata_aklasse &

#matthias@eee:
# sudo ifconfig ra0 192.168.1.210; sudo ping 192.168.1.230
# udpsimpleclient 192.168.1.1 64000 0.2 gpsdata_chasecar01
# BPS=1 udpsimpleclient 192.168.1.230 64000 0.2 :

# sudo iwconfig wlan0 channel 11 essid KogniMobil mode ad-hoc
# sudo ifconfig wlan0 192.168.28.111
# ping 192.168.28.1
