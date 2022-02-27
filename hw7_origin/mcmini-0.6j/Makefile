FILE=pthread-simulator
TEST_FILE=test-simple/mc_assert.c
TEST_FILE=test-simple/mc_segfault.c
TEST_FILE=test-simple/mc_choose.c
TEST_FILE=test/mc-mutex-deadlock.cpp
TEST_EXE=${basename ${TEST_FILE}}
HEADERS=mc.h pthread-wrappers.h
LIB=libmc.so
# For verbose output, use:  ./mcmini --verbose a.out
CFLAGS=-g3 -O0

mcmini: mcmini.c ${LIB}
	gcc -o $@ ${CFLAGS} $<

${LIB}: pthread-simulator.o mc.o
	gcc -shared -o $@ $^ -ldl -pthread

%.o: %.c ${HEADERS}
	gcc ${CFLAGS} -fPIC -c $<

test/mc-mutex-deadlock: test/mc-mutex-deadlock.c mcmini.h
	gcc ${CFLAGS} -I. -pthread -o $@ $<
test/mc-philosophers-deadlock: test/mc-philosophers-deadlock.c mcmini.h
	gcc ${CFLAGS} -I. -pthread -o $@ $<

check-all: mcmini
	cd test-simple && make check
	cd test && make check

check: test/mc-mutex-deadlock mcmini
	./mcmini ./$<

check-verbose: test/mc-mutex-deadlock mcmini
	./mcmini --verbose ./$<


vi vim: ${FILE}.c
	vim $<

# We can hard-wire in the path, using -rpath.
#   To see the effect, do:  readelf -d a.out | grep PATH
# -lpthread needed here for pthread_create
# If we wanted it to be transparent, we could have compiled ${TEST_EXE}
#   by itself, and then done:  LE_PRELOAD=`pwd`/${LIB} ${TEST_EXE}
# ${TEST_EXE}: ${TEST_FILE} ${LIB}
# 	g++ ${CFLAGS} $< -o $@ -L. -lmc -lpthread \
# 	  -Wl,--enable-new-dtags,-rpath,'$$ORIGIN',-rpath,$$PWD
test: ${TEST_EXE}
	./${TEST_EXE}
# We can also direclty call:  mcmini a.out

# Initial command:  (gdb) break model_check
gdb: mcmini ${TEST_EXE} ${LIB}
	gdb -ex 'set breakpoint pending on' -ex 'break model_check' \
	    --args ./mcmini --verbose ./${TEST_EXE}

clean:
	rm -f a.out mcmini *.o *.so ${TEST_EXE}
	rm -f mc-philosophers-deadlock mc-mutex-deadlock
	cd test-simple && make clean
	cd test && make clean

distclean: clean

dist: distclean
	echo `basename $$PWD`|grep --quiet mcmini-`sed 's%[^0-9]*%%' VERSION` \
	  || (echo -e '\n*** VERSION file different from dirname\n' && exit 1)
	dir=`basename $$PWD` && cd .. && tar czvf $$dir.tgz ./$$dir
	dir=`basename $$PWD` && ls -l ../$$dir.tgz

dist-force: distclean
	dir=`basename $$PWD`  && newdir=mcmini-`sed 's%[^0-9]*%%' VERSION` && \
	  test ! -e ../$$newdir && cp -pr ../$$dir ../$$newdir && \
	  cd ../$$newdir && make dist && rm -rf ../$$newdir
	
