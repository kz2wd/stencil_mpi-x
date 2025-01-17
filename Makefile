CC      = mpicc
CFLAGS += -Wall -g -O2
LDLIBS += -lm -lrt -fopenmp

all: stencil stencil_mpi_horizontal stencil_mpi+x

clean:
	-rm stencil

mrproper: clean
	-rm *~

archive: stencil.c Makefile
	( cd .. ; tar czf stencil.tar.gz stencil/ )
