
/*

MPI STENCIL WITH SPLIT ALONG ONE DIMENSION ONLY

Each process computes HALO_SIZE rows.
We only send rows 0 and i + 1, which are ghost ones.

Pros : 

No need to inefficiently search the values to send to update other process
(or to find a better memory representation)


Cons :
More data to transfer compared to a square setup

*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <mpi.h>

#define STENCIL_SIZE 100

#define HALO_SIZE 52

typedef float stencil_t;

/** conduction coeff used in computation */
static const stencil_t alpha = 0.02;

/** threshold for convergence */
static const stencil_t epsilon = 0.0001;

/** max number of steps */
static const int stencil_max_steps = 100000;

static stencil_t *values = NULL;
static stencil_t *prev_values = NULL;

static int size_x = STENCIL_SIZE;
static int size_y = STENCIL_SIZE;

static int rank;
static int world_size;
static int tag;

#define TOP_HALO values
#define BOT_HALO values + size_x * (HALO_SIZE - 1)

#define IS_FIRST rank == 0
#define IS_LAST rank == (world_size - 1)


/** init stencil values to 0, borders to non-zero */
static void stencil_init(void)
{
	tag = 1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	values = calloc(size_x * HALO_SIZE, sizeof(stencil_t));
	prev_values = calloc(size_x * HALO_SIZE, sizeof(stencil_t));

	
	if (IS_FIRST){
		for (int x = 0; x < size_x; x++) {
			values[x + size_x * 0] = x;
		}
	}
	if (IS_LAST) {
		for (int x = 0; x < size_x; x++)
		{
			values[x + size_x * (HALO_SIZE - 1)] = size_x - x - 1;
		}	
	}
	
	
	for (int y = 0; y < HALO_SIZE; y++)
	{
		values[0 + size_x * y] = y + HALO_SIZE * rank;
		values[size_x - 1 + size_x * y] = size_y - (y + HALO_SIZE * rank) - 1;
	}
	memcpy(prev_values, values, size_x * size_y * sizeof(stencil_t));
}

static void stencil_free(void)
{
	free(values);
	free(prev_values);
}

/** display a (part of) the stencil values */
static void stencil_display(int x0, int x1, int y0, int y1)
{
	// Todo, for now, you don't display it
	return; 
	if (STENCIL_SIZE >= 20)
		return;
	int x, y;
	for (y = y0; y <= y1; y++)
	{
		for (x = x0; x <= x1; x++)
		{
			printf("%4.5g ", values[x + size_x * y]);
		}
		printf("\n");
	}
}

static int stencil_internal(void)
{
	int convergence = 1;

	for (int y = 1; y < HALO_SIZE - 1; y++)
	{
		for (int x = 1; x < size_x - 1; x++)
		{
			values[x + size_x * y] =
				alpha * (prev_values[x - 1 + size_x * y] +
						 prev_values[x + 1 + size_x * y] +
						 prev_values[x + size_x * (y - 1)] +
						 prev_values[x + size_x * (y + 1)]) +
				(1.0 - 4.0 * alpha) * prev_values[x + size_x * y];
			if (convergence && (fabs(prev_values[x + size_x * y] - values[x + size_x * y]) > epsilon))
			{
				convergence = 0;
			}
		}
	}
	return convergence;
}

static void handle_top_neighbor() {
	if (IS_FIRST) return;
	// Make it non lock, otherwise troubles :x
	MPI_Status status;
	MPI_Send(TOP_HALO, size_x, MPI_FLOAT, rank - 1, tag, MPI_COMM_WORLD);
	MPI_Recv(TOP_HALO, size_x, MPI_FLOAT, rank - 1, tag, MPI_COMM_WORLD, &status);
}

static void handle_bot_neighbor() {
	if (IS_LAST) return;
	MPI_Status status;
	MPI_Recv(BOT_HALO, size_x, MPI_FLOAT, rank + 1, tag, MPI_COMM_WORLD, &status);
	MPI_Send(BOT_HALO, size_x, MPI_FLOAT, rank + 1, tag, MPI_COMM_WORLD);	
}


static void handle_convergence(int *local_convergence) {
	MPI_Allreduce(local_convergence, local_convergence, 1, MPI_INT, MPI_LAND, MPI_COMM_WORLD);
}


static void stencil_communicate(int* convergence)
{
	// A lot to optimize here, but let's start with a simple version
	handle_top_neighbor();
	handle_bot_neighbor();

	handle_convergence(convergence);
}

/** compute the next stencil step, return 1 if computation has converged */
static int stencil_step(void)
{
	int convergence = 1;
	/* switch buffers */
	stencil_t *tmp = prev_values;
	prev_values = values;
	values = tmp;

	// Do internal
	convergence = stencil_internal();
	// Communicate adjacent rows
	stencil_communicate(&convergence);

	// barrier
	MPI_Barrier(MPI_COMM_WORLD);

	return convergence;
}

int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	stencil_init();
	if (IS_FIRST) {
		printf("# init:\n");
		// stencil_display(0, size_x - 1, 0, size_y - 1);
	}
	

	struct timespec t1, t2;
	clock_gettime(CLOCK_MONOTONIC, &t1);
	int s;
	for (s = 0; s < stencil_max_steps; s++)
	{
		int convergence = stencil_step();
		if (convergence)
		{
			break;
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &t2);
	const double t_usec = (t2.tv_sec - t1.tv_sec) * 1000000.0 + (t2.tv_nsec - t1.tv_nsec) / 1000.0;
	if (IS_FIRST) {
		printf("# steps = %d\n", s);
		printf("# time = %g usecs.\n", t_usec);
		printf("# gflops = %g\n", (6.0 * size_x * size_y * s) / (t_usec * 1000));
	// stencil_display(0, size_x - 1, 0, size_y - 1);	
	}
	stencil_free();

	MPI_Finalize();

	return 0;
}
