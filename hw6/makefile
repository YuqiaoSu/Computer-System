  # build an executable named myprog from myprog.c

FILE = sim.c

default: run

output: ${FILE}
	gcc -g -O0 -o output ${FILE}

run: output
	./output


  clean: 
	rm -f output a.out *~

# 'make' views $v as a make variable and expands $v into the value of v.
# By typing $$, make will reduce it to a single '$' and pass it to the shell.
# The shell will view $dir as a shell variable and expand it.
dist:
        dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
        dir=`basename $$PWD`; ls -l ../$$dir.tar.gz