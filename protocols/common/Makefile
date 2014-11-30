all: objs

CC:=ccache g++
C:=ccache gcc
MAKEDEPEND:=makedepend -Y
MAKE=gmake
INCS:=-I../../common -I../../common_bft -I../../../gmp -I../../../sfs/include/sfslite #-I/usr/include/linux -I/usr/lib/gcc/i486-linux-gnu/4.3/include/

#CPPFLAGS:= $(INCS) -g -Wall -DRECOVERY 
CPPFLAGS:= $(INCS) -g -ggdb -Wall -DRECOVERY -Wno-non-template-friend
#CPPFLAGS:= $(INCS) -O9 -funroll-loops -ffast-math -malign-double -march=pentiumpro -fomit-frame-pointer -fno-exceptions -DNDEBUG 
#CPPFLAGS:= $(INCS) -O3 -march=pentiumpro -fomit-frame-pointer -fno-exceptions -DNDEBUG   -DRECOVERY
#CPPFLAGS:= $(INCS) -O3 -march=pentiumpro -fno-exceptions -DNDEBUG   -DRECOVERY -pg
#CPPFLAGS:= $(INCS) -O3 -march=pentiumpro -fno-exceptions -DNDEBUG   -DRECOVERY 

CFLAGS:= $(CPPFLAGS)

SINCS:= -I/usr/include/linux -I/usr/include -I/usr/lib/gcc/i486-linux-gnu/4.3/include/ -I/usr/include/c++/4.3.3

%.o:: %.cc
	$(CC) $(CPPFLAGS)  -o $@ -c $<

%.o:: %.c
	$(C) $(CFLAGS) -lpthread -o $@ -c $<


C_FILES=\
Digest.cc \
DSum.cc \
Request_history.cc

c_FILES= TCP_network.c

H_FILES := $(C_FILES:%.C=%.H)

h_FILES := $(c_FILES:%.c=%.h)

OBJ_FILES:=$(C_FILES:%.cc=%.o) $(c_FILES:%.c=%.o)

objs: $(OBJ_FILES)

clean:
	-rm -f *.o
	-rm -f *~

clobber:
	-rm -f *.o
	-rm -f libfullquorum.a
	-rm -f *~

depend:
	$(MAKEDEPEND) $(INCS) $(SINCS) $(C_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.
