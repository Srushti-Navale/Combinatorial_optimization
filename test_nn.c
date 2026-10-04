#include <stdio.h>
#include <stdlib.h>

#include "data_loader.h"
#include "nearest_neighbor.h"
#include "tsp_utils.h"

int main(void)
{
    City *cities = NULL;

    int total = load_cities("cities.csv", &cities);

    if (total <= 0) {
        printf("Failed to load cities.\n");
        return 1;
    }

    /* Test with 10 cities */
    int n = 10;

    int *tour = malloc(n * sizeof(int));

    if (tour == NULL) {
        free_cities(cities);
        return 1;
    }

    if (!nearest_neighbor(cities, n, tour)) {
        printf("Nearest Neighbor failed.\n");
        free(tour);
        free_cities(cities);
        return 1;
    }

    printf("Nearest Neighbor tour:\n");

    for (int i = 0; i < n; i++) {
        printf("%d ", tour[i]);
    }

    printf("\n\n");

    printf("Tour valid: %s\n",
           validate_tour(tour, n) ? "YES" : "NO");

    printf("Tour length: %.2f\n",
           calculate_tour_length(cities, tour, n));

    free(tour);
    free_cities(cities);

    return 0;
}