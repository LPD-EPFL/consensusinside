all: thr_client thr_manager server multicast_replica


CC:=ccache g++
C:=ccache gcc
AR:=ar
RANLIB:=ranlib
MAKEDEPEND:=makedepend 
INCS:=-I../common -I../common_bft -I../abstract -I../modular_bft -I../protocols/common -I../protocols/quorum -I../protocols/tpc -I../protocols/oneacceptor -I../protocols/multipaxos -I../protocols/chain -I../protocols/ring -I../libtask #-I../../gmp -I../../sfs/include/sfslite #-I/usr/include/linux -I/usr/lib/gcc/i486-linux-gnu/4.3/include/
CPPFLAGS:= $(INCS) -g -ggdb -Wall -lrt
CFLAGS:= $(CPPFLAGS) -g --std=c99
CFLAGS_SAFE:= $(CPPFLAGS) -g
#LIBDIRS:=-L../gmp -L../../sfs/lib/sfslite 
# THis doesn't work for chain, pbft
#LIBS:=lib.a -lsfscrypt -lasync -lgmp -lpthread -ltcmalloc
#LIBS:=lib.a -lsfscrypt -lasync -lgmp -lpthread
#Added By Maysam Yabandeh
#LIBS:=lib.a ../libtask/libtask.a -lsfscrypt -lasync -lgmp -lpthread
LIBS:=lib.a ../libtask/libtask.a -lpthread
#LIBS:=lib.a -lsfscrypt -lasync -lgmp -lpthread

SINCS:= -I/usr/include/linux -I/usr/include -I/usr/lib/gcc/i486-linux-gnu/4.3/include -I/usr/include/c++/4.3.3 

%.o:: %.cc
	$(CC) $(CPPFLAGS) -o $@ -c $<

multicast_replica.o: multicast_replica.c
	$(C) $(CFLAGS_SAFE) -o $@ -c $<

%.o:: %.c
	$(C) $(CFLAGS) -o $@ -c $<

MAKE=gmake

C_FILES=\
thr_manager.cc incremental_stats.cc 

c_FILES=\
replica_main.c \
multicast_replica.c \
thr_client.cc

H_FILES := $(C_FILES:%.C=%.H)

h_FILES := $(c_FILES:%.c=%.h)

OBJ_FILES:=$(C_FILES:%.cc=%.o) $(c_FILES:%.c=%.o)

objs: $(OBJ_FILES)

clean:
	-rm -f *.o
	-rm -f *~

clobber:
	-rm -f thr_manager thr_client* server 
	-rm -f *.o
	-rm -f *~

depend:
	$(MAKEDEPEND) $(INCS) $(SINCS) $(C_FILES)

thr_manager: lib.a thr_manager.o incremental_stats.o
	$(CC) -o thr_manager $(CPPFLAGS) $(LIBDIRS) thr_manager.o incremental_stats.o $(LIBS)

thr_manager_aliph: lib.a thr_manager_aliph.o incremental_stats.o
	$(CC) -o thr_manager_aliph $(CPPFLAGS) $(LIBDIRS) thr_manager_aliph.o incremental_stats.o $(LIBS)

server: lib.a replica_main.o incremental_stats.o
	$(CC) -o server $(CPPFLAGS) $(LIBDIRS) replica_main.o incremental_stats.o $(LIBS)

thr_client: lib.a thr_client.o incremental_stats.o
	$(CC) -o thr_client $(CPPFLAGS) $(LIBDIRS) thr_client.o incremental_stats.o $(LIBS) 

thr_client_aliph: lib.a thr_client_aliph.o incremental_stats.o
	$(CC) -o thr_client_aliph $(CPPFLAGS) $(LIBDIRS) thr_client_aliph.o incremental_stats.o $(LIBS) 

multicast_replica: lib.a multicast_replica.o
	$(CC) -o multicast_replica $(CPPFLAGS) $(LIBDIRS) multicast_replica.o $(LIBS) -lrt

# DO NOT DELETE THIS LINE -- make depend depends on it.
