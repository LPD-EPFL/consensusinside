all: objs

CC:=ccache g++
C:=ccache gcc
MAKEDEPEND:=makedepend -Y
INCS:=-I../common -I../../common -I../../common_bft -I../../../gmp -I../../../sfs/include/sfslite #-I/usr/include/linux -I/usr/lib/gcc/i486-linux-gnu/4.3/include/
DEFINES=-UREPLY_BY_PRIMARY -DRECOVERY -DUSE_MACS

CPPFLAGS:= $(INCS) -g -Wall $(DEFINES) 

CFLAGS:= $(CPPFLAGS)

SINCS:= -I/usr/include/linux -I/usr/include -I/usr/lib/gcc/i486-linux-gnu/4.3/include/ -I/usr/include/c++/4.3.3

%.o:: %.cc
	$(CC) $(CPPFLAGS)  -o $@ -c $<

%.o:: %.c
	$(C) $(CFLAGS) -o $@ -c $<


R_FILES=\
SuperFastHash.cc \
R_Client.cc           R_Replica.cc        R_Message.cc\
R_Reply.cc 				R_Mes_queue.cc 		R_Principal.cc\
R_Request.cc		  R_ACK.cc \
R_Deliver.cc \
R_Node.cc\
R_Checkpoint.cc \
R_Rep_info.cc		  ring_libbyz.cc


c_FILES=

H_FILES := $(R_FILES:%.C=%.H)

h_FILES := $(c_FILES:%.c=%.h)

OBJ_FILES:=$(R_FILES:%.cc=%.o) $(c_FILES:%.c=%.o)

objs: $(OBJ_FILES)

clean:
	-rm -f *.o
	-rm -f *~

clobber:
	-rm -f *.o
	-rm -f *~

depend:
	$(MAKEDEPEND) $(INCS) $(SINCS) $(R_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

