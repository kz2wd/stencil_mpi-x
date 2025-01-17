CC      = mpicc
# CFLAGS += -Wall -g -O4 -DSTENCIL_SIZE=10 -DHALO_SIZE=3
LDLIBS += -lm -lrt -fopenmp

all: stencil stencil_mpi_horizontal stencil_mpi+x

clean:
	-rm stencil stencil_mpi_horizontal stencil_mpi+x

mrproper: clean
	-rm *~

archive: stencil.c Makefile
	( cd .. ; tar czf stencil.tar.gz stencil/ )
