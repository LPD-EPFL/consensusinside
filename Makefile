AR:=ar
RANLIB:=ranlib
MAKEDEPEND:=makedepend -Y

CFLAGS = -ggdb -g -lrt
SUBDIRS = common 
SUBDIRS += modular_bft 
SUBDIRS += protocols/common
SUBDIRS += protocols/multipaxos
SUBDIRS += protocols/oneacceptor 
SUBDIRS += protocols/tpc 
#SUBDIRS += protocols/quorum 
#SUBDIRS += protocols/zlight
#SUBDIRS += protocols/ring
#SUBDIRS += protocols/chain 
#SUBDIRS += protocols/pbft 
#SUBDIRS += protocols/pbft/replica
#SUBDIRS += protocols/pbft/client 

subdirs:
	@for i in $(SUBDIRS); do \
	#echo "make all in $$i..."; \
	$(MAKE) -C $$i ${MAKE_OPT}; \
	if [ $$? != 0 ]; then exit 2; fi; \
	done

#lib.a: subdirs
#	$(AR) rcv $@ `find . -name *.o` 
#	changed by Maysam, it is not necessary and it raised a conflict with the main file of libtask
lib.a: subdirs
	$(AR) rcv $@ `find . -name *.o | grep -v "^./benchmarks/"` 
	$(RANLIB) $@
	mv lib.a benchmarks/

libtask.a: 
	make -C libtask

all: lib.a libtask.a
	$(MAKE) -C benchmarks ${MAKE_OPT}

clean:
	@for i in $(SUBDIRS); do \
	#echo "make clean in $$i..."; \
	make -C $$i clean; \
	done
	make -C benchmarks clean
	make -C libtask clean
	
