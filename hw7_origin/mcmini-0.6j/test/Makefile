CFLAGS=-I.. -g3 -pthread 

all: mc-circ-buffer mc-mutex-deadlock mc-philosophers-deadlock

mc-circ-buffer: mc-circ-buffer.c ../mcmini.h
	gcc ${CFLAGS} -o $@ $<

mc-mutex-deadlock: mc-mutex-deadlock.c ../mcmini.h
	gcc ${CFLAGS} -o $@ $<

# mc-mutex-deadlock: mc-mutex-deadlock.cpp
# 	g++ ${CFLAGS} -o $@ $<

mc-philosophers-deadlock: mc-philosophers-deadlock.c
	gcc ${CFLAGS} -o $@ $<

check: ../mcmini all
	for file in mc-[a-bd-z]*.c; do \
	  echo ""; \
	  echo "***** $$file *****"; \
	  ../mcmini --max-frontier-length 3000 `basename $$file .c`; \
	done

clean:
	rm -f mc-circ-buffer mc-mutex-deadlock mc-philosophers-deadlock
	cd mc-aba && make clean
