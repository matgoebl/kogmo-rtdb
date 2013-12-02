project( kogmo_rtdb )

cmake_minimum_required(VERSION 2.8)

# Remove definitions that would include UniBw customizations to RTDB
# (see tas_build.cmake)
remove_definitions( ${RTDB_OBJECT_DEFS} )

file( GLOB RTDB_FUNCS ./objects/kogmo_rtdb_obj_*_funcs.c )
file( GLOB RTDB_RECORD ./record/kogmo_rtdb_avirawcodec.c ./record/kogmo_rtdb_timeidx.c )

set(SOURCES
    rtdb/kogmo_rtdb_obj_local.c
    rtdb/kogmo_rtdb_ipc_posix.c
    rtdb/kogmo_rtdb_rtmalloc.c
    rtdb/kogmo_time.c
    rtdb/kogmo_rtdb_helpers.c
    rtdb/kogmo_rtdb_housekeeping.c
    rtdb/kogmo_rtdb_obj_base_funcs.c
    rtdb/kogmo_rtdb_objdata.c
    rtdb/kogmo_rtdb_objmeta.c
    rtdb/kogmo_rtdb_objdata_slot.c
    rtdb/kogmo_rtdb_trace.c
    rtdb/kogmo_rtdb_utils.c
    rtdb/rtmalloc/suba.c
    ${RTDB_FUNCS}
    ${RTDB_RECORD}
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${CMAKE_CURRENT_SOURCE_DIR}/rtdb/
    ${CMAKE_CURRENT_SOURCE_DIR}/objects/
)
add_definitions(
    -DRTMALLOC_suba
    -DKOGMO_RTDB_INCLUDE_A2_OBJECTS
    -fPIC
    -c
    -Wall
    -O2
    -g
    -Wstrict-prototypes
    -fno-strict-aliasing
    -D_GNU_SOURCE
    -D_REENTRANT
)

# Build libkogmo_rtdb
add_library( kogmo_rtdb SHARED ${SOURCES} )

if(macx)
    target_link_libraries( kogmo_rtdb pthread m c)
else()
    target_link_libraries( kogmo_rtdb rt pthread m c)
endif()

FILE( GLOB API_HEADERS "include/*" )
install( FILES ${API_HEADERS} DESTINATION "include/kogmo_rtdb/" )
#set( OBJ_HEADERS
#    ${CMAKE_CURRENT_SOURCE_DIR}/objects/kogmo_rtdb_obj_base.h
#        ${CMAKE_CURRENT_SOURCE_DIR}/objects/kogmo_rtdb_obj_base_funcs.h
#        ${CMAKE_CURRENT_SOURCE_DIR}/objects/kogmo_rtdb_obj_funcs.h
#        ${CMAKE_CURRENT_SOURCE_DIR}/objects/kogmo_rtdb_obj_packing.h
#    )
#install( FILES ${OBJ_HEADERS} DESTINATION "include/kogmo_rtdb/base" )

# Install for default target (libraries should do so)
add_custom_command( TARGET kogmo_rtdb
        POST_BUILD
        COMMAND make install
)




project( kogmo_rtdb_applications )

include_directories( BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/rtdb/
        ${CMAKE_CURRENT_SOURCE_DIR}/objects/
)
add_definitions(
    -DRTMALLOC_suba
    -DKOGMO_RTDB_INCLUDE_A2_OBJECTS
    -fPIC
    -c
    -Wall
    -O2
    -g
    -Wstrict-prototypes
    -D_GNU_SOURCE
    -D_REENTRANT
)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)


# Build kogmo_rtdb_man
add_executable( kogmo_rtdb_man rtdb/kogmo_rtdb_man.c )
target_link_libraries( kogmo_rtdb_man kogmo_rtdb )

# Build kogmo_rtdb_dump
add_executable( kogmo_rtdb_dump rtdb/kogmo_rtdb_dump.c )
target_link_libraries( kogmo_rtdb_dump kogmo_rtdb )

# Build kogmo_rtdb_play
add_executable( kogmo_rtdb_play record/kogmo_rtdb_play.c )
target_link_libraries( kogmo_rtdb_play kogmo_rtdb )

# Build kogmo_rtdb_record
add_executable( kogmo_rtdb_record record/kogmo_rtdb_record.c  )
target_link_libraries( kogmo_rtdb_record kogmo_rtdb )

# Build kogmo_time_convert
add_executable( kogmo_time_convert rtdb/kogmo_time_convert.c )
target_link_libraries( kogmo_time_convert kogmo_rtdb )

# Build kogmo_rtdb_reader
add_executable( kogmo_rtdb_reader examples/kogmo_rtdb_reader.c )
target_link_libraries( kogmo_rtdb_reader kogmo_rtdb )

# Build kogmo_rtdb_writer
add_executable( kogmo_rtdb_writer examples/kogmo_rtdb_writer.c )
target_link_libraries( kogmo_rtdb_writer kogmo_rtdb )

# All of RTDB
add_custom_target( rtdb )
add_dependencies( rtdb
    kogmo_rtdb
    kogmo_rtdb_man
    kogmo_rtdb_dump
    kogmo_rtdb_record
    kogmo_time_convert
    kogmo_rtdb_play
    kogmo_rtdb_reader
    kogmo_rtdb_writer
)
