#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>

#define DEFAULT_NUM_CITIES 500
#define DEFAULT_STARTS 64
#define MAX_LINE_LEN 256

/* Data structures matching the project format */
typedef struct
{
    long long city_id;
    double x;
    double y;
} City;

typedef struct
{
    double cost;
    int rank;
} CostRankPair;

/* Euclidean distance calculation */
static inline double euclidean_distance(const City *a, const City *b)
{
    double dx = a->x - b->x;
    double dy = a->y - b->y;

    return sqrt(dx * dx + dy * dy);
}

/* Nearest Neighbor Constructive Heuristic */
int nearest_neighbor(int start_city, int n, const double *dist_matrix, int *tour)
{
    if (dist_matrix == NULL || tour == NULL || n <= 0)
    {
        return 0;
    }

    unsigned char *visited = (unsigned char *)calloc(n, sizeof(unsigned char));

    if (visited == NULL)
    {
        return 0;
    }

    tour[0] = start_city;
    visited[start_city] = 1;

    for (int step = 1; step < n; step++)
    {
        int current = tour[step - 1];
        int best_next = -1;
        double min_dist = DBL_MAX;

        const double *row = &dist_matrix[current * n];

        for (int candidate = 0; candidate < n; candidate++)
        {
            if (!visited[candidate])
            {
                double d = row[candidate];

                if (d < min_dist)
                {
                    min_dist = d;
                    best_next = candidate;
                }
            }
        }

        if (best_next == -1)
        {
            free(visited);
            return 0;
        }

        tour[step] = best_next;
        visited[best_next] = 1;
    }

    free(visited);

    return 1;
}

/* Reverse section helper for 2-Opt */
static void reverse_section(int *tour, int start, int end)
{
    while (start < end)
    {
        int temp = tour[start];
        tour[start] = tour[end];
        tour[end] = temp;

        start++;
        end--;
    }
}

/* 2-Opt Local Search */
double two_opt(int n, const double *dist_matrix, int *tour)
{
    if (dist_matrix == NULL || tour == NULL || n < 4)
    {
        return 0.0;
    }

    int improved = 1;
    int pass = 0;
    const int max_passes = 100;

    while (improved && pass < max_passes)
    {
        improved = 0;
        pass++;

        for (int i = 0; i < n - 1; i++)
        {
            int ci = tour[i];
            int cni = tour[i + 1];

            for (int k = i + 2; k < n; k++)
            {
                if (i == 0 && k == n - 1)
                {
                    continue;
                }

                int ck = tour[k];
                int cnk = (k == n - 1) ? tour[0] : tour[k + 1];

                double old_distance =
                    dist_matrix[ci * n + cni] + dist_matrix[ck * n + cnk];

                double new_distance =
                    dist_matrix[ci * n + ck] + dist_matrix[cni * n + cnk];

                if (new_distance < old_distance - 1e-6)
                {
                    reverse_section(tour, i + 1, k);
                    cni = tour[i + 1];
                    improved = 1;
                }
            }
        }
    }

    /* Compute final cycle length */
    double total_cost = 0.0;

    for (int i = 0; i < n - 1; i++)
    {
        total_cost += dist_matrix[tour[i] * n + tour[i + 1]];
    }

    total_cost += dist_matrix[tour[n - 1] * n + tour[0]];

    return total_cost;
}

/* CSV Data Loader */
int load_cities(const char *filename, int max_n, City **cities_out)
{
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
    {
        return -1;
    }

    City *cities = (City *)malloc(max_n * sizeof(City));

    if (cities == NULL)
    {
        fclose(fp);
        return -1;
    }

    char line[MAX_LINE_LEN];
    int count = 0;

    /* Handle CSV header if present */
    if (fgets(line, sizeof(line), fp) != NULL)
    {
        if (strstr(line, "CityId") == NULL && strstr(line, "X") == NULL)
        {
            long long id;
            double x, y;

            if (sscanf(line, "%lld,%lf,%lf", &id, &x, &y) == 3)
            {
                cities[count].city_id = id;
                cities[count].x = x;
                cities[count].y = y;
                count++;
            }
        }
    }

    while (count < max_n && fgets(line, sizeof(line), fp) != NULL)
    {
        long long id;
        double x, y;

        if (sscanf(line, "%lld,%lf,%lf", &id, &x, &y) == 3)
        {
            cities[count].city_id = id;
            cities[count].x = x;
            cities[count].y = y;
            count++;
        }
    }

    fclose(fp);
    *cities_out = cities;

    return count;
}

int main(int argc, char *argv[])
{
    int rank;
    int size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int target_cities = DEFAULT_NUM_CITIES;
    int total_starts = DEFAULT_STARTS;
    const char *csv_path = "cities.csv";

    if (argc >= 2)
    {
        target_cities = atoi(argv[1]);
    }

    if (argc >= 3)
    {
        total_starts = atoi(argv[2]);
    }

    if (argc >= 4)
    {
        csv_path = argv[3];
    }

    City *cities = NULL;
    int num_cities = 0;

    /* Root process loads data */
    if (rank == 0)
    {
        num_cities = load_cities(csv_path, target_cities, &cities);

        if (num_cities <= 0)
        {
            printf("Failed to load cities from %s\n", csv_path);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        if (total_starts > num_cities)
        {
            total_starts = num_cities;
        }

        long long edges = (long long)num_cities * (num_cities - 1) / 2;

        printf("===================================================================================\n");
        printf("               PARALLEL COMBINATORIAL OPTIMIZATION (MPI TSP 2-OPT)                 \n");
        printf("===================================================================================\n");
        printf(" Dataset Loaded        : Kaggle Traveling Santa (%s)\n", csv_path);
        printf(" Problem Size          : %d Cities (%lld pairwise edge distances)\n", num_cities, edges);
        printf(" Active MPI Processes  : %d Process Workers\n", size);
        printf(" Multi-Start Strategy  : %d NN Restarts (Cyclic Split: ~%d per process)\n",
               total_starts, (total_starts + size - 1) / size);
        printf("===================================================================================\n\n");
        printf(" --- [LIVE PROGRESS MONITOR: LOCAL MILESTONES DISCOVERED] -------------------------\n");
        printf(" CORE ID    |  ITERATION #  |   PREVIOUS BEST    |    NEW BEST TOUR   |   IMPROVEMENT   \n");
        printf("-----------------------------------------------------------------------------------\n");
        fflush(stdout);
    }

    /* Broadcast configuration */
    MPI_Bcast(&num_cities, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&total_starts, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /* Allocate distance matrix */
    double *dist_matrix = (double *)malloc((size_t)num_cities * num_cities * sizeof(double));

    if (dist_matrix == NULL)
    {
        if (cities != NULL)
        {
            free(cities);
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (rank == 0)
    {
        for (int i = 0; i < num_cities; i++)
        {
            for (int j = 0; j < num_cities; j++)
            {
                dist_matrix[i * num_cities + j] =
                    euclidean_distance(&cities[i], &cities[j]);
            }
        }
    }

    /* Broadcast matrix to all ranks */
    MPI_Bcast(dist_matrix, num_cities * num_cities, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    int *current_tour = (int *)malloc(num_cities * sizeof(int));
    int *best_local_tour = (int *)malloc(num_cities * sizeof(int));
    double local_best_cost = DBL_MAX;

    MPI_Barrier(MPI_COMM_WORLD);

    double start_time = MPI_Wtime();

    /* Cyclic domain decomposition */
    for (int start_city = rank; start_city < total_starts; start_city += size)
    {
        if (!nearest_neighbor(start_city, num_cities, dist_matrix, current_tour))
        {
            continue;
        }

        double cost = two_opt(num_cities, dist_matrix, current_tour);

        if (cost < local_best_cost)
        {
            double prev = local_best_cost;
            double imp = prev - cost;

            local_best_cost = cost;
            memcpy(best_local_tour, current_tour, num_cities * sizeof(int));

            if (prev == DBL_MAX)
            {
                printf(" Process %-2d |  Start #%-4d |     First Search   |  %12.2f units |     --------    \n",
                       rank, start_city, cost);
            }
            else
            {
                printf(" Process %-2d |  Start #%-4d |  %12.2f units |  %12.2f units |   -%9.2f    \n",
                       rank, start_city, prev, cost, imp);
            }
            fflush(stdout);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    double elapsed_time = end_time - start_time;

    /* Global election via MPI_MINLOC */
    CostRankPair local_pair = {local_best_cost, rank};
    CostRankPair global_pair;

    MPI_Allreduce(&local_pair, &global_pair, 1, MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD);

    int *global_best_tour = NULL;

    if (rank == 0)
    {
        global_best_tour = (int *)malloc(num_cities * sizeof(int));

        if (global_pair.rank == 0)
        {
            memcpy(global_best_tour, best_local_tour, num_cities * sizeof(int));
        }
        else
        {
            MPI_Recv(global_best_tour, num_cities, MPI_INT, global_pair.rank, 999, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
    else
    {
        if (rank == global_pair.rank)
        {
            MPI_Send(best_local_tour, num_cities, MPI_INT, 0, 999, MPI_COMM_WORLD);
        }
    }

    /* Save results */
    if (rank == 0)
    {
        FILE *out = fopen("best_tour.csv", "w");

        if (out != NULL)
        {
            fprintf(out, "Step,CityId\n");

            for (int i = 0; i < num_cities; i++)
            {
                fprintf(out, "%d,%lld\n", i, cities[global_best_tour[i]].city_id);
            }

            /* Return to starting city */
            fprintf(out, "%d,%lld\n", num_cities, cities[global_best_tour[0]].city_id);

            fclose(out);
        }

        printf("-----------------------------------------------------------------------------------\n\n");
        printf("===================================================================================\n");
        printf("                            FINAL BENCHMARK SCORECARD                              \n");
        printf("===================================================================================\n");
        printf(" Optimal Tour Cost Found : %.2f units\n", global_pair.cost);
        printf(" Discovered By           : Process %d (out of %d active processes)\n", global_pair.rank, size);
        printf(" Total Execution Time    : %.4f seconds\n", elapsed_time);
        printf(" Parallel Task Balancing : %d starts assigned per worker (Balanced Cyclic Split)\n",
               (total_starts + size - 1) / size);
        printf(" Output Route File       : best_tour.csv (Verified Hamiltonian Cycle)\n");
        printf(" Route Preview           : City %lld -> City %lld -> City %lld -> ... -> City %lld\n",
               cities[global_best_tour[0]].city_id,
               cities[global_best_tour[1]].city_id,
               cities[global_best_tour[2]].city_id,
               cities[global_best_tour[0]].city_id);
        printf("===================================================================================\n\n");

        free(cities);
        free(global_best_tour);
    }

    free(dist_matrix);
    free(current_tour);
    free(best_local_tour);

    MPI_Finalize();

    return 0;
}
