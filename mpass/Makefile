include Makefile.defines.in

OBJFILES = $(OBJ_DIR)/mpass.o $(OBJ_DIR)/connection.o $(OBJ_DIR)/test_queue_seq.o $(OBJ_DIR)/test_queue_busy_wait.o \
           $(OBJ_DIR)/test_queue_pthread_wait.o $(OBJ_DIR)/test_dynamic_array.o $(OBJ_DIR)/test_infrastructure.o $(OBJ_DIR)/test_maysam_shm.o

.PHONY: clean

all: $(OBJ_DIR)/mpass

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/mpass: $(OBJFILES)
	$(CC) $(CCFLAGS) -o $@ $(OBJFILES) $(LDFLAGS)
	cp $(OBJ_DIR)/mpass .

$(OBJ_DIR)/mpass.o: $(OBJ_DIR) $(SRC_DIR)/mpass.c $(SRC_DIR)/test/test_queue_seq.h $(SRC_DIR)/test/test_queue_busy_wait.h
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/mpass.c -c -o $@

$(OBJ_DIR)/connection.o: $(OBJ_DIR) $(SRC_DIR)/mpass/connection.cc $(SRC_DIR)/mpass/connection.h
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/mpass/connection.cc -c -o $@

$(OBJ_DIR)/test_queue_seq.o: $(OBJ_DIR) $(SRC_DIR)/mpass/queue.h $(SRC_DIR)/test/test_queue_seq.cc
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/test/test_queue_seq.cc -c -o $@

$(OBJ_DIR)/test_queue_busy_wait.o: $(OBJ_DIR) $(SRC_DIR)/mpass/queue.h $(SRC_DIR)/test/test_queue_busy_wait.cc
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/test/test_queue_busy_wait.cc -c -o $@

$(OBJ_DIR)/test_queue_pthread_wait.o: $(OBJ_DIR) $(SRC_DIR)/mpass/queue.h $(SRC_DIR)/test/test_queue_pthread_wait.cc
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/test/test_queue_pthread_wait.cc -c -o $@

$(OBJ_DIR)/test_dynamic_array.o: $(OBJ_DIR) $(SRC_DIR)/common/dynamic_array.h $(SRC_DIR)/test/test_dynamic_array.cc
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/test/test_dynamic_array.cc -c -o $@

$(OBJ_DIR)/test_infrastructure.o: $(OBJ_DIR) $(SRC_DIR)/test/test_infrastructure.cc
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/test/test_infrastructure.cc -c -o $@

$(OBJ_DIR)/test_maysam_shm.o: $(OBJ_DIR) $(SRC_DIR)/test/test_maysam_shm.cc
	$(CPP) $(CCFLAGS) $(CPPFLAGS) $(SRC_DIR)/test/test_maysam_shm.cc -c -o $@

################
# common tasks #
################

clean:
	rm -rf $(TARGET_DIR)
	rm -f mpass


