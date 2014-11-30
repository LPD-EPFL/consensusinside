all: objs

CC:=ccache g++
C:=ccache gcc
MAKEDEPEND:=makedepend 
INCS:=-I../common -I../common_bft -I../abstract -I../protocols/quorum -I../protocols/tpc -I../protocols/oneacceptor -I../protocols/multipaxos -I../protocols/zlight -I../protocols/chain -I../protocols/ring -I../protocols/pbft -I../protocols/pbft/replica -I../protocols/pbft/client #-I../../gmp -I../../sfs/include/sfslite 

CPPFLAGS:= $(INCS) -g -Wall -DRECOVERY -Wno-non-template-friend -lrt

CFLAGS:= $(CPPFLAGS)

SINCS:= -I/usr/include/linux -I/usr/include -I/usr/lib/gcc/i486-linux-gnu/4.3/include/ -I/usr/include/c++/4.3.3

%.o:: %.cc
	$(CC) $(CPPFLAGS)  -o $@ -c $<

%.o:: %.c
	$(C) $(CFLAGS) -o $@ -c $<


C_FILES=\
libmodular_BFT.cc

c_FILES= 

H_FILES := $(C_FILES:%.C=%.H)

h_FILES := $(c_FILES:%.c=%.h)

OBJ_FILES:=$(C_FILES:%.cc=%.o) $(c_FILES:%.c=%.o)

objs: $(OBJ_FILES)

clean:
	-rm -f *.o
	-rm -f *~

clobber:
	-rm -f *.o
	-rm -f libmodular_BFT.a
	-rm -f *~

depend:
	$(MAKEDEPEND) $(INCS) $(SINCS) $(C_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.
