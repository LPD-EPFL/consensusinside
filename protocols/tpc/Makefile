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
T_Client.cc               T_Replica.cc \
T_Reply.cc 					T_Message.cc\
T_Ack.cc 					T_Commit.cc\
T_Request.cc            T_Node.cc \
T_Rep_info.cc         	T_Principal.cc \
T_Time.cc \
tpc_libbyz.cc \
T_CommitAck.cc


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
