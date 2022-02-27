CFLAGS=-I.. -g3 -pthread 

EXECUTABLES=leader-election  mc_assert  mc_choose  mc_segfault

all: ${EXECUTABLES}

%: %.c
	gcc ${CFLAGS} -o $@ $<

check: ../mcmini all
	for file in *.c; do \
	  echo ""; \
	  echo "***** $$file *****"; \
	  ../mcmini --max-frontier-length 3000 `basename $$file .c`; \
	done

clean:
	rm -f ${EXECUTABLES} a.out
