all: objs

CC:=ccache g++
C:=ccache gcc
MAKEDEPEND:=makedepend -Y
INCS:=-I../common -I../../common -I../../common_bft -I../../../gmp -I../../../sfs/include/sfslite -I../../libtask  #-I/usr/include/linux -I/usr/lib/gcc/i486-linux-gnu/4.3/include/

CPPFLAGS:= $(INCS) -g -Wall -DRECOVERY -Wno-non-template-friend

CFLAGS:= $(CPPFLAGS)

SINCS:= -I/usr/include/linux -I/usr/include -I/usr/lib/gcc/i486-linux-gnu/4.3/include/ -I/usr/include/c++/4.3.3

%.o:: %.cc
	$(CC) $(CPPFLAGS)  -o $@ -c $<

%.o:: %.c
	$(C) $(CFLAGS) -o $@ -c $<


C_FILES=\
O_Client.cc               O_Replica.cc \
O_Reply.cc 					O_Message.cc\
O_Prepare.cc 					\
O_Request.cc            O_Node.cc \
O_Rep_info.cc         	O_Principal.cc \
O_Time.cc \
oneacceptor_libbyz.cc \
O_PrepareResponse.cc \
O_Abandon.cc \
O_M_Replica.cc \
O_AcceptedProposals.h


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
	-rm -f *~

depend:
	$(MAKEDEPEND) $(INCS) $(SINCS) $(C_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.
