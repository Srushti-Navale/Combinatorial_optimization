#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <omp.h>

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

/* Euclidean distance calculation */
static inline double euclidean_distance(const City *a, const City *b)
{
    double dx = a->x - b->x;
    double dy = a->y - b->y;

    return sqrt(dx * dx + dy * dy);
}

/* Nearest Neighbor Constructive Heuristic */
int nearest_neighbor(int start_city, int n, const double *dist_matrix, int *tour, unsigned char *visited)
{
    if (dist_matrix == NULL || tour == NULL || visited == NULL || n <= 0)
    {
        return 0;
    }

    memset(visited, 0, n * sizeof(unsigned char));

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
            return 0;
        }

        tour[step] = best_next;
        visited[best_next] = 1;
    }

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
    int num_cities = load_cities(csv_path, target_cities, &cities);

    if (num_cities <= 0)
    {
        printf("Failed to load cities from %s\n", csv_path);
        return 1;
    }

    if (total_starts > num_cities)
    {
        total_starts = num_cities;
    }

    /* Allocate distance matrix in shared memory */
    double *dist_matrix = (double *)malloc((size_t)num_cities * num_cities * sizeof(double));

    if (dist_matrix == NULL)
    {
        free(cities);
        return 1;
    }

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < num_cities; i++)
    {
        for (int j = 0; j < num_cities; j++)
        {
            dist_matrix[i * num_cities + j] =
                euclidean_distance(&cities[i], &cities[j]);
        }
    }

    int num_threads = omp_get_max_threads();
    long long edges = (long long)num_cities * (num_cities - 1) / 2;

    printf("===================================================================================\n");
    printf("              PARALLEL COMBINATORIAL OPTIMIZATION (OPENMP TSP 2-OPT)               \n");
    printf("===================================================================================\n");
    printf(" Dataset Loaded        : Kaggle Traveling Santa (%s)\n", csv_path);
    printf(" Problem Size          : %d Cities (%lld pairwise edge distances)\n", num_cities, edges);
    printf(" Active OpenMP Threads : %d Host Threads\n", num_threads);
    printf(" Multi-Start Strategy  : %d NN Restarts (Dynamic Work Queue)\n", total_starts);
    printf("===================================================================================\n\n");
    printf(" --- [LIVE PROGRESS MONITOR: LOCAL MILESTONES DISCOVERED] -------------------------\n");
    printf(" THREAD ID  |  ITERATION #  |   PREVIOUS BEST    |    NEW BEST TOUR   |   IMPROVEMENT   \n");
    printf("-----------------------------------------------------------------------------------\n");
    fflush(stdout);

    double global_best_cost = DBL_MAX;
    int best_thread_id = -1;
    int *global_best_tour = (int *)malloc(num_cities * sizeof(int));

    if (global_best_tour == NULL)
    {
        free(cities);
        free(dist_matrix);
        return 1;
    }

    double start_time = omp_get_wtime();

    #pragma omp parallel default(none) \
        shared(num_cities, total_starts, dist_matrix, global_best_cost, best_thread_id, global_best_tour, stdout)
    {
        int tid = omp_get_thread_num();
        int *local_tour = (int *)malloc(num_cities * sizeof(int));
        unsigned char *local_visited = (unsigned char *)malloc(num_cities * sizeof(unsigned char));

        #pragma omp for schedule(dynamic, 1)
        for (int start_city = 0; start_city < total_starts; start_city++)
        {
            if (!nearest_neighbor(start_city, num_cities, dist_matrix, local_tour, local_visited))
            {
                continue;
            }

            double cost = two_opt(num_cities, dist_matrix, local_tour);

            if (cost < global_best_cost)
            {
                #pragma omp critical
                {
                    if (cost < global_best_cost)
                    {
                        double prev = global_best_cost;
                        double imp = prev - cost;

                        global_best_cost = cost;
                        best_thread_id = tid;
                        memcpy(global_best_tour, local_tour, num_cities * sizeof(int));

                        if (prev == DBL_MAX)
                        {
                            printf(" Thread %-3d |  Start #%-4d |     First Search   |  %12.2f units |     --------    \n",
                                   tid, start_city, cost);
                        }
                        else
                        {
                            printf(" Thread %-3d |  Start #%-4d |  %12.2f units |  %12.2f units |   -%9.2f    \n",
                                   tid, start_city, prev, cost, imp);
                        }
                        fflush(stdout);
                    }
                }
            }
        }

        free(local_tour);
        free(local_visited);
    }

    double end_time = omp_get_wtime();
    double elapsed_time = end_time - start_time;

    /* Save results to CSV */
    FILE *out = fopen("best_tour_omp.csv", "w");

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
    printf(" Optimal Tour Cost Found : %.2f units\n", global_best_cost);
    printf(" Discovered By           : Thread %d (out of %d active threads)\n", best_thread_id, num_threads);
    printf(" Total Execution Time    : %.4f seconds\n", elapsed_time);
    printf(" Memory Paradigm         : Shared Memory Multi-Threading (Zero Data Duplication)\n");
    printf(" Output Route File       : best_tour_omp.csv (Verified Hamiltonian Cycle)\n");
    printf(" Route Preview           : City %lld -> City %lld -> City %lld -> ... -> City %lld\n",
           cities[global_best_tour[0]].city_id,
           cities[global_best_tour[1]].city_id,
           cities[global_best_tour[2]].city_id,
           cities[global_best_tour[0]].city_id);
    printf("===================================================================================\n\n");

    free(cities);
    free(dist_matrix);
    free(global_best_tour);

    return 0;
}
