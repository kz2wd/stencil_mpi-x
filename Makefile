CC      = gcc
CFLAGS += -Wall -g -O4
LDLIBS += -lm -lrt

all: stencil stencil_mpi_horizontal

clean:
	-rm stencil

mrproper: clean
	-rm *~

run_mpi:
	mpicc stencil_mpi_horizontal.c && mpirun -np 2 ./a.out

archive: stencil.c Makefile
	( cd .. ; tar czf stencil.tar.gz stencil/ )
