all: objs

CC:=ccache g++
C:=ccache gcc
MAKEDEPEND:=makedepend -Y
INCS:=-I.. -I../../common -I../../../common -I../../../common_bft -I../../../../gmp -I../../../../sfs/include/sfslite #-I/usr/include/linux -I/usr/lib/gcc/i486-linux-gnu/4.3/include/

CPPFLAGS:= $(INCS) -g -Wall -DRECOVERY -Wno-non-template-friend -UPBFT_DO_SWITCHING

CFLAGS:= $(CPPFLAGS)

SINCS:= -I/usr/include/linux -I/usr/include -I/usr/lib/gcc/i486-linux-gnu/4.3/include/ -I/usr/include/c++/4.3.3

%.o:: %.cc
	$(CC) $(CPPFLAGS) -o $@ -c $<

%.o:: %.c
	$(C) $(CFLAGS) -o $@ -c $<


C_FILES=\
PBFT_R_Client.cc             PBFT_R_Replica.cc            PBFT_R_New_key.cc\
PBFT_R_Commit.cc             PBFT_R_Message.cc            PBFT_R_Reply.cc \
PBFT_R_Node.cc               PBFT_R_Request.cc \
PBFT_R_Checkpoint.cc         PBFT_R_Pre_prepare.cc        PBFT_R_Req_queue.cc \
PBFT_R_Prepare.cc            PBFT_R_Status.cc             PBFT_R_Prepared_cert.cc\
PBFT_R_Principal.cc          PBFT_R_ITimer.cc 			PBFT_R_Time.cc \
PBFT_R_Meta_data.cc          PBFT_R_Data.cc               PBFT_R_Fetch.cc \
PBFT_R_Meta_data_cert.cc     PBFT_R_State.cc              PBFT_R_pbft_libbyz.cc \
PBFT_R_View_change.cc        PBFT_R_New_view.cc           PBFT_R_View_change_ack.cc \
PBFT_R_View_info.cc          PBFT_R_NV_info.cc            PBFT_R_Rep_info.cc \
PBFT_R_Meta_data_d.cc        PBFT_R_Query_stable.cc       PBFT_R_Reply_stable.cc \
PBFT_R_Stable_estimator.cc   PBFT_R_Statistics.cc \
PBFT_R_Big_req_table.cc      PBFT_R_Pre_prepare_info.cc	


c_FILES= 

H_FILES := $(C_FILES:%.C=%.H)

h_FILES := $(c_FILES:%.c=%.h)

OBJ_FILES:=$(C_FILES:%.cc=%.o) $(c_FILES:%.c=%.o)

objs: $(OBJ_FILES)

clean:
	-rm *.o
	-rm *~

clobber:
	-rm *.o
	-rm *~

depend:
	$(MAKEDEPEND) $(INCS) $(SINCS) $(C_FILES)

# DO NOT DELETE THIS LINE -- make depend depends on it.
