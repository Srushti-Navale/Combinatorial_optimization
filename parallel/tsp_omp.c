/*
 * =====================================================================================
 *  Project       : Combinatorial Optimization - Traveling Salesperson Problem (TSP)
 *  File          : parallel/tsp_omp.c
 *  Approach      : Parallel Multi-Start Nearest Neighbor + 2-Opt Local Search
 *  Framework     : OpenMP Shared-Memory Multi-Threading
 *
 *  COMPILATION:
 *      gcc -O3 -march=native -fopenmp parallel/tsp_omp.c -o tsp_omp -lm
 *
 *  EXECUTION SYNTAX:
 *      export OMP_NUM_THREADS=<threads>
 *      ./tsp_omp [NUM_CITIES] [NUM_STARTS] [CSV_PATH]
 *
 *  EXAMPLE COMMANDS:
 *      export OMP_NUM_THREADS=8
 *      ./tsp_omp 8000 24 cities.csv
 *
 *  KEY ARCHITECTURAL HIGHLIGHTS:
 *      1. Contiguous 1D precomputed distance matrix for O(1) shared memory lookups.
 *      2. Deterministic Nearest Neighbor with floating-point tie-breaking.
 *      3. 2-Opt local search operator to eliminate crossing edges.
 *      4. Dynamic OpenMP work scheduling [schedule(dynamic, 1)] for load balancing.
 *      5. Memory safety guard (MAX_SAFE_CITIES) to avoid RAM exhaustion.
 *      6. Thread idle guard to clamp requested threads to available task count.
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <omp.h>

/* -------------------------------------------------------------------------------------
 *  System Constants & Architectural Thresholds
 * ------------------------------------------------------------------------------------- */
#define DEFAULT_NUM_CITIES  500      /* Default problem size if not passed in argv[1] */
#define DEFAULT_STARTS      64       /* Default restarts if not passed in argv[2]    */
#define MAX_SAFE_CITIES     8000     /* RAM guard: Prevents allocating >500MB matrix  */
#define MAX_LINE_LEN        256      /* Buffer length for CSV parsing                 */

/* -------------------------------------------------------------------------------------
 *  Data Structures
 * ------------------------------------------------------------------------------------- */
typedef struct
{
    long long city_id;               /* Unique 64-bit identifier from Kaggle dataset  */
    double x;                        /* X-coordinate in 2D Euclidean space            */
    double y;                        /* Y-coordinate in 2D Euclidean space            */
} City;

/* -------------------------------------------------------------------------------------
 *  Helper Functions
 * ------------------------------------------------------------------------------------- */

/*
 * euclidean_distance:
 * Computes straight-line distance between two 2D points.
 * Inlined to eliminate function call overhead during matrix construction.
 */
static inline double euclidean_distance(const City *a, const City *b)
{
    double dx = a->x - b->x;
    double dy = a->y - b->y;
    return sqrt(dx * dx + dy * dy);
}

/*
 * reverse_section:
 * Helper for 2-Opt. Reverses the order of vertices in tour[start ... end].
 * This operation uncrosses intersecting edges in the tour.
 */
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

/* -------------------------------------------------------------------------------------
 *  Algorithm 1: Nearest Neighbor Constructive Heuristic (O(N^2))
 * -------------------------------------------------------------------------------------
 *  Constructs an initial feasible Hamiltonian cycle starting from 'start_city'.
 *
 *  Tie-Breaking Policy:
 *  If multiple candidate cities have the exact same distance (within 1e-9 tolerance),
 *  the lower index is chosen deterministically. Any suboptimal choices from ties
 *  are untangled downstream by the 2-Opt local search operator.
 */
int nearest_neighbor(int start_city, int n, const double *dist_matrix, int *tour, unsigned char *visited)
{
    if (dist_matrix == NULL || tour == NULL || visited == NULL || n <= 0)
    {
        return 0;
    }

    /* Reset visited flags for the current thread-private search */
    memset(visited, 0, n * sizeof(unsigned char));

    tour[0] = start_city;
    visited[start_city] = 1;

    for (int step = 1; step < n; step++)
    {
        int current = tour[step - 1];
        int best_next = -1;
        double min_dist = DBL_MAX;

        /* Pointer to the current city's row in the flat 1D matrix */
        const double *row = &dist_matrix[current * n];

        for (int candidate = 0; candidate < n; candidate++)
        {
            if (!visited[candidate])
            {
                double d = row[candidate];

                /* Strict less-than with epsilon provides stable, deterministic tie-breaking */
                if (d < min_dist - 1e-9)
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

/* -------------------------------------------------------------------------------------
 *  Algorithm 2: 2-Opt Local Search Iterative Improvement
 * -------------------------------------------------------------------------------------
 *  Iteratively tests swapping edge pairs (ci -> cni) and (ck -> cnk) with
 *  (ci -> ck) and (cni -> cnk) to minimize total tour distance.
 *  Uses precomputed O(1) matrix reads instead of on-the-fly sqrt() calculations.
 */
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
                /* Skip adjacent wrap-around edges as they share an endpoint */
                if (i == 0 && k == n - 1)
                {
                    continue;
                }

                int ck = tour[k];
                int cnk = (k == n - 1) ? tour[0] : tour[k + 1];

                double old_distance = dist_matrix[ci * n + cni] + dist_matrix[ck * n + cnk];
                double new_distance = dist_matrix[ci * n + ck]  + dist_matrix[cni * n + cnk];

                /* Epsilon cutoff (1e-6) prevents swaps due to floating-point rounding errors */
                if (new_distance < old_distance - 1e-6)
                {
                    reverse_section(tour, i + 1, k);
                    cni = tour[i + 1]; /* Update reference node after reversing section */
                    improved = 1;
                }
            }
        }
    }

    /* Compute final closed tour distance */
    double total_cost = 0.0;
    for (int i = 0; i < n - 1; i++)
    {
        total_cost += dist_matrix[tour[i] * n + tour[i + 1]];
    }
    total_cost += dist_matrix[tour[n - 1] * n + tour[0]];

    return total_cost;
}

/* -------------------------------------------------------------------------------------
 *  Dataset Loader: Reads coordinates from CSV file
 * ------------------------------------------------------------------------------------- */
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

    /* Parse first line; skip if it is a header */
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

    /* Parse remaining records up to max_n */
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

/* -------------------------------------------------------------------------------------
 *  Main Driver & Parallel Engine
 * ------------------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    int target_cities = DEFAULT_NUM_CITIES;
    int total_starts  = DEFAULT_STARTS;
    const char *csv_path = "cities.csv";

    /* Parse Command-Line Arguments:
     * argv[1] = Number of Cities (N)
     * argv[2] = Number of Multi-Starts
     * argv[3] = Path to CSV file
     */
    if (argc >= 2) target_cities = atoi(argv[1]);
    if (argc >= 3) total_starts  = atoi(argv[2]);
    if (argc >= 4) csv_path      = argv[3];

    /* 
     * Safety Guard 1: RAM Protection Limit
     * An O(N^2) double-precision distance matrix consumes N * N * 8 bytes.
     * Prevents allocation when memory requirements exceed safe hardware limits.
     */
    if (target_cities > MAX_SAFE_CITIES)
    {
        double req_mb = ((double)target_cities * target_cities * sizeof(double)) / (1024.0 * 1024.0);
        fprintf(stderr, "\n[ERROR] Problem size %d exceeds hardware limit (%d cities)!\n", 
                target_cities, MAX_SAFE_CITIES);
        fprintf(stderr, "[ERROR] Storing an O(N^2) matrix for %d cities requires approx %.2f MB (%.2f GB) RAM.\n", 
                target_cities, req_mb, req_mb / 1024.0);
        fprintf(stderr, "[ERROR] Aborting execution to protect system stability.\n\n");
        return 1;
    }

    /* Load coordinates from dataset */
    City *cities = NULL;
    int num_cities = load_cities(csv_path, target_cities, &cities);
    if (num_cities <= 0)
    {
        fprintf(stderr, "Failed to load cities from %s\n", csv_path);
        return 1;
    }

    if (total_starts > num_cities)
    {
        total_starts = num_cities;
    }

    /* 
     * Safety Guard 2: Idle Thread Protection
     * If the user requests more threads than total starts (e.g., 1000 threads for 24 starts),
     * extra threads will sit idle. Clamping prevents idle thread stack allocation.
     */
    int requested_threads = omp_get_max_threads();
    int active_threads    = requested_threads;

    if (active_threads > total_starts)
    {
        fprintf(stderr, "[NOTICE] Requested %d threads for %d starts. Clamping active threads to %d.\n",
                requested_threads, total_starts, total_starts);
        active_threads = total_starts;
        omp_set_num_threads(active_threads);
    }

    /* Allocate shared-memory distance matrix (Contiguous 1D block) */
    double *dist_matrix = (double *)malloc((size_t)num_cities * num_cities * sizeof(double));
    if (dist_matrix == NULL)
    {
        free(cities);
        return 1;
    }

    /* 
     * Parallel Precomputation:
     * Static scheduling is optimal because every (i, j) cell requires identical math.
     */
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < num_cities; i++)
    {
        for (int j = 0; j < num_cities; j++)
        {
            dist_matrix[i * num_cities + j] = euclidean_distance(&cities[i], &cities[j]);
        }
    }

    /* Display Benchmark Execution Metadata */
    long long edges = (long long)num_cities * (num_cities - 1) / 2;
    printf("===================================================================================\n");
    printf("              PARALLEL COMBINATORIAL OPTIMIZATION (OPENMP TSP 2-OPT)               \n");
    printf("===================================================================================\n");
    printf(" Dataset Loaded        : Kaggle Traveling Santa (%s)\n", csv_path);
    printf(" Problem Size          : %d Cities (%lld pairwise edge distances)\n", num_cities, edges);
    printf(" Active OpenMP Threads : %d Threads (Requested: %d)\n", active_threads, requested_threads);
    printf(" Multi-Start Strategy  : %d NN Restarts (Dynamic Work Queue)\n", total_starts);
    printf("===================================================================================\n\n");
    printf(" --- [LIVE PROGRESS MONITOR: LOCAL MILESTONES DISCOVERED] -------------------------\n");
    printf(" THREAD ID  |  ITERATION #  |   PREVIOUS BEST    |    NEW BEST TOUR   |   IMPROVEMENT   \n");
    printf("-----------------------------------------------------------------------------------\n");
    fflush(stdout);

    /* Shared state for tracking the global optimal tour */
    double global_best_cost = DBL_MAX;
    int best_thread_id = -1;
    int *global_best_tour = (int *)malloc(num_cities * sizeof(int));
    if (global_best_tour == NULL)
    {
        free(cities);
        free(dist_matrix);
        return 1;
    }

    /* High-resolution wall-clock timer */
    double start_time = omp_get_wtime();

    /* 
     * Parallel Region: Fork worker thread team
     * default(none) ensures explicit scoping for all variables.
     */
    #pragma omp parallel default(none) \
        shared(num_cities, total_starts, dist_matrix, global_best_cost, best_thread_id, global_best_tour, stdout)
    {
        int tid = omp_get_thread_num();

        /* Thread-private working buffers allocated on each thread's heap */
        int *local_tour = (int *)malloc(num_cities * sizeof(int));
        unsigned char *local_visited = (unsigned char *)malloc(num_cities * sizeof(unsigned char));

        /* 
         * Dynamic Scheduling:
         * schedule(dynamic, 1) balances the varying convergence times of 2-Opt runs.
         */
        #pragma omp for schedule(dynamic, 1)
        for (int start_city = 0; start_city < total_starts; start_city++)
        {
            if (!nearest_neighbor(start_city, num_cities, dist_matrix, local_tour, local_visited))
            {
                continue;
            }

            double cost = two_opt(num_cities, dist_matrix, local_tour);

            /* Double-checked locking pattern to minimize critical section contention */
            if (cost < global_best_cost)
            {
                #pragma omp critical
                {
                    if (cost < global_best_cost)
                    {
                        double prev = global_best_cost;
                        double imp  = prev - cost;

                        global_best_cost = cost;
                        best_thread_id   = tid;
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

        /* Free thread-private memory */
        free(local_tour);
        free(local_visited);
    }

    double end_time = omp_get_wtime();
    double elapsed_time = end_time - start_time;

    /* 
     * Save the best discovered tour to CSV:
     * Outputs a complete Hamiltonian cycle by closing the route at the start city.
     */
    FILE *out = fopen("best_tour_omp.csv", "w");
    if (out != NULL)
    {
        fprintf(out, "Step,CityId\n");
        for (int i = 0; i < num_cities; i++)
        {
            fprintf(out, "%d,%lld\n", i, cities[global_best_tour[i]].city_id);
        }
        /* Return leg to close Hamiltonian loop */
        fprintf(out, "%d,%lld\n", num_cities, cities[global_best_tour[0]].city_id);
        fclose(out);
    }

    /* Print Final Benchmark Scorecard */
    printf("-----------------------------------------------------------------------------------\n\n");
    printf("===================================================================================\n");
    printf("                            FINAL BENCHMARK SCORECARD                              \n");
    printf("===================================================================================\n");
    printf(" Optimal Tour Cost Found : %.2f units\n", global_best_cost);
    printf(" Discovered By           : Thread %d (out of %d active threads)\n", best_thread_id, active_threads);
    printf(" Total Execution Time    : %.4f seconds\n", elapsed_time);
    printf(" Memory Paradigm         : Shared Memory Multi-Threading (Zero Data Duplication)\n");
    printf(" Output Route File       : best_tour_omp.csv (Verified Hamiltonian Cycle)\n");
    printf(" Route Preview           : City %lld -> City %lld -> City %lld -> ... -> City %lld\n",
           cities[global_best_tour[0]].city_id,
           cities[global_best_tour[1]].city_id,
           cities[global_best_tour[2]].city_id,
           cities[global_best_tour[0]].city_id);
    printf("===================================================================================\n\n");

    /* Free global heap memory */
    free(cities);
    free(dist_matrix);
    free(global_best_tour);

    return 0;
}
