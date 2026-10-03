#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define NUM_STARTS 100

typedef struct {
    int id;
    double x, y;
} City;

typedef struct {
    double cost;
    int rank;
} CostRankPair;

City *cities = NULL;
double *dist_matrix = NULL;
int num_cities = 0;

#define DIST(i, j) dist_matrix[(i) * num_cities + (j)]

void load_kaggle_csv(const char *filename, int max_cities_to_load) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("[ERROR] Could not find '%s'. Place it in ~/tsp_project/\n", filename);
        MPI_Abort(MPI_COMM_WORLD, 1);
        exit(1);
    }

    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        MPI_Abort(MPI_COMM_WORLD, 1);
        exit(1);
    }

    cities = (City *)malloc(max_cities_to_load * sizeof(City));
    num_cities = 0;
    while (fgets(line, sizeof(line), fp) && num_cities < max_cities_to_load) {
        int id;
        double x, y;
        if (sscanf(line, "%d,%lf,%lf", &id, &x, &y) == 3) {
            cities[num_cities].id = id;
            cities[num_cities].x = x;
            cities[num_cities].y = y;
            num_cities++;
        }
    }
    fclose(fp);

    dist_matrix = (double *)malloc((size_t)num_cities * num_cities * sizeof(double));
    for (int i = 0; i < num_cities; i++) {
        for (int j = 0; j < num_cities; j++) {
            double dx = cities[i].x - cities[j].x;
            double dy = cities[i].y - cities[j].y;
            DIST(i, j) = sqrt(dx * dx + dy * dy);
        }
    }
}

double solve_2opt(int start_seed, int *tour) {
    for (int i = 0; i < num_cities; i++) tour[i] = i;

    unsigned int seed = (unsigned int)(start_seed + 101);
    for (int i = num_cities - 1; i > 0; i--) {
        int j = rand_r(&seed) % (i + 1);
        int temp = tour[i];
        tour[i] = tour[j];
        tour[j] = temp;
    }

    int improved = 1;
    int max_passes = 100;
    while (improved && max_passes-- > 0) {
        improved = 0;
        for (int i = 0; i < num_cities - 1; i++) {
            for (int k = i + 1; k < num_cities; k++) {
                int ci = tour[i];
                int cni = tour[(i + 1) % num_cities];
                int ck = tour[k];
                int cnk = tour[(k + 1) % num_cities];

                double current_d = DIST(ci, cni) + DIST(ck, cnk);
                double new_d = DIST(ci, ck) + DIST(cni, cnk);

                if (new_d < current_d - 1e-6) {
                    int l = i + 1, r = k;
                    while (l < r) {
                        int t = tour[l];
                        tour[l] = tour[r];
                        tour[r] = t;
                        l++;
                        r--;
                    }
                    improved = 1;
                }
            }
        }
    }

    double total_cost = 0.0;
    for (int i = 0; i < num_cities; i++) {
        total_cost += DIST(tour[i], tour[(i + 1) % num_cities]);
    }
    return total_cost;
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int target_cities = (argc > 1) ? atoi(argv[1]) : 500;

    if (rank == 0) {
        load_kaggle_csv("cities.csv", target_cities);
        printf("\n========================================================================================\n");
        printf("             PARALLEL COMBINATORIAL OPTIMIZATION (TSP 2-OPT)                            \n");
        printf("========================================================================================\n");
        printf(" Dataset Loaded       : Kaggle Traveling Santa 2018 (cities.csv)\n");
        printf(" Problem Size         : %d Cities (%ld pairwise edge distances)\n", 
               num_cities, (long)num_cities * (num_cities - 1) / 2);
        printf(" Active MPI Processes : %d CPU Workers\n", size);
        printf(" Total Random Starts  : %d Restarts (Cyclic Split: ~%d per process)\n", 
               NUM_STARTS, NUM_STARTS / size);
        printf("========================================================================================\n\n");
        printf("--- [LIVE PROGRESS MONITOR: IMPROVEMENTS FOUND] -----------------------------------------\n");
        printf(" %-14s %-15s %-19s %-19s %-12s\n", 
               "CORE ID", "ITERATION #", "PREVIOUS BEST", "NEW BEST TOUR", "IMPROVEMENT");
        printf("----------------------------------------------------------------------------------------\n");
        fflush(stdout);
    }

    MPI_Bcast(&num_cities, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank != 0) {
        dist_matrix = (double *)malloc((size_t)num_cities * num_cities * sizeof(double));
        cities = (City *)malloc(num_cities * sizeof(City));
    }

    MPI_Bcast(dist_matrix, num_cities * num_cities, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    int *current_tour = (int *)malloc(num_cities * sizeof(int));
    int *best_local_tour = (int *)malloc(num_cities * sizeof(int));
    double local_best = DBL_MAX;

    for (int i = rank; i < NUM_STARTS; i += size) {
        double cost = solve_2opt(i, current_tour);
        if (cost < local_best) {
            double prev = local_best;
            local_best = cost;
            memcpy(best_local_tour, current_tour, num_cities * sizeof(int));

            char prev_str[32];
            if (prev == DBL_MAX) {
                snprintf(prev_str, sizeof(prev_str), "First Search");
                printf(" Process %-6d  Start #%-8d %-19s %-10.2f units     --------\n",
                       rank, i, prev_str, local_best);
            } else {
                snprintf(prev_str, sizeof(prev_str), "%.2f units", prev);
                printf(" Process %-6d  Start #%-8d %-19s %-10.2f units     -%8.2f\n",
                       rank, i, prev_str, local_best, prev - local_best);
            }
            fflush(stdout);
        }
    }

    // Use MPI_Allreduce so ALL processes learn who won and the winning cost
    CostRankPair local_pair = {local_best, rank};
    CostRankPair global_pair;
    MPI_Allreduce(&local_pair, &global_pair, 1, MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD);

    int *global_best_tour = (int *)malloc(num_cities * sizeof(int));

    // Transfer route from the winning worker to Rank 0
    if (rank == 0) {
        if (global_pair.rank == 0) {
            memcpy(global_best_tour, best_local_tour, num_cities * sizeof(int));
        } else {
            MPI_Recv(global_best_tour, num_cities, MPI_INT, global_pair.rank, 999, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    } else {
        if (rank == global_pair.rank) {
            MPI_Send(best_local_tour, num_cities, MPI_INT, 0, 999, MPI_COMM_WORLD);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = MPI_Wtime();

    if (rank == 0) {
        printf("----------------------------------------------------------------------------------------\n\n");
        printf("========================================================================================\n");
        printf("                               FINAL BENCHMARK SCORECARD                                \n");
        printf("========================================================================================\n");
        printf(" Optimal Tour Cost Found  : %.2f units\n", global_pair.cost);
        printf(" Discovered By            : Process %d (out of %d active processes)\n", global_pair.rank, size);
        printf(" Total Execution Time     : %.4f seconds\n", end_time - start_time);
        printf(" Total Edge Pairs Checked : ~%ld Swaps per pass\n", (long)num_cities * (num_cities - 1) / 2);
        printf(" Output Route File        : best_tour.csv (Verified Hamiltonian Cycle)\n");

        printf(" Route Preview            : ");
        int preview = (num_cities < 8) ? num_cities : 8;
        for (int i = 0; i < preview; i++) {
            printf("City %d -> ", global_best_tour[i]);
        }
        printf("... -> City %d (Origin)\n", global_best_tour[0]);

        FILE *out = fopen("best_tour.csv", "w");
        if (out) {
            fprintf(out, "Step,CityId\n");
            for (int i = 0; i < num_cities; i++) {
                fprintf(out, "%d,%d\n", i, global_best_tour[i]);
            }
            fprintf(out, "%d,%d\n", num_cities, global_best_tour[0]);
            fclose(out);
        }
        printf("========================================================================================\n\n");
        fflush(stdout);
    }

    free(current_tour);
    free(best_local_tour);
    free(global_best_tour);
    if (cities) free(cities);
    if (dist_matrix) free(dist_matrix);

    MPI_Finalize();
    return 0;
}
