CC = gcc
CFLAGS = -Wall -Isrc -std=c99 -g

.PHONY: all build clean

SRC_DIR = src
TST_DIR = test
INSTALL_DIR = install
MAIN_PROGRAM = main.o

SRCS = ${SRC_DIR}/*.c
EXAMPLE_OBJ = example.o
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
SIGNAL = signal_test.o

all: build

build: main ${MAIN_OBJ} ${MAIN_OBJ_p} ${SWITCH_OBJ} ${EQUITY_OBJ} ${JOIN_OBJ} ${JOIN-MAIN_OBJ} ${CREATE-MANY} ${CREATE-MANY-RECURSIVE} ${CREATE-MANY-ONCE} ${SWITCH_MANY} ${SWITCH_MANY_JOIN} ${SWITCH_MANY_CASCADE} ${FIBONACCI} ${MUTEX} ${MUTEX_2} ${PREEMPTION} ${DEADLOCK} ${SIGNAL} ${MUTEX_3} install

%.o: ${SRC_DIR}/%.c 
	${CC} ${CFLAGS} -fPIC -c $< -o $@

%_p.o: ${SRC_DIR}/%.c 
	${CC} ${CFLAGS} -fPIC -DUSE_PTHREAD -c $< -o $@
	
%.o: ${TST_DIR}/%.c
	${CC} ${CFLAGS} -fPIC -c $< -o $@

main: thread.o ${MAIN_PROGRAM} 
	${CC} ${CFLAGS} $^ -o $@

pthread_main: thread.o ${MAIN_PROGRAM}
	${CC} ${CFLAGS}  -DUSE_PTHREAD $^ -o $@

install: thread.o 

	mkdir ${INSTALL_DIR}
	mkdir ${INSTALL_DIR}/bin
	${CC} -shared $^ -o ${INSTALL_DIR}/libthread.so
	${CC} ${CFLAGS} ${MAIN_OBJ} thread.o -o ${INSTALL_DIR}/bin/01-main
	${CC} ${CFLAGS} ${SWITCH_OBJ} thread.o -o ${INSTALL_DIR}/bin/02-switch
	${CC} ${CFLAGS} ${EQUITY_OBJ} thread.o -o ${INSTALL_DIR}/bin/03-equity
	${CC} ${CFLAGS} ${JOIN_OBJ} thread.o -o ${INSTALL_DIR}/bin/11-join
	${CC} ${CFLAGS} ${JOIN-MAIN_OBJ} thread.o -o ${INSTALL_DIR}/bin/12-join-main
	${CC} ${CFLAGS} ${CREATE-MANY} thread.o -o ${INSTALL_DIR}/bin/21-create-many
	${CC} ${CFLAGS} ${CREATE-MANY-RECURSIVE} thread.o -o ${INSTALL_DIR}/bin/22-create-many-recursive
	${CC} ${CFLAGS} ${CREATE-MANY-ONCE} thread.o -o ${INSTALL_DIR}/bin/23-create-many-once
	${CC} ${CFLAGS} ${SWITCH_MANY} thread.o -o ${INSTALL_DIR}/bin/31-switch-many
	${CC} ${CFLAGS} ${SWITCH_MANY_JOIN} thread.o -o ${INSTALL_DIR}/bin/32-switch-many-join
	${CC} ${CFLAGS} ${SWITCH_MANY_CASCADE} thread.o -o ${INSTALL_DIR}/bin/33-switch-many-cascade
	${CC} ${CFLAGS} ${FIBONACCI} thread.o -o ${INSTALL_DIR}/bin/51-fibonacci
	${CC} ${CFLAGS} ${MUTEX} thread.o -o ${INSTALL_DIR}/bin/61-mutex
	${CC} ${CFLAGS} ${MUTEX_2} thread.o -o ${INSTALL_DIR}/bin/62-mutex
	${CC} ${CFLAGS} ${MUTEX_3} thread.o -o ${INSTALL_DIR}/bin/63-mutex
	${CC} ${CFLAGS} ${PREEMPTION} thread.o -o ${INSTALL_DIR}/bin/71-preemption
	${CC} ${CFLAGS} ${DEADLOCK} thread.o -o ${INSTALL_DIR}/bin/81-deadlock

clean:
	rm -f *.o example ${MAIN_OBJ} ${SWITCH_OBJ} ${EQUITY_OBJ} ${JOIN_OBJ} ${JOIN-MAIN_OBJ} ${CREATE-MANY} ${CREATE-MANY-RECURSIVE} ${CREATE-MANY-ONCE} ${SWITCH_MANY} ${SWITCH_MANY_JOIN} ${SWITCH_MANY_CASCADE} ${FIBONACCI} ${MUTEX} ${MUTEX_2} ${MUTEX_3} ${PREEMPTION} ${DEADLOCK} 
	rm -rf install
