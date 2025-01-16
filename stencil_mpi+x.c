
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
#include <unistd.h>

#include <mpi.h>
#include <omp.h>

#define RESET "\e[0m"

#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"

#define STENCIL_SIZE 1000000

#define HALO_SIZE 100

typedef float stencil_t;

/** conduction coeff used in computation */
static const stencil_t alpha = 0.02;

/** threshold for convergence */
static const stencil_t epsilon = 0.0001;

/** max number of steps */
static const int stencil_max_steps = 100;

static stencil_t *values = NULL;
static stencil_t *prev_values = NULL;

static int size_x = STENCIL_SIZE;
static int size_y = STENCIL_SIZE;

static int rank;
static int world_size;
static int tag;

#define TOP_HALO values
#define BOT_HALO (values + size_x * (HALO_SIZE + 1))
#define FIRST_ROW (values + size_x)
#define LAST_ROW (values + size_x * (HALO_SIZE))

#define IS_FIRST (rank == 0)
#define IS_LAST (rank == (world_size - 1))


/** init stencil values to 0, borders to non-zero */
static void stencil_init(void)
{
	tag = 1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	values = calloc(size_x * (HALO_SIZE + 2), sizeof(stencil_t));
	prev_values = calloc(size_x * (HALO_SIZE + 2), sizeof(stencil_t));

	if (IS_FIRST){
		for (int x = 0; x < size_x; x++) {
			FIRST_ROW[x] = x;
		}
	}
	if (IS_LAST) {
		for (int x = 0; x < size_x; x++)
		{
			FIRST_ROW[x + size_x * (HALO_SIZE - 1)] = size_x - x - 1;
		}	
	}
	
	
	for (int y = -1; y < HALO_SIZE + 1; y++)
	{
		FIRST_ROW[0 + size_x * y] = y + HALO_SIZE * rank;
		FIRST_ROW[size_x - 1 + size_x * y] = size_y - (y + HALO_SIZE * rank) - 1;
	}
	memcpy(prev_values, values, size_x * (HALO_SIZE + 2) * sizeof(stencil_t));
}

static void stencil_free(void)
{
	free(values);
	free(prev_values);
}

/** display a (part of) the stencil values */
static void stencil_display(int x0, int x1, int y0, int y1)
{
	if (rank % 2 == 0) printf(RED);
	else printf(GRN);
	int x, y;
	for (y = y0; y <= y1; y++)
	{
		for (x = x0; x <= x1; x++)
		{
			printf("%4.5g ", FIRST_ROW[x + size_x * y]);
		}
		printf("\n");
	}
	printf(RESET);
}

static int stencil_internal(void)
{
	int convergence = 1;

	int start = 1;
	if (IS_FIRST) {
		start = 2;
	}
	int end = HALO_SIZE + 1;
	if (IS_LAST) {
		end -= 1;
	}
	
	#pragma omp parallel for shared(values, prev_values, convergence) default(none) \
	firstprivate(size_x, start, end, alpha, epsilon) scehdule(static)
	for (int y = start; y < end; y++)
	{
		for (int x = 1; x < size_x - 1; x++)
		{
			values[x + size_x * y] =
				alpha * (prev_values[x - 1 + size_x * y] +
						 prev_values[x + 1 + size_x * y] +
						 prev_values[x + size_x * (y - 1)] +
						 prev_values[x + size_x * (y + 1)]) +
				(1.0 - 4.0 * alpha) * prev_values[x + size_x * y];
			
			#pragma omp critical
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
	MPI_Status statuses[2];
	MPI_Request requests[2];
	MPI_Isend(FIRST_ROW, size_x, MPI_FLOAT, rank - 1, tag, MPI_COMM_WORLD, &requests[0]);
	MPI_Irecv(TOP_HALO, size_x, MPI_FLOAT, rank - 1, tag, MPI_COMM_WORLD, &requests[1]);

	MPI_Waitall(2, requests, statuses);
}

static void handle_bot_neighbor() {
	if (IS_LAST) return;
	
	MPI_Status statuses[2];
	MPI_Request requests[2];
	MPI_Isend(LAST_ROW, size_x, MPI_FLOAT, rank + 1, tag, MPI_COMM_WORLD, &requests[1]);
	MPI_Irecv(BOT_HALO, size_x, MPI_FLOAT, rank + 1, tag, MPI_COMM_WORLD, &requests[0]);
	
	MPI_Waitall(2, requests, statuses);
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
	printf("STEP DONE\n");
	return convergence;
}

void mpi_stencil_display() {
	if (STENCIL_SIZE > 10 || HALO_SIZE > 10) return;
	MPI_Barrier(MPI_COMM_WORLD);
	for (int i = 0; i < world_size; ++i){
		if (i == rank){
			stencil_display(0, size_x - 1, 0, HALO_SIZE - 1);	
		}
		sleep(0.1);
		MPI_Barrier(MPI_COMM_WORLD);
	}
}

int main(int argc, char **argv)
{
	// MPI init for thread, I took it from chatGPT.
	int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);
	if (provided < MPI_THREAD_FUNNELED) {
        printf("MPI does not support the required threading level.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

	stencil_init();
	if (IS_FIRST) {
		printf("# init:\n");
	}
	mpi_stencil_display();

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
		MPI_Barrier(MPI_COMM_WORLD);
	}
	clock_gettime(CLOCK_MONOTONIC, &t2);
	const double t_usec = (t2.tv_sec - t1.tv_sec) * 1000000.0 + (t2.tv_nsec - t1.tv_nsec) / 1000.0;
	if (IS_FIRST) {
		printf("# steps = %d\n", s);
		printf("# time = %g usecs.\n", t_usec);
		printf("# gflops = %g\n", (6.0 * size_x * HALO_SIZE * world_size * s) / (t_usec * 1000));
	}
	mpi_stencil_display();
	stencil_free();

	MPI_Finalize();

	return 0;
}
