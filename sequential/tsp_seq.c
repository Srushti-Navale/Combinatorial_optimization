#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "data_loader.h"
#include "subset.h"
#include "nearest_neighbor.h"
#include "two_opt.h"
#include "tsp_utils.h"

int main(int argc, char *argv[])
{
    /* Check command-line argument */
    if (argc != 2)
    {
        printf("Usage: %s <NUM_CITIES>\n", argv[0]);
        return 1;
    }

    int requested_n = atoi(argv[1]);

    if (requested_n <= 0)
    {
        printf("Number of cities must be positive.\n");
        return 1;
    }

    /* Load complete dataset */
    City *cities = NULL;

    int total_cities = load_cities("cities.csv", &cities);

    if (total_cities <= 0)
    {
        printf("Failed to load cities.csv\n");
        return 1;
    }

    /* Determine actual problem size */
    int n = get_subset_size(requested_n, total_cities);

    if (n < 2)
    {
        printf("Need at least 2 cities.\n");
        free_cities(cities);
        return 1;
    }

    printf("============================================================\n");
    printf("              SEQUENTIAL TSP (2-OPT)\n");
    printf("============================================================\n");

    printf("Total cities in dataset: %d\n", total_cities);
    printf("Cities used: %d\n", n);

    /* Allocate tour */
    int *tour = malloc(n * sizeof(int));

    if (tour == NULL)
    {
        printf("Failed to allocate memory for tour.\n");
        free_cities(cities);
        return 1;
    }

    /* Start timing */
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* Step 1: Nearest Neighbor */
    if (!nearest_neighbor(cities, n, tour))
    {
        printf("Nearest Neighbor failed.\n");
        free(tour);
        free_cities(cities);
        return 1;
    }

    double nn_length =
        calculate_tour_length(cities, tour, n);

    /* Step 2: 2-opt */
    two_opt(cities, n, tour);

    double final_length =
        calculate_tour_length(cities, tour, n);

    /* Stop timing */
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    /* Validate final tour */
    int valid = validate_tour(tour, n);

    printf("Nearest Neighbor length: %.2f\n", nn_length);
    printf("Final tour length: %.2f\n", final_length);
    printf("Tour valid: %s\n", valid ? "YES" : "NO");
    printf("Execution time: %.6f seconds\n", elapsed);

    /* Show first few cities of the tour */
    printf("Tour preview: ");

    int preview = n < 10 ? n : 10;

    for (int i = 0; i < preview; i++)
    {
        printf("%d ", tour[i]);
    }

    if (n > 10)
    {
        printf("...");
    }

    printf("\n");

    /* Save the final tour */
    FILE *out = fopen("best_tour.csv", "w");

    if (out != NULL)
    {
        fprintf(out, "Step,CityId\n");

        for (int i = 0; i < n; i++)
        {
            fprintf(out, "%d,%lld\n", i, cities[tour[i]].city_id);
        }

        /* Return to the starting city */
        fprintf(out, "%d,%lld\n",
                n,
                cities[tour[0]].city_id);

        fclose(out);

        printf("Best tour saved to best_tour.csv\n");
    }
    else
    {
        printf("Warning: Could not create best_tour.csv\n");
    }

    free(tour);
    free_cities(cities);

    return 0;
}