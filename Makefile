CC      = gcc
CFLAGS += -Wall -g -O2
LDLIBS += -lm -lrt

all: stencil

clean:
	-rm stencil

mrproper: clean
	-rm *~

run_mpi:
	mpicc stencil_mpi_horizontal.c && mpirun -np 2 ./a.out

archive: stencil.c Makefile
	( cd .. ; tar czf stencil.tar.gz stencil/ )
