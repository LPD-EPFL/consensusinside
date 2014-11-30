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
M_Client.cc               M_Replica.cc \
M_Reply.cc 					M_Message.cc\
M_Prepare.cc 					\
M_Request.cc            M_Node.cc \
M_Rep_info.cc         	M_Principal.cc \
M_Time.cc \
multipaxos_libbyz.cc \
M_Abandon.cc \
M_Accept.cc \
M_PrepareResponse.cc \
M_Learn.cc


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
