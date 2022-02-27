# First target is default target, if you just type:  make

FILE=condition-variable-example.c

default: run

run: reader-writer
	./reader-writer

gdb: reader-writer
	gdb --args reader-writer

reader-writer: ${FILE}
	gcc -g3 -O0 -o reader-writer ${FILE} -pthread

emacs: ${FILE}
	emacs ${FILE}
vi: ${FILE}
	vi ${FILE}

clean:
	rm -f reader-writer a.out *~

# 'make' views $v as a make variable and expands $v into the value of v.
# By typing $$, make will reduce it to a single '$' and pass it to the shell.
# The shell will view $dir as a shell variable and expand it.
dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
	dir=`basename $$PWD`; ls -l ../$$dir.tar.gz
