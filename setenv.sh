# Please source this file in your shell by typing ". ./rtdbenv.sh"!

# Total amount of Memory dedicated to the Database, including all objects
# with histories,  e.g. 64MB for local tests, 500MB on kognimobil-refsys
export KOGMO_RTDB_HEAPSIZE=128M

# Name of current database: kogmo_rtdb_man will start under this name,
# clients using libkogmo_rtdb.so will connect to this database.
# If you want, you can connect your clients to someone others database by
# setting KOGMO_RTDB_DBHOST=local:$username
#export KOGMO_RTDB_DBHOST=""  # the system database is the default
export KOGMO_RTDB_DBHOST="local:`id -un`"  # your private database with your name

export KOGMO_RTDB_HOME="`pwd`"
export PATH="$KOGMO_RTDB_HOME/bin/:$PATH"
export LD_LIBRARY_PATH="$KOGMO_RTDB_HOME/lib/:$LD_LIBRARY_PATH"

# for MAC OS-X:
export DYLD_LIBRARY_PATH="$KOGMO_RTDB_HOME/lib/:$DYLD_LIBRARY_PATH"

# Here you can store your own objects:
export KOGMO_RTDB_OBJECTS="$KOGMO_RTDB_HOME/objects"
