
CC = gcc
CFLAGS = -Wall -Isrc -D_GNU_SOURCE -Iinclude -std=c99 -g

.PHONY: all build clean graphs main pthread_main

SRC_DIR = src
TST_DIR = tst
INSTALL_DIR = install
MAIN_PROGRAM = main.o

SRCS = ${SRC_DIR}/*.c
MAIN_OBJ = 01-main.o
SWITCH_OBJ = 02-switch.o
EQUITY_OBJ = 03-equity.o
JOIN_OBJ = 11-join.o
JOIN-MAIN_OBJ = 12-join-main.o
CREATE-MANY = 21-create-many.o
CREATE-MANY-RECURSIVE = 22-create-many-recursive.o
CREATE-MANY-ONCE = 23-create-many-once.o
SWITCH_MANY = 31-switch-many.o
SWITCH_MANY_JOIN = 32-switch-many-join.o
SWITCH_MANY_CASCADE = 33-switch-many-cascade.o
FIBONACCI = 51-fibonacci.o
MUTEX = 61-mutex.o
MUTEX_2 = 62-mutex.o
MUTEX_2 = 62-mutex.o
MUTEX_3 = 63-mutex-equity.o
PREEMPTION = 71-preemption.o
DEADLOCK = 81-deadlock.o


%.o: ${SRC_DIR}/%.c 
	${CC} ${CFLAGS} -fPIC -c $< -o $@

%_p.o: ${SRC_DIR}/%.c 
	${CC} ${CFLAGS} -fPIC -DUSE_PTHREAD -c $< -o $@
	
%.o: ${TST_DIR}/%.c
	${CC} ${CFLAGS} -fPIC -c $< -o $@
	
thread_with_enable_preemption.o: src/thread.c 
	${CC} ${CFLAGS} -DENABLEPREEMPTION -fPIC -c $< -o $@

all: install

install: main ${MAIN_OBJ} ${MAIN_OBJ_p} ${SWITCH_OBJ} ${EQUITY_OBJ} ${JOIN_OBJ} ${JOIN-MAIN_OBJ} ${CREATE-MANY} ${CREATE-MANY-RECURSIVE} ${CREATE-MANY-ONCE} ${SWITCH_MANY} ${SWITCH_MANY_JOIN} ${SWITCH_MANY_CASCADE} ${FIBONACCI} ${MUTEX} ${MUTEX_2} ${PREEMPTION} ${DEADLOCK} ${SIGNAL} ${MUTEX_3} thread_with_enable_preemption.o build
main: thread.o ${MAIN_PROGRAM} 
	${CC} ${CFLAGS} $^ -o $@

pthread_main: ${SRC_DIR}/main.c ${SRC_DIR}/thread.h 
	${CC} ${CFLAGS} $^ -o $@ -DUSE_PTHREAD -lpthread 

run: install
	@echo "Running all executables..."
	@for file in $(wildcard $(INSTALL_DIR)/bin/*); do \
        echo "Running $$file"; \
        if echo $$file | grep -q -E "install/bin/(2|3|5)"; then \
            if [ $$file = install/bin/31-switch-many ] || [ $$file = install/bin/32-switch-many-join ] || [ $$file = install/bin/33-switch-many-cascade ]; then \
                ./$$file 20 30; \
            else \
                ./$$file 20; \
            fi; \
		elif [ $$file = install/bin/71-preemption ]; then \
            ./$$file 20 5; \
        else \
            ./$$file; \
        fi; \
    done



