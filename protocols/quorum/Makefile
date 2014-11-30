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
Q_Client.cc               Q_Replica.cc \
Q_Reply.cc 					Q_Message.cc\
Q_Request.cc            Q_Node.cc \
Q_Rep_info.cc         	Q_Principal.cc \
Q_Checkpoint.cc  Q_Panic.cc Q_Abort.cc \
Q_Missing.cc Q_Get_a_grip.cc \
Q_Abort_certificate.cc \
Q_Smasher.cc \
Q_ITimer.cc Q_Time.cc \
quorum_libbyz.cc 


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
