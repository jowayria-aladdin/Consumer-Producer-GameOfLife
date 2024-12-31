#define _XOPEN_SOURCE 600
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GRID_SIZE 20
#define NUM_THREADS 4
#define GENERATIONS 32

int grid[GRID_SIZE][GRID_SIZE];
int next_grid[GRID_SIZE][GRID_SIZE];
pthread_barrier_t barrier;

void print_grid()
{
    system("clear");
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            if (grid[i][j] == 1)
            {
                printf("# ");
            }
            else
            {
                printf("  ");
            }
        }
        printf("\n");
    }
    usleep(500000);
}
void *compute_next_gen(void *arg)
{
    int thread_id = *(int *)arg;
    free(arg);
    int start_row = thread_id * (GRID_SIZE / NUM_THREADS);
    int end_row = start_row + (GRID_SIZE / NUM_THREADS);
    int alive_count = 0;
    int gen;
    int i;
    for (gen = 0; gen < GENERATIONS; gen++)
    {
        for (i = start_row; i < end_row; i++)
        {
            int j;
            for (j = 0; j < GRID_SIZE; j++)
            {
                alive_count = 0;
                int dx;
                int dy;
                for (dx = -1; dx <= 1; dx++)
                {
                    for (dy = -1; dy <= 1; dy++)
                    {
                        if (dx == 0 && dy == 0)
                            continue;
                        int nx = i + dx;
                        int ny = j + dy;
                        if (nx >= 0 && nx < GRID_SIZE && ny >= 0 && ny < GRID_SIZE)
                        {
                            alive_count += grid[nx][ny];
                        }
                    }
                }
                if (grid[i][j] == 1)
                {
                    next_grid[i][j] = (alive_count < 2 || alive_count > 3) ? 0 : 1;
                }
                else
                {
                    next_grid[i][j] = (alive_count == 3) ? 1 : 0;
                }
            }
        }

        pthread_barrier_wait(&barrier);

        if (thread_id == 0)
        {
            for (int i = 0; i < GRID_SIZE; i++)
            {
                for (int j = 0; j < GRID_SIZE; j++)
                {
                    grid[i][j] = next_grid[i][j];
                }
            }
            print_grid();
        }

        pthread_barrier_wait(&barrier);
    }

    return NULL;
}


void initialize_grid(int grid[GRID_SIZE][GRID_SIZE])
{
    int i;
    int j;
    for (i = 0; i < GRID_SIZE; i++)
    {
        for (j = 0; j < GRID_SIZE; j++)
        {
            grid[i][j] = 0;
        }
    }
}

void initialize_patterns(int grid[GRID_SIZE][GRID_SIZE])
{
    initialize_grid(grid);

    grid[1][1] = 1;
    grid[1][2] = 1;
    grid[2][1] = 1;
    grid[2][2] = 1;

    grid[5][6] = 1;
    grid[6][6] = 1;
    grid[7][6] = 1;

    grid[10][10] = 1;
    grid[11][11] = 1;
    grid[12][9] = 1;
    grid[12][10] = 1;
    grid[12][11] = 1;
}

int main()
{
    initialize_patterns(grid);
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
    {
        int *thread_id = malloc(sizeof(int));
        *thread_id = i;
        pthread_create(&threads[i], NULL, compute_next_gen, (void *)thread_id);
    }
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }
    pthread_barrier_destroy(&barrier);

    return 0;
}
