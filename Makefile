CC      = gcc
CFLAGS += -Wall -g -O2
LDLIBS += -lm -lrt

all: stencil

clean:
	-rm stencil

mrproper: clean
	-rm *~

run_mpi:
	mpicc stencil_mpi_horizontal.c && mpirun -np 5 ./a.out

run_omp+mpi:
	mpicc stencil_mpi+x.c && mpirun -np 1 ./a.out

archive: stencil.c Makefile
	( cd .. ; tar czf stencil.tar.gz stencil/ )
