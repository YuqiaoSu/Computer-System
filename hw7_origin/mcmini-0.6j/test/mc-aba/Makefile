CFLAGS=-I../.. -g3 -pthread

CC=gcc

all: mc-aba-0 mc-aba-1 mc-aba-2 mc-aba-3 mc-aba-4 mc-aba-5

mc-aba-%: mc-aba-%.c ../../mcmini.h
	${CC} ${CFLAGS} -o $@ $<

clean:
	rm -f mc-aba-? a.out
dist: clean
	dir=`basename $$PWD` && cd .. && tar zcvf $$dir.tar.gz ./$$dir
	dir=`basename $$PWD` && ls -l ../$$dir.tar.gz
